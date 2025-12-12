#include "hooks.h"
#include "hook_manager.h"
#include "logger.h"
#include "../Common/config.h"
#include <iphlpapi.h>
#include <string>

#pragma comment(lib, "iphlpapi.lib")

namespace hwid {
namespace hooks {
namespace network {

// 原始函数指针
GetAdaptersInfo_t Original_GetAdaptersInfo = nullptr;
GetAdaptersAddresses_t Original_GetAdaptersAddresses = nullptr;

// 伪装MAC地址
void spoof_mac_address(BYTE* mac, size_t adapter_index = 0) {
    if (!Config::instance().get_hardware_config().spoof_network) {
        return;
    }
    
    auto& network_info = Config::instance().get_network_adapter(adapter_index);
    
    // 复制伪装的MAC地址
    memcpy(mac, network_info.mac_address, 6);
    
    LOG_DEBUG("[Network] Spoofed MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// 检查是否是物理网卡（排除虚拟网卡）
bool is_physical_adapter(const char* description) {
    if (!description) return true;
    
    std::string desc(description);
    
    // 常见虚拟网卡关键词
    const char* virtual_keywords[] = {
        "Virtual", "VMware", "VirtualBox", "Hyper-V", "TAP-",
        "VPN", "Tunnel", "Loopback", "Microsoft Wi-Fi Direct",
        "Bluetooth", "WAN Miniport"
    };
    
    for (const auto& keyword : virtual_keywords) {
        if (desc.find(keyword) != std::string::npos) {
            return false;
        }
    }
    
    return true;
}

// Hook: GetAdaptersInfo
DWORD WINAPI Hooked_GetAdaptersInfo(PIP_ADAPTER_INFO AdapterInfo, PULONG SizePointer) {
    // 调用原始函数
    DWORD result = Original_GetAdaptersInfo(AdapterInfo, SizePointer);
    
    if (result != ERROR_SUCCESS || AdapterInfo == nullptr) {
        return result;
    }
    
    // 检查是否启用网络伪装
    if (!Config::instance().get_hardware_config().spoof_network) {
        return result;
    }
    
    // 遍历所有适配器
    PIP_ADAPTER_INFO current = AdapterInfo;
    size_t adapter_index = 0;
    
    while (current) {
        // 只伪装物理网卡
        if (is_physical_adapter(current->Description)) {
            auto& network_info = Config::instance().get_network_adapter(adapter_index);
            
            // 伪装MAC地址
            if (current->AddressLength == 6) {
                LOG_DEBUG("[Network] Original MAC[%zu]: %02X:%02X:%02X:%02X:%02X:%02X",
                         adapter_index,
                         current->Address[0], current->Address[1], current->Address[2],
                         current->Address[3], current->Address[4], current->Address[5]);
                
                memcpy(current->Address, network_info.mac_address, 6);
                
                LOG_DEBUG("[Network] Spoofed MAC[%zu]: %02X:%02X:%02X:%02X:%02X:%02X",
                         adapter_index,
                         current->Address[0], current->Address[1], current->Address[2],
                         current->Address[3], current->Address[4], current->Address[5]);
            }
            
            adapter_index++;
        }
        
        current = current->Next;
    }
    
    return result;
}

// Hook: GetAdaptersAddresses
ULONG WINAPI Hooked_GetAdaptersAddresses(
    ULONG Family, ULONG Flags, PVOID Reserved,
    PIP_ADAPTER_ADDRESSES AdapterAddresses, PULONG SizePointer)
{
    // 调用原始函数
    ULONG result = Original_GetAdaptersAddresses(
        Family, Flags, Reserved, AdapterAddresses, SizePointer);
    
    if (result != ERROR_SUCCESS || AdapterAddresses == nullptr) {
        return result;
    }
    
    // 检查是否启用网络伪装
    if (!Config::instance().get_hardware_config().spoof_network) {
        return result;
    }
    
    // 遍历所有适配器
    PIP_ADAPTER_ADDRESSES current = AdapterAddresses;
    size_t adapter_index = 0;
    
    while (current) {
        // 只伪装物理网卡（Ethernet或WiFi）
        if (current->IfType == IF_TYPE_ETHERNET_CSMACD || 
            current->IfType == IF_TYPE_IEEE80211) {
            
            // 检查是否是物理网卡
            char desc[256] = {0};
            WideCharToMultiByte(CP_UTF8, 0, current->Description, -1, 
                               desc, sizeof(desc), nullptr, nullptr);
            
            if (is_physical_adapter(desc)) {
                auto& network_info = Config::instance().get_network_adapter(adapter_index);
                
                // 伪装物理地址
                if (current->PhysicalAddressLength == 6) {
                    LOG_DEBUG("[Network] Original MAC[%zu]: %02X:%02X:%02X:%02X:%02X:%02X (%ls)",
                             adapter_index,
                             current->PhysicalAddress[0], current->PhysicalAddress[1], 
                             current->PhysicalAddress[2], current->PhysicalAddress[3], 
                             current->PhysicalAddress[4], current->PhysicalAddress[5],
                             current->Description);
                    
                    memcpy(current->PhysicalAddress, network_info.mac_address, 6);
                    
                    LOG_DEBUG("[Network] Spoofed MAC[%zu]: %02X:%02X:%02X:%02X:%02X:%02X",
                             adapter_index,
                             current->PhysicalAddress[0], current->PhysicalAddress[1],
                             current->PhysicalAddress[2], current->PhysicalAddress[3],
                             current->PhysicalAddress[4], current->PhysicalAddress[5]);
                }
                
                adapter_index++;
            }
        }
        
        current = current->Next;
    }
    
    return result;
}

bool install() {
    LOG_INFO("Installing Network hooks...");
    
    bool success = true;
    
    // GetAdaptersInfo
    if (!HookManager::instance().create_hook_api(
            "GetAdaptersInfo", "iphlpapi.dll", "GetAdaptersInfo",
            (void*)&Hooked_GetAdaptersInfo, (void**)&Original_GetAdaptersInfo)) {
        LOG_ERROR("Failed to hook GetAdaptersInfo");
        success = false;
    }
    
    // GetAdaptersAddresses
    if (!HookManager::instance().create_hook_api(
            "GetAdaptersAddresses", "iphlpapi.dll", "GetAdaptersAddresses",
            (void*)&Hooked_GetAdaptersAddresses, (void**)&Original_GetAdaptersAddresses)) {
        LOG_ERROR("Failed to hook GetAdaptersAddresses");
        success = false;
    }
    
    LOG_INFO("Network hooks installed: %s", success ? "OK" : "PARTIAL");
    return success;
}

void uninstall() {
    LOG_INFO("Uninstalling Network hooks...");
    
    HookManager::instance().remove_hook("GetAdaptersInfo");
    HookManager::instance().remove_hook("GetAdaptersAddresses");
}

} // namespace network
} // namespace hooks
} // namespace hwid

