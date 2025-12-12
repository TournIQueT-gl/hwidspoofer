#include "hooks.h"
#include "hook_manager.h"
#include "logger.h"
#include "../Common/config.h"
#include <comdef.h>
#include <Wbemidl.h>
#include <string>
#include <unordered_map>

#pragma comment(lib, "wbemuuid.lib")

namespace hwid {
namespace hooks {
namespace wmi {

// 原始函数指针
IWbemClassObject_Get_t Original_IWbemClassObject_Get = nullptr;

// VTable索引
constexpr int IWBEMCLASSOBJECT_GET_INDEX = 4;  // IWbemClassObject::Get的VTable索引

// 存储原始VTable
static void* g_original_vtable = nullptr;
static bool g_hooked = false;

// WMI属性名到伪装值的映射
struct WmiPropertySpoof {
    const wchar_t* class_hint;  // 类名提示（可选）
    HwidType hwid_type;          // 对应的硬件ID类型
};

// WMI属性映射表
static const std::unordered_map<std::wstring, WmiPropertySpoof> g_property_map = {
    // 磁盘相关
    {L"SerialNumber",       {L"Win32_DiskDrive", HwidType::DISK_SERIAL}},
    {L"PNPDeviceID",        {nullptr, HwidType::DISK_PRODUCT_ID}},
    
    // BIOS相关
    {L"SMBIOSBIOSVersion",  {L"Win32_BIOS", HwidType::BIOS_VERSION}},
    {L"Manufacturer",       {L"Win32_BIOS", HwidType::BIOS_MANUFACTURER}},
    {L"ReleaseDate",        {L"Win32_BIOS", HwidType::BIOS_RELEASE_DATE}},
    
    // 主板相关
    {L"Product",            {L"Win32_BaseBoard", HwidType::BASEBOARD_PRODUCT}},
    
    // CPU相关
    {L"ProcessorId",        {L"Win32_Processor", HwidType::CPU_ID}},
    
    // 系统相关
    {L"UUID",               {L"Win32_ComputerSystemProduct", HwidType::SYSTEM_UUID}},
    {L"IdentifyingNumber",  {nullptr, HwidType::BIOS_SERIAL}},
    
    // 网络相关
    {L"MACAddress",         {L"Win32_NetworkAdapter", HwidType::MAC_ADDRESS}},
    
    // 显卡相关
    {L"PNPDeviceID",        {L"Win32_VideoController", HwidType::GPU_SERIAL}},
};

// 获取伪装值
std::wstring get_wmi_spoofed_value(const std::wstring& property_name) {
    auto& config = Config::instance();
    auto& hw_config = config.get_hardware_config();
    
    // SerialNumber - 可能来自多个WMI类
    if (property_name == L"SerialNumber") {
        // 优先返回BIOS序列号（最常用）
        if (hw_config.spoof_bios && !hw_config.bios.serial_number.empty()) {
            return std::wstring(hw_config.bios.serial_number.begin(), 
                               hw_config.bios.serial_number.end());
        }
        // 或者磁盘序列号
        if (hw_config.spoof_disk && !hw_config.disks.empty()) {
            return std::wstring(hw_config.disks[0].serial_number.begin(),
                               hw_config.disks[0].serial_number.end());
        }
    }
    
    // ProcessorId
    if (property_name == L"ProcessorId") {
        if (hw_config.spoof_cpu) {
            return std::wstring(hw_config.cpu.processor_id.begin(),
                               hw_config.cpu.processor_id.end());
        }
    }
    
    // UUID
    if (property_name == L"UUID") {
        if (hw_config.spoof_system) {
            return std::wstring(hw_config.system.uuid.begin(),
                               hw_config.system.uuid.end());
        }
    }
    
    // IdentifyingNumber（系统序列号）
    if (property_name == L"IdentifyingNumber") {
        if (hw_config.spoof_bios) {
            return std::wstring(hw_config.bios.serial_number.begin(),
                               hw_config.bios.serial_number.end());
        }
    }
    
    // Product（主板产品名）
    if (property_name == L"Product") {
        if (hw_config.spoof_baseboard) {
            return std::wstring(hw_config.baseboard.product.begin(),
                               hw_config.baseboard.product.end());
        }
    }
    
    // Manufacturer
    if (property_name == L"Manufacturer") {
        if (hw_config.spoof_bios) {
            return std::wstring(hw_config.bios.manufacturer.begin(),
                               hw_config.bios.manufacturer.end());
        }
    }
    
    // MACAddress
    if (property_name == L"MACAddress") {
        if (hw_config.spoof_network && !hw_config.network_adapters.empty()) {
            return std::wstring(hw_config.network_adapters[0].mac_string.begin(),
                               hw_config.network_adapters[0].mac_string.end());
        }
    }
    
    // SMBIOSBIOSVersion
    if (property_name == L"SMBIOSBIOSVersion") {
        if (hw_config.spoof_bios) {
            return std::wstring(hw_config.bios.sm_bios_version.begin(),
                               hw_config.bios.sm_bios_version.end());
        }
    }
    
    // Caption（通常是产品名）
    if (property_name == L"Caption" || property_name == L"Name") {
        if (hw_config.spoof_baseboard) {
            return std::wstring(hw_config.baseboard.product.begin(),
                               hw_config.baseboard.product.end());
        }
    }
    
    // Version
    if (property_name == L"Version") {
        if (hw_config.spoof_baseboard) {
            return std::wstring(hw_config.baseboard.version.begin(),
                               hw_config.baseboard.version.end());
        }
    }
    
    return L"";
}

// 检查是否应该伪装该属性
bool should_spoof_wmi_property(const std::wstring& property_name) {
    // 常见需要伪装的属性
    static const std::vector<std::wstring> spoof_properties = {
        L"SerialNumber",
        L"ProcessorId",
        L"UUID",
        L"IdentifyingNumber",
        L"Product",
        L"Manufacturer",
        L"MACAddress",
        L"SMBIOSBIOSVersion",
        L"Version",
        L"Caption",
        L"Name",
        L"PNPDeviceID"
    };
    
    for (const auto& prop : spoof_properties) {
        if (_wcsicmp(property_name.c_str(), prop.c_str()) == 0) {
            return true;
        }
    }
    
    return false;
}

// Hook函数: IWbemClassObject::Get
HRESULT STDMETHODCALLTYPE Hooked_IWbemClassObject_Get(
    IWbemClassObject* This,
    LPCWSTR wszName,
    long lFlags,
    VARIANT* pVal,
    long* pType,
    long* plFlavor)
{
    // 调用原始函数
    HRESULT hr = Original_IWbemClassObject_Get(This, wszName, lFlags, pVal, pType, plFlavor);
    
    if (FAILED(hr) || wszName == nullptr || pVal == nullptr) {
        return hr;
    }
    
    // 检查是否启用WMI伪装
    if (!Config::instance().get_hardware_config().spoof_wmi) {
        return hr;
    }
    
    std::wstring property_name(wszName);
    
    // 检查是否需要伪装
    if (should_spoof_wmi_property(property_name)) {
        std::wstring spoofed = get_wmi_spoofed_value(property_name);
        
        if (!spoofed.empty() && pVal->vt == VT_BSTR && pVal->bstrVal != nullptr) {
            // 记录原始值
            LOG_DEBUG("[WMI] Intercepted %ls: %ls", wszName, pVal->bstrVal);
            
            // 释放原始字符串
            SysFreeString(pVal->bstrVal);
            
            // 设置伪装值
            pVal->bstrVal = SysAllocString(spoofed.c_str());
            
            LOG_DEBUG("[WMI] Spoofed %ls -> %ls", wszName, spoofed.c_str());
        }
    }
    
    return hr;
}

// 安装VTable Hook
bool install_vtable_hook() {
    // 创建WMI定位器
    IWbemLocator* pLoc = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);
    
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create WbemLocator: 0x%08X", hr);
        return false;
    }
    
    // 连接到WMI
    IWbemServices* pSvc = nullptr;
    hr = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr,
        0, nullptr, nullptr, &pSvc);
    
    if (FAILED(hr)) {
        LOG_ERROR("Failed to connect to WMI: 0x%08X", hr);
        pLoc->Release();
        return false;
    }
    
    // 执行查询获取一个IWbemClassObject实例
    IEnumWbemClassObject* pEnumerator = nullptr;
    hr = pSvc->ExecQuery(
        _bstr_t("WQL"),
        _bstr_t("SELECT * FROM Win32_BIOS"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr, &pEnumerator);
    
    if (FAILED(hr)) {
        LOG_ERROR("WMI query failed: 0x%08X", hr);
        pSvc->Release();
        pLoc->Release();
        return false;
    }
    
    // 获取第一个对象
    IWbemClassObject* pObj = nullptr;
    ULONG uReturn = 0;
    hr = pEnumerator->Next(WBEM_INFINITE, 1, &pObj, &uReturn);
    
    if (SUCCEEDED(hr) && uReturn > 0 && pObj) {
        // 获取VTable
        void** vtable = *(void***)pObj;
        
        // 获取Get方法的地址
        void* get_method = vtable[IWBEMCLASSOBJECT_GET_INDEX];
        
        // 使用MinHook创建Hook
        if (HookManager::instance().create_hook(
                "IWbemClassObject::Get", 
                get_method,
                (void*)&Hooked_IWbemClassObject_Get,
                (void**)&Original_IWbemClassObject_Get)) {
            g_hooked = true;
            LOG_INFO("WMI VTable hook installed successfully");
        }
        
        pObj->Release();
    }
    
    // 清理
    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    
    return g_hooked;
}

bool install() {
    LOG_INFO("Installing WMI hooks...");
    
    // 初始化COM（如果尚未初始化）
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool com_initialized = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
    
    if (!com_initialized) {
        LOG_ERROR("Failed to initialize COM: 0x%08X", hr);
        return false;
    }
    
    // 设置安全级别
    hr = CoInitializeSecurity(
        nullptr, -1, nullptr, nullptr,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_NONE, nullptr);
    
    // 安装VTable Hook
    bool success = install_vtable_hook();
    
    LOG_INFO("WMI hooks installed: %s", success ? "OK" : "FAILED");
    return success;
}

void uninstall() {
    LOG_INFO("Uninstalling WMI hooks...");
    
    if (g_hooked) {
        HookManager::instance().remove_hook("IWbemClassObject::Get");
        g_hooked = false;
    }
}

} // namespace wmi
} // namespace hooks
} // namespace hwid

