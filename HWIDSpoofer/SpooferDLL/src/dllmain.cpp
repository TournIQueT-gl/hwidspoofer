#include <Windows.h>
#include "hook_manager.h"
#include "hooks.h"
#include "logger.h"
#include "../Common/config.h"
#include <filesystem>

using namespace hwid;

// Resolve the directory containing this DLL. Never return a partial path.
bool try_get_dll_directory(std::wstring& directory) {
    directory.clear();

    HMODULE hModule = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&try_get_dll_directory), &hModule)) {
        return false;
    }

    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(hModule, path, ARRAYSIZE(path));
    // A return value equal to the buffer size is treated as truncation on all
    // supported Windows versions, so a truncated path is never used.
    if (length == 0 || length >= ARRAYSIZE(path)) {
        return false;
    }

    std::wstring full_path(path, length);
    const size_t pos = full_path.find_last_of(L"\\/");
    if (pos == std::wstring::npos || pos == 0) {
        return false;
    }

    directory = full_path.substr(0, pos);
    return !directory.empty();
}

// 初始化Spoofer
bool initialize_spoofer() {
    // 获取DLL目录
    std::wstring dll_dir;
    if (!try_get_dll_directory(dll_dir)) {
        return false;
    }

    // 初始化日志
    std::wstring log_path = dll_dir + L"\\hwid_spoofer.log";
    Logger::instance().initialize(log_path);
    Logger::instance().set_level(LogLevel::DEBUG);

    LOG_INFO("========================================");
    LOG_INFO("HWID Spoofer DLL Initializing...");
    LOG_INFO("DLL Directory: %ls", dll_dir.c_str());

    // 加载配置
    std::wstring config_path = dll_dir + L"\\hwid_config.json";
    if (!Config::instance().load_from_file(config_path)) {
        LOG_WARNING("Config file not found, generating random config...");
        Config::instance().generate_random_config();
        Config::instance().save_to_file(std::string(config_path.begin(), config_path.end()));
    }

    LOG_INFO("Configuration loaded successfully");

    // 初始化Hook管理器
    if (!HookManager::instance().initialize()) {
        LOG_ERROR("Failed to initialize HookManager!");
        return false;
    }

    LOG_INFO("HookManager initialized");

    // 安装所有Hook
    if (!hooks::install_all_hooks()) {
        LOG_ERROR("Failed to install hooks!");
        return false;
    }

    LOG_INFO("All hooks installed successfully");

    // 启用所有Hook
    if (!HookManager::instance().enable_all_hooks()) {
        LOG_ERROR("Failed to enable hooks!");
        return false;
    }

    LOG_INFO("All hooks enabled");
    LOG_INFO("HWID Spoofer initialized successfully!");
    LOG_INFO("Active hooks: %zu", HookManager::instance().get_active_hook_count());
    LOG_INFO("========================================");

    return true;
}

// 清理Spoofer
void cleanup_spoofer() {
    LOG_INFO("========================================");
    LOG_INFO("HWID Spoofer shutting down...");

    // 卸载所有Hook
    hooks::uninstall_all_hooks();

    // 清理Hook管理器
    HookManager::instance().uninitialize();

    LOG_INFO("HWID Spoofer shutdown complete");
    LOG_INFO("========================================");

    // 关闭日志
    Logger::instance().shutdown();
}

// DLL入口点
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            // 禁用DLL_THREAD_ATTACH/DETACH通知以提高性能
            DisableThreadLibraryCalls(hModule);

            // 在新线程中初始化，避免死锁
            // 注意：某些情况下可能需要同步初始化
            if (!initialize_spoofer()) {
                return FALSE;
            }
            break;

        case DLL_PROCESS_DETACH:
            // 仅在正常卸载时清理（lpReserved为NULL）
            if (lpReserved == nullptr) {
                cleanup_spoofer();
            }
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}

// 导出函数 - 用于手动控制
extern "C" {
    __declspec(dllexport) BOOL WINAPI EnableSpoofer() {
        return HookManager::instance().enable_all_hooks() ? TRUE : FALSE;
    }

    __declspec(dllexport) BOOL WINAPI DisableSpoofer() {
        return HookManager::instance().disable_all_hooks() ? TRUE : FALSE;
    }

    __declspec(dllexport) BOOL WINAPI ReloadConfig() {
        std::wstring dll_dir;
        if (!try_get_dll_directory(dll_dir)) {
            return FALSE;
        }
        std::wstring config_path = dll_dir + L"\\hwid_config.json";
        return Config::instance().load_from_file(config_path) ? TRUE : FALSE;
    }

    __declspec(dllexport) BOOL WINAPI GenerateRandomConfig() {
        Config::instance().generate_random_config();
        return TRUE;
    }

    __declspec(dllexport) DWORD WINAPI GetActiveHookCount() {
        return static_cast<DWORD>(HookManager::instance().get_active_hook_count());
    }

    __declspec(dllexport) BOOL WINAPI IsHookEnabled(const char* hook_name) {
        return HookManager::instance().is_hook_enabled(hook_name) ? TRUE : FALSE;
    }
}
