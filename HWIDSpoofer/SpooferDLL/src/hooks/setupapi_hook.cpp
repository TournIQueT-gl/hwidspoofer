#include "hooks.h"
#include "hook_manager.h"
#include "logger.h"
#include "../Common/config.h"
#include <SetupAPI.h>
#include <devguid.h>
#include <string>
#include <algorithm>

#pragma comment(lib, "setupapi.lib")

namespace hwid {
namespace hooks {
namespace setupapi {

// 原始函数指针
SetupDiGetDeviceRegistryPropertyW_t Original_SetupDiGetDeviceRegistryPropertyW = nullptr;

// 设备属性类型
enum DeviceProperty {
    SPDRP_DEVICEDESC = 0x00000000,
    SPDRP_HARDWAREID = 0x00000001,
    SPDRP_COMPATIBLEIDS = 0x00000002,
    SPDRP_SERVICE = 0x00000004,
    SPDRP_CLASS = 0x00000007,
    SPDRP_CLASSGUID = 0x00000008,
    SPDRP_DRIVER = 0x00000009,
    SPDRP_MFG = 0x0000000B,
    SPDRP_FRIENDLYNAME = 0x0000000C,
    SPDRP_PHYSICAL_DEVICE_OBJECT_NAME = 0x0000000E,
};

// 检查是否是磁盘设备的硬件ID
bool is_disk_hardware_id(const std::wstring& hw_id) {
    // 常见磁盘硬件ID模式
    return hw_id.find(L"DISK&") != std::wstring::npos ||
           hw_id.find(L"SCSI\\Disk") != std::wstring::npos ||
           hw_id.find(L"IDE\\Disk") != std::wstring::npos ||
           hw_id.find(L"NVME\\") != std::wstring::npos ||
           hw_id.find(L"USBSTOR\\") != std::wstring::npos;
}

// 检查是否是网卡设备
bool is_network_hardware_id(const std::wstring& hw_id) {
    return hw_id.find(L"PCI\\VEN_") != std::wstring::npos &&
           (hw_id.find(L"&DEV_") != std::wstring::npos);
}

// 修改硬件ID中的序列号
std::wstring spoof_hardware_id(const std::wstring& original) {
    std::wstring result = original;
    auto& config = Config::instance().get_hardware_config();
    
    // 查找并替换序列号部分
    // 格式: XXXX&Rev_XXXX&SERIAL
    size_t serial_pos = result.find(L"&");
    if (serial_pos != std::wstring::npos) {
        // 查找最后一个&后面的部分（通常是序列号）
        size_t last_amp = result.rfind(L"&");
        if (last_amp != std::wstring::npos && last_amp > 0) {
            // 检查是否有下划线分隔（如Rev_、Ven_等）
            std::wstring suffix = result.substr(last_amp + 1);
            if (suffix.find(L"_") == std::wstring::npos && suffix.length() > 4) {
                // 这可能是序列号，替换它
                if (!config.disks.empty()) {
                    std::string& serial = config.disks[0].serial_number;
                    std::wstring w_serial(serial.begin(), serial.end());
                    result = result.substr(0, last_amp + 1) + w_serial;
                }
            }
        }
    }
    
    return result;
}

// Hook: SetupDiGetDeviceRegistryPropertyW
BOOL WINAPI Hooked_SetupDiGetDeviceRegistryPropertyW(
    HDEVINFO DeviceInfoSet,
    PSP_DEVINFO_DATA DeviceInfoData,
    DWORD Property,
    PDWORD PropertyRegDataType,
    PBYTE PropertyBuffer,
    DWORD PropertyBufferSize,
    PDWORD RequiredSize)
{
    // 调用原始函数
    BOOL result = Original_SetupDiGetDeviceRegistryPropertyW(
        DeviceInfoSet, DeviceInfoData, Property,
        PropertyRegDataType, PropertyBuffer, PropertyBufferSize, RequiredSize);
    
    if (!result || PropertyBuffer == nullptr) {
        return result;
    }
    
    auto& config = Config::instance().get_hardware_config();
    
    // 处理硬件ID
    if (Property == SPDRP_HARDWAREID && config.spoof_disk) {
        wchar_t* hw_id = (wchar_t*)PropertyBuffer;
        std::wstring original(hw_id);
        
        if (is_disk_hardware_id(original)) {
            std::wstring spoofed = spoof_hardware_id(original);
            
            if (spoofed != original && spoofed.length() < PropertyBufferSize / sizeof(wchar_t)) {
                wcscpy_s((wchar_t*)PropertyBuffer, PropertyBufferSize / sizeof(wchar_t), 
                        spoofed.c_str());
                LOG_DEBUG("[SetupAPI] Spoofed HardwareID");
            }
        }
    }
    
    // 处理设备描述
    if (Property == SPDRP_DEVICEDESC) {
        // 可以修改设备描述
    }
    
    // 处理制造商
    if (Property == SPDRP_MFG && config.spoof_baseboard) {
        wchar_t* mfg = (wchar_t*)PropertyBuffer;
        std::wstring w_mfg(config.baseboard.manufacturer.begin(), 
                          config.baseboard.manufacturer.end());
        
        if (!w_mfg.empty() && w_mfg.length() < PropertyBufferSize / sizeof(wchar_t)) {
            wcscpy_s(mfg, PropertyBufferSize / sizeof(wchar_t), w_mfg.c_str());
            LOG_DEBUG("[SetupAPI] Spoofed Manufacturer: %ls", w_mfg.c_str());
        }
    }
    
    return result;
}

bool install() {
    LOG_INFO("Installing SetupAPI hooks...");
    
    bool success = true;
    
    // SetupDiGetDeviceRegistryPropertyW
    if (!HookManager::instance().create_hook_api(
            "SetupDiGetDeviceRegistryPropertyW", "setupapi.dll", 
            "SetupDiGetDeviceRegistryPropertyW",
            (void*)&Hooked_SetupDiGetDeviceRegistryPropertyW, 
            (void**)&Original_SetupDiGetDeviceRegistryPropertyW)) {
        LOG_ERROR("Failed to hook SetupDiGetDeviceRegistryPropertyW");
        success = false;
    }
    
    LOG_INFO("SetupAPI hooks installed: %s", success ? "OK" : "PARTIAL");
    return success;
}

void uninstall() {
    LOG_INFO("Uninstalling SetupAPI hooks...");
    
    HookManager::instance().remove_hook("SetupDiGetDeviceRegistryPropertyW");
}

} // namespace setupapi
} // namespace hooks
} // namespace hwid

