#pragma once

#include <Windows.h>
#include <string>
#include <vector>

namespace hwid {

// 进程信息
struct ProcessInfo {
    DWORD pid;
    std::wstring name;
    std::wstring path;
};

// 注入器类
class Injector {
public:
    // 注入方式
    enum class Method {
        LOADLIBRARY,        // LoadLibrary注入
        MANUAL_MAP,         // 手动映射（更隐蔽）
        THREAD_HIJACK,      // 线程劫持
    };
    
    // 注入DLL到指定进程
    static bool inject(DWORD pid, const std::wstring& dll_path, Method method = Method::LOADLIBRARY);
    
    // 注入DLL到指定进程名
    static bool inject_by_name(const std::wstring& process_name, const std::wstring& dll_path, 
                               Method method = Method::LOADLIBRARY);
    
    // 卸载DLL
    static bool eject(DWORD pid, const std::wstring& dll_name);
    
    // 检查DLL是否已注入
    static bool is_injected(DWORD pid, const std::wstring& dll_name);
    
    // 获取进程列表
    static std::vector<ProcessInfo> get_process_list();
    
    // 根据进程名获取PID
    static DWORD get_pid_by_name(const std::wstring& process_name);
    
    // 根据窗口标题获取PID
    static DWORD get_pid_by_window(const std::wstring& window_title);
    
    // 提升权限
    static bool enable_debug_privilege();
    
    // 等待进程启动
    static DWORD wait_for_process(const std::wstring& process_name, DWORD timeout_ms = INFINITE);

private:
    // LoadLibrary注入
    static bool inject_loadlibrary(HANDLE hProcess, const std::wstring& dll_path);
    
    // 手动映射注入
    static bool inject_manual_map(HANDLE hProcess, const std::wstring& dll_path);
};

// 进程工具函数
namespace process {
    // 打开进程（带权限提升）
    HANDLE open_process(DWORD pid, DWORD access = PROCESS_ALL_ACCESS);
    
    // 获取进程模块基址
    HMODULE get_module_base(DWORD pid, const std::wstring& module_name);
    
    // 获取进程位数
    bool is_process_64bit(HANDLE hProcess);
    
    // 获取当前进程位数
    bool is_current_64bit();
    
    // 检查进程是否运行中
    bool is_process_running(DWORD pid);
    
    // 等待进程空闲
    bool wait_for_input_idle(DWORD pid, DWORD timeout_ms = 5000);
}

} // namespace hwid

