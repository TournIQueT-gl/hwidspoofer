/*
 * IRP Hook模块 - 全局拦截所有进程的硬件查询
 * 
 * 通过Hook磁盘驱动的IRP分发函数，实现全系统级别的HWID伪装
 */

#include <ntddk.h>
#include <ntdddisk.h>
#include <ntddscsi.h>

// 外部全局配置
extern struct _DRIVER_GLOBALS g_Globals;

// 原始分发函数指针
static PDRIVER_DISPATCH g_OriginalDiskControl = NULL;
static PDRIVER_DISPATCH g_OriginalScsiControl = NULL;
static PDRIVER_OBJECT g_DiskDriverObject = NULL;
static PDRIVER_OBJECT g_ScsiDriverObject = NULL;

// IOCTL定义
#define IOCTL_STORAGE_QUERY_PROPERTY        0x002D1400
#define IOCTL_DISK_GET_DRIVE_GEOMETRY       0x00070000
#define IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS 0x00560000
#define IOCTL_ATA_PASS_THROUGH              0x0004D02C
#define IOCTL_ATA_PASS_THROUGH_DIRECT       0x0004D030
#define SMART_RCV_DRIVE_DATA                0x0007C088

// Storage Property ID
#define StorageDeviceProperty       0
#define StorageAdapterProperty      1
#define StorageDeviceIdProperty     2
#define StorageDeviceUniqueIdProperty 3

#pragma pack(push, 1)
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
    ULONG BusType;
    ULONG RawPropertiesLength;
    UCHAR RawDeviceProperties[1];
} STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;

typedef struct _STORAGE_DEVICE_ID_DESCRIPTOR {
    ULONG Version;
    ULONG Size;
    ULONG NumberOfIdentifiers;
    UCHAR Identifiers[1];
} STORAGE_DEVICE_ID_DESCRIPTOR, *PSTORAGE_DEVICE_ID_DESCRIPTOR;
#pragma pack(pop)

// 字符串覆盖辅助函数
VOID SpoofString(PCHAR Target, PCHAR Source, SIZE_T MaxLen)
{
    if (!Target || !Source || !Source[0])
        return;
    
    SIZE_T srcLen = strlen(Source);
    SIZE_T tgtLen = strlen(Target);
    SIZE_T copyLen = min(srcLen, min(tgtLen, MaxLen - 1));
    
    if (copyLen > 0)
    {
        RtlCopyMemory(Target, Source, copyLen);
        // 填充空格保持原长度
        if (copyLen < tgtLen)
        {
            RtlFillMemory(Target + copyLen, tgtLen - copyLen, ' ');
        }
    }
}

// IRP完成例程 - 修改返回的数据
NTSTATUS StorageQueryCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    
    PIO_STACK_LOCATION stack = (PIO_STACK_LOCATION)Context;
    
    if (!g_Globals.Enabled || !NT_SUCCESS(Irp->IoStatus.Status))
    {
        if (Irp->PendingReturned)
            IoMarkIrpPending(Irp);
        return STATUS_SUCCESS;
    }
    
    // 检查是否是Storage Query Property
    if (stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_QUERY_PROPERTY)
    {
        PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
        ULONG outputLen = (ULONG)Irp->IoStatus.Information;
        
        if (buffer && outputLen >= sizeof(STORAGE_DEVICE_DESCRIPTOR))
        {
            PSTORAGE_DEVICE_DESCRIPTOR desc = (PSTORAGE_DEVICE_DESCRIPTOR)buffer;
            
            // 伪装序列号
            if (desc->SerialNumberOffset > 0 && 
                desc->SerialNumberOffset < outputLen &&
                g_Globals.DiskSerial[0])
            {
                PCHAR serial = (PCHAR)buffer + desc->SerialNumberOffset;
                SpoofString(serial, g_Globals.DiskSerial, 64);
                DbgPrint("[HWIDSpoofer] Spoofed disk serial in STORAGE_QUERY\n");
            }
            
            // 伪装产品ID
            if (desc->ProductIdOffset > 0 && 
                desc->ProductIdOffset < outputLen &&
                g_Globals.DiskProductId[0])
            {
                PCHAR productId = (PCHAR)buffer + desc->ProductIdOffset;
                SpoofString(productId, g_Globals.DiskProductId, 64);
                DbgPrint("[HWIDSpoofer] Spoofed disk product ID\n");
            }
        }
    }
    
    if (Irp->PendingReturned)
        IoMarkIrpPending(Irp);
    
    return STATUS_SUCCESS;
}

// Hook后的磁盘DeviceIoControl处理
NTSTATUS HookedDiskDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG ioctl = stack->Parameters.DeviceIoControl.IoControlCode;
    
    // 需要拦截的IOCTL
    BOOLEAN needHook = FALSE;
    
    switch (ioctl)
    {
        case IOCTL_STORAGE_QUERY_PROPERTY:
        case IOCTL_ATA_PASS_THROUGH:
        case IOCTL_ATA_PASS_THROUGH_DIRECT:
        case SMART_RCV_DRIVE_DATA:
            needHook = g_Globals.Enabled && g_Globals.DiskSpoofEnabled;
            break;
    }
    
    if (needHook)
    {
        // 复制当前栈位置信息用于完成例程
        PIO_STACK_LOCATION stackCopy = ExAllocatePoolWithTag(NonPagedPool, 
            sizeof(IO_STACK_LOCATION), 'kooH');
        
        if (stackCopy)
        {
            RtlCopyMemory(stackCopy, stack, sizeof(IO_STACK_LOCATION));
            
            // 设置完成例程
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp, StorageQueryCompletion, stackCopy, 
                TRUE, TRUE, TRUE);
            
            return IoCallDriver(DeviceObject, Irp);
        }
    }
    
    // 调用原始处理函数
    if (g_OriginalDiskControl)
    {
        return g_OriginalDiskControl(DeviceObject, Irp);
    }
    
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_SUPPORTED;
}

// 安装IRP Hook
NTSTATUS InstallIrpHook()
{
    NTSTATUS status;
    UNICODE_STRING driverName;
    
    DbgPrint("[HWIDSpoofer] Installing IRP hooks...\n");
    
    // Hook 磁盘驱动 (disk.sys)
    RtlInitUnicodeString(&driverName, L"\\Driver\\Disk");
    status = ObReferenceObjectByName(&driverName, OBJ_CASE_INSENSITIVE, NULL,
        0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)&g_DiskDriverObject);
    
    if (NT_SUCCESS(status) && g_DiskDriverObject)
    {
        g_OriginalDiskControl = g_DiskDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];
        InterlockedExchangePointer(
            (PVOID*)&g_DiskDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL],
            HookedDiskDeviceControl);
        DbgPrint("[HWIDSpoofer] Hooked disk.sys DeviceIoControl\n");
    }
    else
    {
        DbgPrint("[HWIDSpoofer] Failed to get disk driver object: 0x%X\n", status);
    }
    
    // Hook partmgr 驱动
    RtlInitUnicodeString(&driverName, L"\\Driver\\partmgr");
    status = ObReferenceObjectByName(&driverName, OBJ_CASE_INSENSITIVE, NULL,
        0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)&g_ScsiDriverObject);
    
    if (NT_SUCCESS(status) && g_ScsiDriverObject)
    {
        g_OriginalScsiControl = g_ScsiDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];
        InterlockedExchangePointer(
            (PVOID*)&g_ScsiDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL],
            HookedDiskDeviceControl);
        DbgPrint("[HWIDSpoofer] Hooked partmgr DeviceIoControl\n");
    }
    
    return STATUS_SUCCESS;
}

// 卸载IRP Hook
VOID UninstallIrpHook()
{
    DbgPrint("[HWIDSpoofer] Uninstalling IRP hooks...\n");
    
    // 恢复磁盘驱动
    if (g_DiskDriverObject && g_OriginalDiskControl)
    {
        InterlockedExchangePointer(
            (PVOID*)&g_DiskDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL],
            g_OriginalDiskControl);
        ObDereferenceObject(g_DiskDriverObject);
        g_DiskDriverObject = NULL;
        g_OriginalDiskControl = NULL;
        DbgPrint("[HWIDSpoofer] Restored disk.sys\n");
    }
    
    // 恢复partmgr驱动
    if (g_ScsiDriverObject && g_OriginalScsiControl)
    {
        InterlockedExchangePointer(
            (PVOID*)&g_ScsiDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL],
            g_OriginalScsiControl);
        ObDereferenceObject(g_ScsiDriverObject);
        g_ScsiDriverObject = NULL;
        g_OriginalScsiControl = NULL;
        DbgPrint("[HWIDSpoofer] Restored partmgr\n");
    }
}

