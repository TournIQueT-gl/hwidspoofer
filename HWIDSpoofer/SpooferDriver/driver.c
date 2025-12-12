/*
 * HWID Spoofer Kernel Driver
 * 
 * 注意：编译此驱动需要Windows Driver Kit (WDK)
 * 运行需要测试模式或有效的代码签名证书
 * 
 * 启用测试模式：
 *   bcdedit /set testsigning on
 *   重启电脑
 */

#include <ntddk.h>
#include <wdm.h>

// 设备名和符号链接
#define DEVICE_NAME     L"\\Device\\HWIDSpoofer"
#define SYMBOLIC_LINK   L"\\DosDevices\\HWIDSpoofer"

// IOCTL定义
#define IOCTL_BASE      0x8000
#define IOCTL_SET_CONFIG    CTL_CODE(IOCTL_BASE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_CONFIG    CTL_CODE(IOCTL_BASE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ENABLE        CTL_CODE(IOCTL_BASE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISABLE       CTL_CODE(IOCTL_BASE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_DISK      CTL_CODE(IOCTL_BASE, 0x810, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_SMBIOS    CTL_CODE(IOCTL_BASE, 0x811, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ENABLE_GLOBAL CTL_CODE(IOCTL_BASE, 0x820, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISABLE_GLOBAL CTL_CODE(IOCTL_BASE, 0x821, METHOD_BUFFERED, FILE_ANY_ACCESS)

// IRP Hook函数声明（来自irp_hook.c）
NTSTATUS InstallIrpHook(void);
VOID UninstallIrpHook(void);

// 最大字符串长度
#define MAX_SERIAL_LENGTH   64
#define MAX_NAME_LENGTH     128

// 驱动全局状态
typedef struct _DRIVER_GLOBALS {
    BOOLEAN Enabled;
    BOOLEAN DiskSpoofEnabled;
    BOOLEAN SmbiosSpoofEnabled;
    BOOLEAN GlobalModeEnabled;     // 全局拦截模式
    
    // 磁盘伪装数据
    CHAR DiskSerial[MAX_SERIAL_LENGTH];
    CHAR DiskProductId[MAX_SERIAL_LENGTH];
    CHAR DiskVendor[MAX_SERIAL_LENGTH];
    
    // SMBIOS伪装数据
    CHAR BiosSerial[MAX_SERIAL_LENGTH];
    CHAR BiosVendor[MAX_NAME_LENGTH];
    CHAR SystemUuid[40];
    CHAR BaseboardSerial[MAX_SERIAL_LENGTH];
    CHAR BaseboardProduct[MAX_NAME_LENGTH];
    
    // 网络
    UCHAR MacAddress[6];
    
} DRIVER_GLOBALS, *PDRIVER_GLOBALS;

// 导出全局变量供其他模块使用
DRIVER_GLOBALS g_Globals = { 0 };
static PDEVICE_OBJECT g_DeviceObject = NULL;

// 函数前向声明
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH DispatchCreateClose;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH DispatchIoControl;

// IRP完成辅助函数
NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS Status, ULONG_PTR Information)
{
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

// 创建/关闭处理
NTSTATUS DispatchCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    return CompleteIrp(Irp, STATUS_SUCCESS, 0);
}

// IOCTL处理
NTSTATUS DispatchIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG ioctl = stack->Parameters.DeviceIoControl.IoControlCode;
    PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG inLength = stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLength = stack->Parameters.DeviceIoControl.OutputBufferLength;
    
    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR information = 0;
    
    switch (ioctl)
    {
        case IOCTL_ENABLE:
            g_Globals.Enabled = TRUE;
            DbgPrint("[HWIDSpoofer] Spoofing enabled\n");
            break;
            
        case IOCTL_DISABLE:
            g_Globals.Enabled = FALSE;
            DbgPrint("[HWIDSpoofer] Spoofing disabled\n");
            break;
            
        case IOCTL_SET_DISK:
            if (buffer && inLength >= MAX_SERIAL_LENGTH)
            {
                RtlCopyMemory(g_Globals.DiskSerial, buffer, MAX_SERIAL_LENGTH - 1);
                g_Globals.DiskSerial[MAX_SERIAL_LENGTH - 1] = '\0';
                g_Globals.DiskSpoofEnabled = TRUE;
                DbgPrint("[HWIDSpoofer] Disk serial set: %s\n", g_Globals.DiskSerial);
            }
            else
            {
                status = STATUS_INVALID_PARAMETER;
            }
            break;
            
        case IOCTL_SET_SMBIOS:
            if (buffer && inLength >= MAX_SERIAL_LENGTH)
            {
                RtlCopyMemory(g_Globals.BiosSerial, buffer, MAX_SERIAL_LENGTH - 1);
                g_Globals.BiosSerial[MAX_SERIAL_LENGTH - 1] = '\0';
                g_Globals.SmbiosSpoofEnabled = TRUE;
                DbgPrint("[HWIDSpoofer] BIOS serial set: %s\n", g_Globals.BiosSerial);
            }
            else
            {
                status = STATUS_INVALID_PARAMETER;
            }
            break;
            
        case IOCTL_GET_CONFIG:
            if (buffer && outLength >= sizeof(DRIVER_GLOBALS))
            {
                RtlCopyMemory(buffer, &g_Globals, sizeof(DRIVER_GLOBALS));
                information = sizeof(DRIVER_GLOBALS);
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        
        case IOCTL_ENABLE_GLOBAL:
            // 启用全局拦截模式 - 影响所有进程
            if (!g_Globals.GlobalModeEnabled)
            {
                status = InstallIrpHook();
                if (NT_SUCCESS(status))
                {
                    g_Globals.GlobalModeEnabled = TRUE;
                    g_Globals.Enabled = TRUE;
                    DbgPrint("[HWIDSpoofer] Global interception mode ENABLED\n");
                }
                else
                {
                    DbgPrint("[HWIDSpoofer] Failed to enable global mode: 0x%X\n", status);
                }
            }
            break;
            
        case IOCTL_DISABLE_GLOBAL:
            // 禁用全局拦截模式
            if (g_Globals.GlobalModeEnabled)
            {
                UninstallIrpHook();
                g_Globals.GlobalModeEnabled = FALSE;
                DbgPrint("[HWIDSpoofer] Global interception mode DISABLED\n");
            }
            break;
            
        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }
    
    return CompleteIrp(Irp, status, information);
}

// 驱动卸载
VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING symLink;
    
    UNREFERENCED_PARAMETER(DriverObject);
    
    DbgPrint("[HWIDSpoofer] Driver unloading...\n");
    
    // 卸载IRP Hooks
    if (g_Globals.GlobalModeEnabled)
    {
        UninstallIrpHook();
        g_Globals.GlobalModeEnabled = FALSE;
    }
    
    // 删除符号链接
    RtlInitUnicodeString(&symLink, SYMBOLIC_LINK);
    IoDeleteSymbolicLink(&symLink);
    
    // 删除设备
    if (g_DeviceObject)
    {
        IoDeleteDevice(g_DeviceObject);
    }
    
    DbgPrint("[HWIDSpoofer] Driver unloaded\n");
}

// 驱动入口
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    
    NTSTATUS status;
    UNICODE_STRING deviceName, symLink;
    
    DbgPrint("[HWIDSpoofer] Driver loading...\n");
    
    // 初始化设备名
    RtlInitUnicodeString(&deviceName, DEVICE_NAME);
    RtlInitUnicodeString(&symLink, SYMBOLIC_LINK);
    
    // 创建设备对象
    status = IoCreateDevice(
        DriverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &g_DeviceObject
    );
    
    if (!NT_SUCCESS(status))
    {
        DbgPrint("[HWIDSpoofer] Failed to create device: 0x%X\n", status);
        return status;
    }
    
    // 创建符号链接
    status = IoCreateSymbolicLink(&symLink, &deviceName);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("[HWIDSpoofer] Failed to create symbolic link: 0x%X\n", status);
        IoDeleteDevice(g_DeviceObject);
        return status;
    }
    
    // 设置分发函数
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoControl;
    DriverObject->DriverUnload = DriverUnload;
    
    // 初始化全局状态
    RtlZeroMemory(&g_Globals, sizeof(g_Globals));
    
    DbgPrint("[HWIDSpoofer] Driver loaded successfully\n");
    
    return STATUS_SUCCESS;
}

