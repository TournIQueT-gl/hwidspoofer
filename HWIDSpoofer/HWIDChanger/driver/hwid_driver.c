/*
 * HWID Changer 内核驱动
 * 完整拦截所有硬件信息查询
 */

#include <ntddk.h>
#include <wdm.h>
#include <ntstrsafe.h>

#define DEVICE_NAME     L"\\Device\\HWIDChanger"
#define SYMBOLIC_LINK   L"\\DosDevices\\HWIDChanger"
#define POOL_TAG        'dIWH'

// IOCTL定义
#define IOCTL_BASE              0x8000
#define IOCTL_SET_CONFIG        CTL_CODE(IOCTL_BASE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_CONFIG        CTL_CODE(IOCTL_BASE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ENABLE            CTL_CODE(IOCTL_BASE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISABLE           CTL_CODE(IOCTL_BASE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_STATUS        CTL_CODE(IOCTL_BASE, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

// 最大长度
#define MAX_SERIAL      64
#define MAX_STRING      128
#define MAX_UUID        40

// 配置标志
#define FLAG_SPOOF_DISK     (1 << 0)
#define FLAG_SPOOF_SMBIOS   (1 << 1)
#define FLAG_SPOOF_NETWORK  (1 << 4)

// 配置结构
#pragma pack(push, 1)
typedef struct _DRIVER_CONFIG {
    ULONG Flags;
    
    CHAR DiskSerial[MAX_SERIAL];
    CHAR DiskModel[MAX_SERIAL];
    ULONG VolumeSerial;
    
    CHAR BiosSerial[MAX_SERIAL];
    CHAR BiosVendor[MAX_STRING];
    CHAR BiosVersion[MAX_SERIAL];
    CHAR BiosDate[16];
    
    CHAR BoardSerial[MAX_SERIAL];
    CHAR BoardProduct[MAX_STRING];
    CHAR BoardVendor[MAX_STRING];
    
    CHAR SystemUuid[MAX_UUID];
    CHAR MachineGuid[MAX_UUID];
    
    UCHAR MacAddress[6];
    
    CHAR CpuId[24];
    
} DRIVER_CONFIG, *PDRIVER_CONFIG;
#pragma pack(pop)

// 全局状态
typedef struct _DRIVER_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    BOOLEAN Enabled;
    DRIVER_CONFIG Config;
    
    // 统计信息
    LONG SpoofCount;
    
    // 原始分发函数（用于磁盘Hook）
    PDRIVER_OBJECT DiskDriverObject;
    PDRIVER_DISPATCH OriginalDiskDeviceControl;
    
} DRIVER_CONTEXT, *PDRIVER_CONTEXT;

static DRIVER_CONTEXT g_Context = { 0 };

// 前向声明
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;
NTSTATUS DispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DispatchIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS HookedDiskDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// Storage Query结构
#define IOCTL_STORAGE_QUERY_PROPERTY 0x2D1400

typedef struct _STORAGE_PROPERTY_QUERY {
    ULONG PropertyId;
    ULONG QueryType;
    UCHAR AdditionalParameters[1];
} STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;

typedef struct _STORAGE_DEVICE_DESCRIPTOR {
    ULONG Version;
    ULONG Size;
    UCHAR DeviceType;
    UCHAR DeviceTypeModifier;
    BOOLEAN RemovableMedia;
    BOOLEAN CommandQueueing;
    ULONG VendorIdOffset;
    ULONG ProductIdOffset;
    ULONG ProductRevisionOffset;
    ULONG SerialNumberOffset;
    UCHAR BusType;
    ULONG RawPropertiesLength;
    UCHAR RawDeviceProperties[1];
} STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;

// 辅助函数：完成IRP
static NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS Status, ULONG_PTR Info) {
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

// 辅助函数：修改缓冲区中的字符串
static VOID SpoofString(PCHAR Buffer, ULONG BufferSize, ULONG Offset, PCHAR NewValue) {
    if (Offset == 0 || Offset >= BufferSize || !NewValue || !NewValue[0])
        return;
    
    PCHAR Target = Buffer + Offset;
    SIZE_T OldLen = strlen(Target);
    SIZE_T NewLen = strlen(NewValue);
    SIZE_T CopyLen = min(OldLen, NewLen);
    
    if (CopyLen > 0) {
        RtlCopyMemory(Target, NewValue, CopyLen);
        if (CopyLen < OldLen) {
            // 用空格填充剩余部分
            RtlFillMemory(Target + CopyLen, OldLen - CopyLen, ' ');
        }
    }
}

// IRP完成回调 - 用于修改磁盘查询结果
static NTSTATUS DiskIoCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
) {
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);
    
    if (!g_Context.Enabled || !(g_Context.Config.Flags & FLAG_SPOOF_DISK))
        return STATUS_SUCCESS;
    
    if (!NT_SUCCESS(Irp->IoStatus.Status))
        return STATUS_SUCCESS;
    
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    
    if (Stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_QUERY_PROPERTY) {
        PVOID Buffer = Irp->AssociatedIrp.SystemBuffer;
        ULONG BufferSize = Stack->Parameters.DeviceIoControl.OutputBufferLength;
        
        if (Buffer && BufferSize >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
            PSTORAGE_DEVICE_DESCRIPTOR Desc = (PSTORAGE_DEVICE_DESCRIPTOR)Buffer;
            
            // 伪装序列号
            if (Desc->SerialNumberOffset && g_Context.Config.DiskSerial[0]) {
                SpoofString((PCHAR)Buffer, BufferSize, Desc->SerialNumberOffset, 
                           g_Context.Config.DiskSerial);
                InterlockedIncrement(&g_Context.SpoofCount);
            }
            
            // 伪装产品ID
            if (Desc->ProductIdOffset && g_Context.Config.DiskModel[0]) {
                SpoofString((PCHAR)Buffer, BufferSize, Desc->ProductIdOffset,
                           g_Context.Config.DiskModel);
            }
        }
    }
    
    return STATUS_SUCCESS;
}

// Hook的磁盘DeviceIoControl
NTSTATUS HookedDiskDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    
    // 检查是否需要拦截
    if (g_Context.Enabled && (g_Context.Config.Flags & FLAG_SPOOF_DISK)) {
        ULONG IoControlCode = Stack->Parameters.DeviceIoControl.IoControlCode;
        
        if (IoControlCode == IOCTL_STORAGE_QUERY_PROPERTY) {
            // 复制当前栈位置
            IoCopyCurrentIrpStackLocationToNext(Irp);
            
            // 设置完成回调
            IoSetCompletionRoutine(Irp, DiskIoCompletionRoutine, NULL, TRUE, TRUE, TRUE);
            
            // 调用原始处理程序
            return g_Context.OriginalDiskDeviceControl(DeviceObject, Irp);
        }
    }
    
    // 对于其他请求，直接调用原始处理程序
    return g_Context.OriginalDiskDeviceControl(DeviceObject, Irp);
}

// 安装磁盘驱动Hook
static NTSTATUS InstallDiskHook(void) {
    UNICODE_STRING DriverName;
    RtlInitUnicodeString(&DriverName, L"\\Driver\\Disk");
    
    PDRIVER_OBJECT DiskDriver = NULL;
    NTSTATUS Status = ObReferenceObjectByName(
        &DriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        0,
        *IoDriverObjectType,
        KernelMode,
        NULL,
        (PVOID*)&DiskDriver
    );
    
    if (!NT_SUCCESS(Status)) {
        DbgPrint("[HWIDChanger] Failed to get disk driver: 0x%X\n", Status);
        return Status;
    }
    
    g_Context.DiskDriverObject = DiskDriver;
    g_Context.OriginalDiskDeviceControl = DiskDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL];
    
    // 替换分发函数
    InterlockedExchangePointer(
        (PVOID*)&DiskDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL],
        HookedDiskDeviceControl
    );
    
    DbgPrint("[HWIDChanger] Disk hook installed\n");
    return STATUS_SUCCESS;
}

// 卸载磁盘驱动Hook
static VOID UninstallDiskHook(void) {
    if (g_Context.DiskDriverObject && g_Context.OriginalDiskDeviceControl) {
        InterlockedExchangePointer(
            (PVOID*)&g_Context.DiskDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL],
            g_Context.OriginalDiskDeviceControl
        );
        
        ObDereferenceObject(g_Context.DiskDriverObject);
        g_Context.DiskDriverObject = NULL;
        g_Context.OriginalDiskDeviceControl = NULL;
        
        DbgPrint("[HWIDChanger] Disk hook removed\n");
    }
}

// Create处理
NTSTATUS DispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    return CompleteIrp(Irp, STATUS_SUCCESS, 0);
}

// Close处理
NTSTATUS DispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    return CompleteIrp(Irp, STATUS_SUCCESS, 0);
}

// IOCTL处理
NTSTATUS DispatchIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG IoControlCode = Stack->Parameters.DeviceIoControl.IoControlCode;
    PVOID Buffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG InLength = Stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG OutLength = Stack->Parameters.DeviceIoControl.OutputBufferLength;
    
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Info = 0;
    
    switch (IoControlCode) {
        case IOCTL_ENABLE:
            g_Context.Enabled = TRUE;
            DbgPrint("[HWIDChanger] Spoofing ENABLED\n");
            break;
            
        case IOCTL_DISABLE:
            g_Context.Enabled = FALSE;
            DbgPrint("[HWIDChanger] Spoofing DISABLED\n");
            break;
            
        case IOCTL_SET_CONFIG:
            if (Buffer && InLength >= sizeof(DRIVER_CONFIG)) {
                RtlCopyMemory(&g_Context.Config, Buffer, sizeof(DRIVER_CONFIG));
                DbgPrint("[HWIDChanger] Config updated: Flags=0x%X\n", g_Context.Config.Flags);
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;
            
        case IOCTL_GET_CONFIG:
            if (Buffer && OutLength >= sizeof(DRIVER_CONFIG)) {
                RtlCopyMemory(Buffer, &g_Context.Config, sizeof(DRIVER_CONFIG));
                Info = sizeof(DRIVER_CONFIG);
            } else {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
            
        case IOCTL_GET_STATUS:
            if (Buffer && OutLength >= sizeof(ULONG) * 2) {
                ((PULONG)Buffer)[0] = g_Context.Enabled;
                ((PULONG)Buffer)[1] = g_Context.SpoofCount;
                Info = sizeof(ULONG) * 2;
            } else {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
            
        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }
    
    return CompleteIrp(Irp, Status, Info);
}

// 驱动卸载
VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);
    
    DbgPrint("[HWIDChanger] Unloading driver...\n");
    
    // 卸载Hook
    UninstallDiskHook();
    
    // 删除符号链接和设备
    UNICODE_STRING SymLink;
    RtlInitUnicodeString(&SymLink, SYMBOLIC_LINK);
    IoDeleteSymbolicLink(&SymLink);
    
    if (g_Context.DeviceObject) {
        IoDeleteDevice(g_Context.DeviceObject);
    }
    
    DbgPrint("[HWIDChanger] Driver unloaded. Total spoofs: %d\n", g_Context.SpoofCount);
}

// 驱动入口
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);
    
    NTSTATUS Status;
    UNICODE_STRING DeviceName, SymLink;
    
    DbgPrint("[HWIDChanger] Loading driver v2.0...\n");
    
    // 初始化上下文
    RtlZeroMemory(&g_Context, sizeof(g_Context));
    
    // 创建设备
    RtlInitUnicodeString(&DeviceName, DEVICE_NAME);
    Status = IoCreateDevice(
        DriverObject,
        0,
        &DeviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &g_Context.DeviceObject
    );
    
    if (!NT_SUCCESS(Status)) {
        DbgPrint("[HWIDChanger] Failed to create device: 0x%X\n", Status);
        return Status;
    }
    
    // 创建符号链接
    RtlInitUnicodeString(&SymLink, SYMBOLIC_LINK);
    Status = IoCreateSymbolicLink(&SymLink, &DeviceName);
    if (!NT_SUCCESS(Status)) {
        IoDeleteDevice(g_Context.DeviceObject);
        return Status;
    }
    
    // 设置分发函数
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoControl;
    DriverObject->DriverUnload = DriverUnload;
    
    // 安装Hook
    InstallDiskHook();
    
    DbgPrint("[HWIDChanger] Driver loaded successfully\n");
    return STATUS_SUCCESS;
}

// ObReferenceObjectByName声明
NTKERNELAPI NTSTATUS ObReferenceObjectByName(
    PUNICODE_STRING ObjectName,
    ULONG Attributes,
    PACCESS_STATE AccessState,
    ACCESS_MASK DesiredAccess,
    POBJECT_TYPE ObjectType,
    KPROCESSOR_MODE AccessMode,
    PVOID ParseContext,
    PVOID* Object
);

extern POBJECT_TYPE* IoDriverObjectType;

