#include "hooks.h"
#include "hook_manager.h"
#include "logger.h"

namespace hwid {
namespace hooks {

// 定义所有原始函数指针
RegQueryValueExW_t Original_RegQueryValueExW = nullptr;
RegQueryValueExA_t Original_RegQueryValueExA = nullptr;
RegGetValueW_t Original_RegGetValueW = nullptr;
RegGetValueA_t Original_RegGetValueA = nullptr;

DeviceIoControl_t Original_DeviceIoControl = nullptr;
GetVolumeInformationW_t Original_GetVolumeInformationW = nullptr;
GetVolumeInformationA_t Original_GetVolumeInformationA = nullptr;

GetAdaptersInfo_t Original_GetAdaptersInfo = nullptr;
GetAdaptersAddresses_t Original_GetAdaptersAddresses = nullptr;

GetSystemFirmwareTable_t Original_GetSystemFirmwareTable = nullptr;
EnumSystemFirmwareTables_t Original_EnumSystemFirmwareTables = nullptr;

IWbemClassObject_Get_t Original_IWbemClassObject_Get = nullptr;

SetupDiGetDeviceRegistryPropertyW_t Original_SetupDiGetDeviceRegistryPropertyW = nullptr;

bool install_all_hooks() {
    LOG_INFO("========================================");
    LOG_INFO("Installing all hooks...");
    LOG_INFO("========================================");
    
    bool success = true;
    int installed = 0;
    int failed = 0;
    
    // 1. 注册表Hook
    LOG_INFO("[1/6] Registry hooks...");
    if (registry::install()) {
        installed++;
    } else {
        failed++;
        success = false;
    }
    
    // 2. 磁盘Hook
    LOG_INFO("[2/6] Disk hooks...");
    if (disk::install()) {
        installed++;
    } else {
        failed++;
        success = false;
    }
    
    // 3. 网络Hook
    LOG_INFO("[3/6] Network hooks...");
    if (network::install()) {
        installed++;
    } else {
        failed++;
        success = false;
    }
    
    // 4. 固件/SMBIOS Hook
    LOG_INFO("[4/6] Firmware hooks...");
    if (firmware::install()) {
        installed++;
    } else {
        failed++;
        success = false;
    }
    
    // 5. SetupAPI Hook
    LOG_INFO("[5/6] SetupAPI hooks...");
    if (setupapi::install()) {
        installed++;
    } else {
        failed++;
        success = false;
    }
    
    // 6. WMI Hook（放最后，因为需要COM初始化）
    LOG_INFO("[6/6] WMI hooks...");
    if (wmi::install()) {
        installed++;
    } else {
        failed++;
        // WMI Hook失败不影响整体
        LOG_WARNING("WMI hook failed, but continuing...");
    }
    
    LOG_INFO("========================================");
    LOG_INFO("Hook installation complete:");
    LOG_INFO("  Installed: %d", installed);
    LOG_INFO("  Failed: %d", failed);
    LOG_INFO("========================================");
    
    return success;
}

void uninstall_all_hooks() {
    LOG_INFO("========================================");
    LOG_INFO("Uninstalling all hooks...");
    LOG_INFO("========================================");
    
    // 按相反顺序卸载
    wmi::uninstall();
    setupapi::uninstall();
    firmware::uninstall();
    network::uninstall();
    disk::uninstall();
    registry::uninstall();
    
    LOG_INFO("All hooks uninstalled");
    LOG_INFO("========================================");
}

} // namespace hooks
} // namespace hwid

