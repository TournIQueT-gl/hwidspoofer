#include "hooks.h"
#include "hook_manager.h"
#include "logger.h"
#include "../Common/config.h"
#include <string>
#include <algorithm>

namespace hwid {
namespace hooks {
namespace registry {

// 原始函数指针
RegQueryValueExW_t Original_RegQueryValueExW = nullptr;
RegQueryValueExA_t Original_RegQueryValueExA = nullptr;
RegGetValueW_t Original_RegGetValueW = nullptr;
RegGetValueA_t Original_RegGetValueA = nullptr;

// 检查是否是需要伪装的注册表值
bool should_spoof_value(const std::wstring& key_path, const std::wstring& value_name) {
    // MachineGuid
    if (value_name == L"MachineGuid") {
        return Config::instance().get_hardware_config().spoof_system;
    }
    
    // ProductId
    if (value_name == L"ProductId" || value_name == L"ProductID") {
        return Config::instance().get_hardware_config().spoof_system;
    }
    
    // BIOS相关
    if (value_name == L"SystemBiosVersion" || 
        value_name == L"VideoBiosVersion" ||
        value_name == L"BaseBoardManufacturer" ||
        value_name == L"BaseBoardProduct" ||
        value_name == L"SystemManufacturer" ||
        value_name == L"SystemProductName" ||
        value_name == L"BIOSVendor" ||
        value_name == L"BIOSVersion" ||
        value_name == L"BIOSReleaseDate") {
        return Config::instance().get_hardware_config().spoof_bios;
    }
    
    // 计算机名
    if (value_name == L"ComputerName") {
        return Config::instance().get_hardware_config().spoof_system;
    }
    
    // HwProfileGuid
    if (value_name == L"HwProfileGuid") {
        return Config::instance().get_hardware_config().spoof_system;
    }
    
    return false;
}

// 获取伪装后的值
std::wstring get_spoofed_value(const std::wstring& value_name) {
    auto& config = Config::instance();
    
    if (value_name == L"MachineGuid") {
        return std::wstring(config.get_spoofed_value_wide(HwidType::MACHINE_GUID));
    }
    
    if (value_name == L"ProductId" || value_name == L"ProductID") {
        return std::wstring(config.get_spoofed_value_wide(HwidType::PRODUCT_ID));
    }
    
    if (value_name == L"BaseBoardManufacturer") {
        return std::wstring(config.get_hardware_config().baseboard.manufacturer.begin(),
                           config.get_hardware_config().baseboard.manufacturer.end());
    }
    
    if (value_name == L"BaseBoardProduct") {
        return std::wstring(config.get_hardware_config().baseboard.product.begin(),
                           config.get_hardware_config().baseboard.product.end());
    }
    
    if (value_name == L"SystemManufacturer") {
        return std::wstring(config.get_hardware_config().bios.manufacturer.begin(),
                           config.get_hardware_config().bios.manufacturer.end());
    }
    
    if (value_name == L"BIOSVendor") {
        return std::wstring(config.get_hardware_config().bios.manufacturer.begin(),
                           config.get_hardware_config().bios.manufacturer.end());
    }
    
    if (value_name == L"BIOSVersion") {
        return std::wstring(config.get_hardware_config().bios.version.begin(),
                           config.get_hardware_config().bios.version.end());
    }
    
    if (value_name == L"BIOSReleaseDate") {
        return std::wstring(config.get_hardware_config().bios.release_date.begin(),
                           config.get_hardware_config().bios.release_date.end());
    }
    
    if (value_name == L"ComputerName") {
        return std::wstring(config.get_hardware_config().system.computer_name.begin(),
                           config.get_hardware_config().system.computer_name.end());
    }
    
    if (value_name == L"HwProfileGuid") {
        return config.get_spoofed_value_wide(HwidType::MACHINE_GUID);
    }
    
    return L"";
}

// Hook: RegQueryValueExW
LSTATUS WINAPI Hooked_RegQueryValueExW(
    HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved,
    LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) 
{
    // 调用原始函数
    LSTATUS result = Original_RegQueryValueExW(hKey, lpValueName, lpReserved, 
                                                lpType, lpData, lpcbData);
    
    if (result != ERROR_SUCCESS || lpValueName == nullptr || lpData == nullptr) {
        return result;
    }
    
    std::wstring value_name(lpValueName);
    
    // 检查是否需要伪装
    if (should_spoof_value(L"", value_name)) {
        std::wstring spoofed = get_spoofed_value(value_name);
        
        if (!spoofed.empty()) {
            size_t required_size = (spoofed.length() + 1) * sizeof(wchar_t);
            
            if (lpcbData && *lpcbData >= required_size) {
                wcscpy_s((wchar_t*)lpData, spoofed.length() + 1, spoofed.c_str());
                *lpcbData = static_cast<DWORD>(required_size);
                
                LOG_DEBUG("[Registry] Spoofed %ls -> %ls", lpValueName, spoofed.c_str());
            }
        }
    }
    
    return result;
}

// Hook: RegQueryValueExA
LSTATUS WINAPI Hooked_RegQueryValueExA(
    HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved,
    LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    // 调用原始函数
    LSTATUS result = Original_RegQueryValueExA(hKey, lpValueName, lpReserved,
                                                lpType, lpData, lpcbData);
    
    if (result != ERROR_SUCCESS || lpValueName == nullptr || lpData == nullptr) {
        return result;
    }
    
    // 转换为宽字符
    std::string value_name_a(lpValueName);
    std::wstring value_name(value_name_a.begin(), value_name_a.end());
    
    // 检查是否需要伪装
    if (should_spoof_value(L"", value_name)) {
        std::wstring spoofed_w = get_spoofed_value(value_name);
        std::string spoofed(spoofed_w.begin(), spoofed_w.end());
        
        if (!spoofed.empty()) {
            size_t required_size = spoofed.length() + 1;
            
            if (lpcbData && *lpcbData >= required_size) {
                strcpy_s((char*)lpData, spoofed.length() + 1, spoofed.c_str());
                *lpcbData = static_cast<DWORD>(required_size);
                
                LOG_DEBUG("[Registry] Spoofed %s -> %s", lpValueName, spoofed.c_str());
            }
        }
    }
    
    return result;
}

// Hook: RegGetValueW
LSTATUS WINAPI Hooked_RegGetValueW(
    HKEY hkey, LPCWSTR lpSubKey, LPCWSTR lpValue,
    DWORD dwFlags, LPDWORD pdwType, PVOID pvData, LPDWORD pcbData)
{
    // 调用原始函数
    LSTATUS result = Original_RegGetValueW(hkey, lpSubKey, lpValue,
                                           dwFlags, pdwType, pvData, pcbData);
    
    if (result != ERROR_SUCCESS || lpValue == nullptr || pvData == nullptr) {
        return result;
    }
    
    std::wstring value_name(lpValue);
    std::wstring sub_key(lpSubKey ? lpSubKey : L"");
    
    // 检查是否需要伪装
    if (should_spoof_value(sub_key, value_name)) {
        std::wstring spoofed = get_spoofed_value(value_name);
        
        if (!spoofed.empty()) {
            size_t required_size = (spoofed.length() + 1) * sizeof(wchar_t);
            
            if (pcbData && *pcbData >= required_size) {
                wcscpy_s((wchar_t*)pvData, spoofed.length() + 1, spoofed.c_str());
                *pcbData = static_cast<DWORD>(required_size);
                
                LOG_DEBUG("[Registry] Spoofed %ls\\%ls -> %ls", 
                         lpSubKey ? lpSubKey : L"", lpValue, spoofed.c_str());
            }
        }
    }
    
    return result;
}

// Hook: RegGetValueA
LSTATUS WINAPI Hooked_RegGetValueA(
    HKEY hkey, LPCSTR lpSubKey, LPCSTR lpValue,
    DWORD dwFlags, LPDWORD pdwType, PVOID pvData, LPDWORD pcbData)
{
    // 调用原始函数
    LSTATUS result = Original_RegGetValueA(hkey, lpSubKey, lpValue,
                                           dwFlags, pdwType, pvData, pcbData);
    
    if (result != ERROR_SUCCESS || lpValue == nullptr || pvData == nullptr) {
        return result;
    }
    
    std::string value_name_a(lpValue);
    std::wstring value_name(value_name_a.begin(), value_name_a.end());
    std::wstring sub_key;
    if (lpSubKey) {
        std::string sub_key_a(lpSubKey);
        sub_key = std::wstring(sub_key_a.begin(), sub_key_a.end());
    }
    
    // 检查是否需要伪装
    if (should_spoof_value(sub_key, value_name)) {
        std::wstring spoofed_w = get_spoofed_value(value_name);
        std::string spoofed(spoofed_w.begin(), spoofed_w.end());
        
        if (!spoofed.empty()) {
            size_t required_size = spoofed.length() + 1;
            
            if (pcbData && *pcbData >= required_size) {
                strcpy_s((char*)pvData, spoofed.length() + 1, spoofed.c_str());
                *pcbData = static_cast<DWORD>(required_size);
                
                LOG_DEBUG("[Registry] Spoofed %s\\%s -> %s",
                         lpSubKey ? lpSubKey : "", lpValue, spoofed.c_str());
            }
        }
    }
    
    return result;
}

bool install() {
    LOG_INFO("Installing Registry hooks...");
    
    bool success = true;
    
    // RegQueryValueExW
    if (!HookManager::instance().create_hook_api(
            "RegQueryValueExW", "advapi32.dll", "RegQueryValueExW",
            (void*)&Hooked_RegQueryValueExW, (void**)&Original_RegQueryValueExW)) {
        LOG_ERROR("Failed to hook RegQueryValueExW");
        success = false;
    }
    
    // RegQueryValueExA
    if (!HookManager::instance().create_hook_api(
            "RegQueryValueExA", "advapi32.dll", "RegQueryValueExA",
            (void*)&Hooked_RegQueryValueExA, (void**)&Original_RegQueryValueExA)) {
        LOG_ERROR("Failed to hook RegQueryValueExA");
        success = false;
    }
    
    // RegGetValueW
    if (!HookManager::instance().create_hook_api(
            "RegGetValueW", "advapi32.dll", "RegGetValueW",
            (void*)&Hooked_RegGetValueW, (void**)&Original_RegGetValueW)) {
        LOG_ERROR("Failed to hook RegGetValueW");
        success = false;
    }
    
    // RegGetValueA
    if (!HookManager::instance().create_hook_api(
            "RegGetValueA", "advapi32.dll", "RegGetValueA",
            (void*)&Hooked_RegGetValueA, (void**)&Original_RegGetValueA)) {
        LOG_ERROR("Failed to hook RegGetValueA");
        success = false;
    }
    
    LOG_INFO("Registry hooks installed: %s", success ? "OK" : "PARTIAL");
    return success;
}

void uninstall() {
    LOG_INFO("Uninstalling Registry hooks...");
    
    HookManager::instance().remove_hook("RegQueryValueExW");
    HookManager::instance().remove_hook("RegQueryValueExA");
    HookManager::instance().remove_hook("RegGetValueW");
    HookManager::instance().remove_hook("RegGetValueA");
}

} // namespace registry
} // namespace hooks
} // namespace hwid

