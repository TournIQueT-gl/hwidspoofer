#include "hooks.h"
#include "hook_manager.h"
#include "logger.h"
#include "../Common/config.h"
#include <ntddscsi.h>
#include <string>
#include <cstring>

namespace hwid {
namespace hooks {
namespace disk {

// 原始函数指针
DeviceIoControl_t Original_DeviceIoControl = nullptr;
GetVolumeInformationW_t Original_GetVolumeInformationW = nullptr;
GetVolumeInformationA_t Original_GetVolumeInformationA = nullptr;

// Storage Query Property结构
#pragma pack(push, 1)
typedef struct _STORAGE_PROPERTY_QUERY {
    DWORD PropertyId;
    DWORD QueryType;
    BYTE AdditionalParameters[1];
} STORAGE_PROPERTY_QUERY;

typedef struct _STORAGE_DEVICE_DESCRIPTOR {
    DWORD Version;
    DWORD Size;
    BYTE DeviceType;
    BYTE DeviceTypeModifier;
    BOOLEAN RemovableMedia;
    BOOLEAN CommandQueueing;
    DWORD VendorIdOffset;
    DWORD ProductIdOffset;
    DWORD ProductRevisionOffset;
    DWORD SerialNumberOffset;
    BYTE BusType;
    DWORD RawPropertiesLength;
    BYTE RawDeviceProperties[1];
} STORAGE_DEVICE_DESCRIPTOR;
#pragma pack(pop)

// 修改字符串（在缓冲区内直接修改）
void spoof_string_in_buffer(char* buffer, size_t buffer_size, size_t offset, 
                            const std::string& new_value) {
    if (offset == 0 || offset >= buffer_size) {
        return;
    }
    
    char* target = buffer + offset;
    size_t available = buffer_size - offset;
    size_t copy_len = std::min(new_value.length(), available - 1);
    
    memcpy(target, new_value.c_str(), copy_len);
    target[copy_len] = '\0';
}

// Hook: DeviceIoControl
BOOL WINAPI Hooked_DeviceIoControl(
    HANDLE hDevice, DWORD dwIoControlCode,
    LPVOID lpInBuffer, DWORD nInBufferSize,
    LPVOID lpOutBuffer, DWORD nOutBufferSize,
    LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
{
    // 调用原始函数
    BOOL result = Original_DeviceIoControl(
        hDevice, dwIoControlCode, lpInBuffer, nInBufferSize,
        lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
    
    if (!result || !lpOutBuffer) {
        return result;
    }
    
    // 检查是否启用磁盘伪装
    if (!Config::instance().get_hardware_config().spoof_disk) {
        return result;
    }
    
    auto& disk_info = Config::instance().get_disk_info(0);
    
    // IOCTL_STORAGE_QUERY_PROPERTY - 最常用的磁盘信息查询
    if (dwIoControlCode == IOCTL_STORAGE_QUERY_PROPERTY) {
        STORAGE_DEVICE_DESCRIPTOR* desc = (STORAGE_DEVICE_DESCRIPTOR*)lpOutBuffer;
        
        if (desc->Size >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
            // 伪装序列号
            if (desc->SerialNumberOffset && !disk_info.serial_number.empty()) {
                spoof_string_in_buffer((char*)lpOutBuffer, nOutBufferSize,
                                      desc->SerialNumberOffset, disk_info.serial_number);
                LOG_DEBUG("[Disk] Spoofed Serial: %s", disk_info.serial_number.c_str());
            }
            
            // 伪装产品ID
            if (desc->ProductIdOffset && !disk_info.product_id.empty()) {
                spoof_string_in_buffer((char*)lpOutBuffer, nOutBufferSize,
                                      desc->ProductIdOffset, disk_info.product_id);
                LOG_DEBUG("[Disk] Spoofed ProductId: %s", disk_info.product_id.c_str());
            }
            
            // 伪装厂商ID
            if (desc->VendorIdOffset && !disk_info.vendor_id.empty()) {
                spoof_string_in_buffer((char*)lpOutBuffer, nOutBufferSize,
                                      desc->VendorIdOffset, disk_info.vendor_id);
                LOG_DEBUG("[Disk] Spoofed VendorId: %s", disk_info.vendor_id.c_str());
            }
            
            // 伪装固件版本
            if (desc->ProductRevisionOffset && !disk_info.revision.empty()) {
                spoof_string_in_buffer((char*)lpOutBuffer, nOutBufferSize,
                                      desc->ProductRevisionOffset, disk_info.revision);
                LOG_DEBUG("[Disk] Spoofed Revision: %s", disk_info.revision.c_str());
            }
        }
    }
    // SMART数据查询
    else if (dwIoControlCode == IOCTL_SMART_RCV_DRIVE_DATA) {
        // SMART数据包含硬盘序列号，可以在这里伪装
        // 具体结构取决于SMART命令
        LOG_DEBUG("[Disk] SMART query intercepted");
    }
    // ATA Pass-Through
    else if (dwIoControlCode == IOCTL_ATA_PASS_THROUGH) {
        // ATA IDENTIFY命令也包含序列号
        LOG_DEBUG("[Disk] ATA Pass-Through intercepted");
    }
    // SCSI Pass-Through
    else if (dwIoControlCode == IOCTL_SCSI_PASS_THROUGH) {
        LOG_DEBUG("[Disk] SCSI Pass-Through intercepted");
    }
    
    return result;
}

// Hook: GetVolumeInformationW
BOOL WINAPI Hooked_GetVolumeInformationW(
    LPCWSTR lpRootPathName, LPWSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags,
    LPWSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize)
{
    // 调用原始函数
    BOOL result = Original_GetVolumeInformationW(
        lpRootPathName, lpVolumeNameBuffer, nVolumeNameSize,
        lpVolumeSerialNumber, lpMaximumComponentLength, lpFileSystemFlags,
        lpFileSystemNameBuffer, nFileSystemNameSize);
    
    if (!result) {
        return result;
    }
    
    // 检查是否启用磁盘伪装
    if (!Config::instance().get_hardware_config().spoof_disk) {
        return result;
    }
    
    // 伪装卷序列号
    if (lpVolumeSerialNumber) {
        auto& disk_info = Config::instance().get_disk_info(0);
        if (disk_info.volume_serial != 0) {
            DWORD original = *lpVolumeSerialNumber;
            *lpVolumeSerialNumber = disk_info.volume_serial;
            LOG_DEBUG("[Disk] Spoofed Volume Serial: %08X -> %08X", 
                     original, disk_info.volume_serial);
        }
    }
    
    return result;
}

// Hook: GetVolumeInformationA
BOOL WINAPI Hooked_GetVolumeInformationA(
    LPCSTR lpRootPathName, LPSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags,
    LPSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize)
{
    // 调用原始函数
    BOOL result = Original_GetVolumeInformationA(
        lpRootPathName, lpVolumeNameBuffer, nVolumeNameSize,
        lpVolumeSerialNumber, lpMaximumComponentLength, lpFileSystemFlags,
        lpFileSystemNameBuffer, nFileSystemNameSize);
    
    if (!result) {
        return result;
    }
    
    // 检查是否启用磁盘伪装
    if (!Config::instance().get_hardware_config().spoof_disk) {
        return result;
    }
    
    // 伪装卷序列号
    if (lpVolumeSerialNumber) {
        auto& disk_info = Config::instance().get_disk_info(0);
        if (disk_info.volume_serial != 0) {
            DWORD original = *lpVolumeSerialNumber;
            *lpVolumeSerialNumber = disk_info.volume_serial;
            LOG_DEBUG("[Disk] Spoofed Volume Serial: %08X -> %08X",
                     original, disk_info.volume_serial);
        }
    }
    
    return result;
}

bool install() {
    LOG_INFO("Installing Disk hooks...");
    
    bool success = true;
    
    // DeviceIoControl
    if (!HookManager::instance().create_hook_api(
            "DeviceIoControl", "kernel32.dll", "DeviceIoControl",
            (void*)&Hooked_DeviceIoControl, (void**)&Original_DeviceIoControl)) {
        LOG_ERROR("Failed to hook DeviceIoControl");
        success = false;
    }
    
    // GetVolumeInformationW
    if (!HookManager::instance().create_hook_api(
            "GetVolumeInformationW", "kernel32.dll", "GetVolumeInformationW",
            (void*)&Hooked_GetVolumeInformationW, (void**)&Original_GetVolumeInformationW)) {
        LOG_ERROR("Failed to hook GetVolumeInformationW");
        success = false;
    }
    
    // GetVolumeInformationA
    if (!HookManager::instance().create_hook_api(
            "GetVolumeInformationA", "kernel32.dll", "GetVolumeInformationA",
            (void*)&Hooked_GetVolumeInformationA, (void**)&Original_GetVolumeInformationA)) {
        LOG_ERROR("Failed to hook GetVolumeInformationA");
        success = false;
    }
    
    LOG_INFO("Disk hooks installed: %s", success ? "OK" : "PARTIAL");
    return success;
}

void uninstall() {
    LOG_INFO("Uninstalling Disk hooks...");
    
    HookManager::instance().remove_hook("DeviceIoControl");
    HookManager::instance().remove_hook("GetVolumeInformationW");
    HookManager::instance().remove_hook("GetVolumeInformationA");
}

} // namespace disk
} // namespace hooks
} // namespace hwid

