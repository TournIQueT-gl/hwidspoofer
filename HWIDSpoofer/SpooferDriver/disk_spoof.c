/*
 * 磁盘序列号伪装模块
 * 
 * 通过Hook存储驱动的IRP处理来拦截磁盘查询
 */

#include <ntddk.h>
#include <ntdddisk.h>
#include <scsi.h>

// 外部配置
extern BOOLEAN g_DiskSpoofEnabled;
extern CHAR g_DiskSerial[];

// 原始分发函数
static PDRIVER_DISPATCH g_OriginalDiskDispatch = NULL;

// Storage Query Property结构
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

#define IOCTL_STORAGE_QUERY_PROPERTY 0x2D1400

// IRP完成回调
NTSTATUS DiskIoCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);
    
    if (!g_DiskSpoofEnabled)
    {
        return STATUS_SUCCESS;
    }
    
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    
    if (stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_QUERY_PROPERTY)
    {
        if (NT_SUCCESS(Irp->IoStatus.Status) && Irp->AssociatedIrp.SystemBuffer)
        {
            PSTORAGE_DEVICE_DESCRIPTOR desc = 
                (PSTORAGE_DEVICE_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;
            
            // 修改序列号
            if (desc->SerialNumberOffset > 0 && desc->SerialNumberOffset < desc->Size)
            {
                PCHAR serial = (PCHAR)desc + desc->SerialNumberOffset;
                SIZE_T origLen = strlen(serial);
                SIZE_T newLen = strlen(g_DiskSerial);
                SIZE_T copyLen = min(origLen, newLen);
                
                if (copyLen > 0)
                {
                    RtlCopyMemory(serial, g_DiskSerial, copyLen);
                    DbgPrint("[HWIDSpoofer] Spoofed disk serial\n");
                }
            }
        }
    }
    
    return STATUS_SUCCESS;
}

// 初始化磁盘伪装
NTSTATUS InitializeDiskSpoof()
{
    DbgPrint("[HWIDSpoofer] Disk spoof initialized\n");
    return STATUS_SUCCESS;
}

// 清理磁盘伪装
VOID CleanupDiskSpoof()
{
    DbgPrint("[HWIDSpoofer] Disk spoof cleaned up\n");
}

