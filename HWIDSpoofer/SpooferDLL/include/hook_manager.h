#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace hwid {

// Hook状态
enum class HookStatus {
    NOT_INSTALLED,
    INSTALLED,
    ENABLED,
    DISABLED,
    FAILED
};

// 单个Hook信息
struct HookInfo {
    std::string name;
    void* target;
    void* detour;
    void* original;
    HookStatus status;
};

// Hook管理器 - 封装MinHook
class HookManager {
public:
    static HookManager& instance() {
        static HookManager instance;
        return instance;
    }
    
    // 初始化/清理
    bool initialize();
    void uninitialize();
    bool is_initialized() const { return m_initialized; }
    
    // 创建Hook
    bool create_hook(const std::string& name, void* target, void* detour, void** original);
    
    // 从模块导出函数创建Hook
    bool create_hook_api(const std::string& name, 
                         const char* module_name, 
                         const char* proc_name, 
                         void* detour, 
                         void** original);
    
    // 从模块导出函数创建Hook (宽字符版本)
    bool create_hook_api(const std::string& name,
                         const wchar_t* module_name,
                         const char* proc_name,
                         void* detour,
                         void** original);
    
    // 启用/禁用单个Hook
    bool enable_hook(const std::string& name);
    bool disable_hook(const std::string& name);
    
    // 启用/禁用所有Hook
    bool enable_all_hooks();
    bool disable_all_hooks();
    
    // 移除Hook
    bool remove_hook(const std::string& name);
    void remove_all_hooks();
    
    // 获取原始函数指针
    template<typename T>
    T get_original(const std::string& name) {
        auto it = m_hooks.find(name);
        if (it != m_hooks.end()) {
            return reinterpret_cast<T>(it->second.original);
        }
        return nullptr;
    }
    
    // 获取Hook信息
    const HookInfo* get_hook_info(const std::string& name) const;
    std::vector<HookInfo> get_all_hooks() const;
    
    // 状态查询
    size_t get_active_hook_count() const;
    bool is_hook_enabled(const std::string& name) const;

private:
    HookManager() = default;
    ~HookManager();
    HookManager(const HookManager&) = delete;
    HookManager& operator=(const HookManager&) = delete;
    
    std::unordered_map<std::string, HookInfo> m_hooks;
    bool m_initialized = false;
};

// 便捷宏定义
#define DECLARE_HOOK(ret_type, call_conv, name, ...) \
    typedef ret_type (call_conv *name##_t)(__VA_ARGS__); \
    extern name##_t Original_##name; \
    ret_type call_conv Hooked_##name(__VA_ARGS__);

#define DEFINE_HOOK(ret_type, call_conv, name, ...) \
    name##_t Original_##name = nullptr;

#define INSTALL_HOOK(module, name) \
    HookManager::instance().create_hook_api(#name, module, #name, \
        (void*)&Hooked_##name, (void**)&Original_##name)

#define INSTALL_HOOK_W(module, name) \
    HookManager::instance().create_hook_api(#name, L##module, #name, \
        (void*)&Hooked_##name, (void**)&Original_##name)

} // namespace hwid

