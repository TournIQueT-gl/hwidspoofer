#include "hook_manager.h"
#include "logger.h"
#include "MinHook.h"

namespace hwid {

HookManager::~HookManager() {
    if (m_initialized) {
        uninitialize();
    }
}

bool HookManager::initialize() {
    if (m_initialized) {
        return true;
    }
    
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        LOG_ERROR("MH_Initialize failed: %s", MH_StatusToString(status));
        return false;
    }
    
    m_initialized = true;
    LOG_DEBUG("MinHook initialized successfully");
    return true;
}

void HookManager::uninitialize() {
    if (!m_initialized) {
        return;
    }
    
    // 禁用并移除所有Hook
    remove_all_hooks();
    
    MH_STATUS status = MH_Uninitialize();
    if (status != MH_OK) {
        LOG_ERROR("MH_Uninitialize failed: %s", MH_StatusToString(status));
    }
    
    m_initialized = false;
    LOG_DEBUG("MinHook uninitialized");
}

bool HookManager::create_hook(const std::string& name, void* target, void* detour, void** original) {
    if (!m_initialized) {
        LOG_ERROR("HookManager not initialized");
        return false;
    }
    
    if (target == nullptr || detour == nullptr) {
        LOG_ERROR("Invalid target or detour for hook: %s", name.c_str());
        return false;
    }
    
    // 检查是否已存在
    if (m_hooks.find(name) != m_hooks.end()) {
        LOG_WARNING("Hook already exists: %s", name.c_str());
        return false;
    }
    
    MH_STATUS status = MH_CreateHook(target, detour, original);
    if (status != MH_OK) {
        LOG_ERROR("MH_CreateHook failed for %s: %s", name.c_str(), MH_StatusToString(status));
        return false;
    }
    
    HookInfo info;
    info.name = name;
    info.target = target;
    info.detour = detour;
    info.original = original ? *original : nullptr;
    info.status = HookStatus::INSTALLED;
    
    m_hooks[name] = info;
    LOG_DEBUG("Hook created: %s (target=0x%p)", name.c_str(), target);
    
    return true;
}

bool HookManager::create_hook_api(const std::string& name, const char* module_name, 
                                   const char* proc_name, void* detour, void** original) {
    if (!m_initialized) {
        LOG_ERROR("HookManager not initialized");
        return false;
    }
    
    // 检查是否已存在
    if (m_hooks.find(name) != m_hooks.end()) {
        LOG_WARNING("Hook already exists: %s", name.c_str());
        return false;
    }
    
    MH_STATUS status = MH_CreateHookApi(module_name, proc_name, detour, original);
    if (status != MH_OK) {
        LOG_ERROR("MH_CreateHookApi failed for %s!%s: %s", 
                  module_name, proc_name, MH_StatusToString(status));
        return false;
    }
    
    // 获取目标地址用于后续操作
    HMODULE hModule = GetModuleHandleA(module_name);
    void* target = nullptr;
    if (hModule) {
        target = GetProcAddress(hModule, proc_name);
    }
    
    HookInfo info;
    info.name = name;
    info.target = target;
    info.detour = detour;
    info.original = original ? *original : nullptr;
    info.status = HookStatus::INSTALLED;
    
    m_hooks[name] = info;
    LOG_DEBUG("API Hook created: %s (%s!%s)", name.c_str(), module_name, proc_name);
    
    return true;
}

bool HookManager::create_hook_api(const std::string& name, const wchar_t* module_name,
                                   const char* proc_name, void* detour, void** original) {
    if (!m_initialized) {
        LOG_ERROR("HookManager not initialized");
        return false;
    }
    
    // 检查是否已存在
    if (m_hooks.find(name) != m_hooks.end()) {
        LOG_WARNING("Hook already exists: %s", name.c_str());
        return false;
    }
    
    MH_STATUS status = MH_CreateHookApiEx(module_name, proc_name, detour, original, nullptr);
    if (status != MH_OK) {
        LOG_ERROR("MH_CreateHookApiEx failed for %ls!%s: %s", 
                  module_name, proc_name, MH_StatusToString(status));
        return false;
    }
    
    // 获取目标地址
    HMODULE hModule = GetModuleHandleW(module_name);
    void* target = nullptr;
    if (hModule) {
        target = GetProcAddress(hModule, proc_name);
    }
    
    HookInfo info;
    info.name = name;
    info.target = target;
    info.detour = detour;
    info.original = original ? *original : nullptr;
    info.status = HookStatus::INSTALLED;
    
    m_hooks[name] = info;
    LOG_DEBUG("API Hook created: %s (%ls!%s)", name.c_str(), module_name, proc_name);
    
    return true;
}

bool HookManager::enable_hook(const std::string& name) {
    auto it = m_hooks.find(name);
    if (it == m_hooks.end()) {
        LOG_ERROR("Hook not found: %s", name.c_str());
        return false;
    }
    
    if (it->second.status == HookStatus::ENABLED) {
        return true;
    }
    
    MH_STATUS status = MH_EnableHook(it->second.target);
    if (status != MH_OK) {
        LOG_ERROR("MH_EnableHook failed for %s: %s", name.c_str(), MH_StatusToString(status));
        it->second.status = HookStatus::FAILED;
        return false;
    }
    
    it->second.status = HookStatus::ENABLED;
    LOG_DEBUG("Hook enabled: %s", name.c_str());
    return true;
}

bool HookManager::disable_hook(const std::string& name) {
    auto it = m_hooks.find(name);
    if (it == m_hooks.end()) {
        LOG_ERROR("Hook not found: %s", name.c_str());
        return false;
    }
    
    if (it->second.status == HookStatus::DISABLED || 
        it->second.status == HookStatus::INSTALLED) {
        return true;
    }
    
    MH_STATUS status = MH_DisableHook(it->second.target);
    if (status != MH_OK) {
        LOG_ERROR("MH_DisableHook failed for %s: %s", name.c_str(), MH_StatusToString(status));
        return false;
    }
    
    it->second.status = HookStatus::DISABLED;
    LOG_DEBUG("Hook disabled: %s", name.c_str());
    return true;
}

bool HookManager::enable_all_hooks() {
    MH_STATUS status = MH_EnableHook(MH_ALL_HOOKS);
    if (status != MH_OK) {
        LOG_ERROR("MH_EnableHook(ALL) failed: %s", MH_StatusToString(status));
        return false;
    }
    
    for (auto& pair : m_hooks) {
        pair.second.status = HookStatus::ENABLED;
    }
    
    LOG_DEBUG("All hooks enabled");
    return true;
}

bool HookManager::disable_all_hooks() {
    MH_STATUS status = MH_DisableHook(MH_ALL_HOOKS);
    if (status != MH_OK) {
        LOG_ERROR("MH_DisableHook(ALL) failed: %s", MH_StatusToString(status));
        return false;
    }
    
    for (auto& pair : m_hooks) {
        pair.second.status = HookStatus::DISABLED;
    }
    
    LOG_DEBUG("All hooks disabled");
    return true;
}

bool HookManager::remove_hook(const std::string& name) {
    auto it = m_hooks.find(name);
    if (it == m_hooks.end()) {
        return true;
    }
    
    // 先禁用
    if (it->second.status == HookStatus::ENABLED) {
        disable_hook(name);
    }
    
    MH_STATUS status = MH_RemoveHook(it->second.target);
    if (status != MH_OK && status != MH_ERROR_NOT_CREATED) {
        LOG_ERROR("MH_RemoveHook failed for %s: %s", name.c_str(), MH_StatusToString(status));
        return false;
    }
    
    m_hooks.erase(it);
    LOG_DEBUG("Hook removed: %s", name.c_str());
    return true;
}

void HookManager::remove_all_hooks() {
    disable_all_hooks();
    
    for (auto& pair : m_hooks) {
        MH_RemoveHook(pair.second.target);
    }
    
    m_hooks.clear();
    LOG_DEBUG("All hooks removed");
}

const HookInfo* HookManager::get_hook_info(const std::string& name) const {
    auto it = m_hooks.find(name);
    if (it != m_hooks.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<HookInfo> HookManager::get_all_hooks() const {
    std::vector<HookInfo> hooks;
    for (const auto& pair : m_hooks) {
        hooks.push_back(pair.second);
    }
    return hooks;
}

size_t HookManager::get_active_hook_count() const {
    size_t count = 0;
    for (const auto& pair : m_hooks) {
        if (pair.second.status == HookStatus::ENABLED) {
            count++;
        }
    }
    return count;
}

bool HookManager::is_hook_enabled(const std::string& name) const {
    auto it = m_hooks.find(name);
    if (it != m_hooks.end()) {
        return it->second.status == HookStatus::ENABLED;
    }
    return false;
}

} // namespace hwid

