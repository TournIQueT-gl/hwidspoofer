#include "injector.h"
#include <iostream>
#include <string>
#include <filesystem>

using namespace hwid;

void print_banner() {
    std::wcout << L"\n";
    std::wcout << L"  ╔═══════════════════════════════════════════════════════════╗\n";
    std::wcout << L"  ║                    HWID Spoofer v1.0                       ║\n";
    std::wcout << L"  ║              Hardware ID Spoofing Tool                     ║\n";
    std::wcout << L"  ╚═══════════════════════════════════════════════════════════╝\n";
    std::wcout << L"\n";
}

void print_usage() {
    std::wcout << L"Usage:\n";
    std::wcout << L"  HWIDInjector.exe [options]\n\n";
    std::wcout << L"Options:\n";
    std::wcout << L"  -p, --pid <PID>          Inject into process with specified PID\n";
    std::wcout << L"  -n, --name <name>        Inject into process with specified name\n";
    std::wcout << L"  -w, --wait <name>        Wait for process to start, then inject\n";
    std::wcout << L"  -d, --dll <path>         Path to SpooferDLL.dll (default: SpooferDLL.dll)\n";
    std::wcout << L"  -e, --eject              Eject (unload) the DLL instead of inject\n";
    std::wcout << L"  -l, --list               List all running processes\n";
    std::wcout << L"  -c, --check              Check if DLL is already injected\n";
    std::wcout << L"  -h, --help               Show this help message\n\n";
    std::wcout << L"Examples:\n";
    std::wcout << L"  HWIDInjector.exe -n notepad.exe\n";
    std::wcout << L"  HWIDInjector.exe -p 1234 -d C:\\path\\to\\SpooferDLL.dll\n";
    std::wcout << L"  HWIDInjector.exe -w game.exe --dll SpooferDLL.dll\n";
    std::wcout << L"  HWIDInjector.exe -l\n";
}

void list_processes() {
    std::wcout << L"\nRunning Processes:\n";
    std::wcout << L"══════════════════════════════════════════════════════════════\n";
    std::wcout << L"  PID       Name\n";
    std::wcout << L"──────────────────────────────────────────────────────────────\n";
    
    auto processes = Injector::get_process_list();
    for (const auto& proc : processes) {
        if (proc.pid > 4) {  // 跳过系统进程
            std::wcout << L"  " << std::setw(8) << proc.pid << L"  " << proc.name << L"\n";
        }
    }
    
    std::wcout << L"══════════════════════════════════════════════════════════════\n";
    std::wcout << L"Total: " << processes.size() << L" processes\n";
}

std::wstring get_dll_path(const std::wstring& specified_path) {
    if (!specified_path.empty()) {
        return specified_path;
    }
    
    // 获取当前exe所在目录
    wchar_t exe_path[MAX_PATH];
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
    
    std::filesystem::path dir = std::filesystem::path(exe_path).parent_path();
    std::filesystem::path dll_path = dir / L"SpooferDLL.dll";
    
    return dll_path.wstring();
}

int wmain(int argc, wchar_t* argv[]) {
    // 设置控制台输出为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    print_banner();
    
    if (argc < 2) {
        print_usage();
        return 0;
    }
    
    // 解析命令行参数
    DWORD target_pid = 0;
    std::wstring target_name;
    std::wstring dll_path;
    bool wait_mode = false;
    bool eject_mode = false;
    bool list_mode = false;
    bool check_mode = false;
    
    for (int i = 1; i < argc; i++) {
        std::wstring arg = argv[i];
        
        if (arg == L"-p" || arg == L"--pid") {
            if (i + 1 < argc) {
                target_pid = _wtoi(argv[++i]);
            }
        }
        else if (arg == L"-n" || arg == L"--name") {
            if (i + 1 < argc) {
                target_name = argv[++i];
            }
        }
        else if (arg == L"-w" || arg == L"--wait") {
            if (i + 1 < argc) {
                target_name = argv[++i];
                wait_mode = true;
            }
        }
        else if (arg == L"-d" || arg == L"--dll") {
            if (i + 1 < argc) {
                dll_path = argv[++i];
            }
        }
        else if (arg == L"-e" || arg == L"--eject") {
            eject_mode = true;
        }
        else if (arg == L"-l" || arg == L"--list") {
            list_mode = true;
        }
        else if (arg == L"-c" || arg == L"--check") {
            check_mode = true;
        }
        else if (arg == L"-h" || arg == L"--help") {
            print_usage();
            return 0;
        }
    }
    
    // 列出进程
    if (list_mode) {
        list_processes();
        return 0;
    }
    
    // 获取DLL路径
    dll_path = get_dll_path(dll_path);
    
    std::wcout << L"DLL Path: " << dll_path << L"\n\n";
    
    // 检查DLL是否存在
    if (!eject_mode && GetFileAttributesW(dll_path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::wcerr << L"Error: DLL file not found: " << dll_path << L"\n";
        return 1;
    }
    
    // 提升权限
    std::wcout << L"[*] Enabling debug privileges...\n";
    if (Injector::enable_debug_privilege()) {
        std::wcout << L"[+] Debug privileges enabled\n";
    } else {
        std::wcout << L"[!] Warning: Failed to enable debug privileges\n";
    }
    
    // 确定目标PID
    if (target_pid == 0 && !target_name.empty()) {
        if (wait_mode) {
            std::wcout << L"[*] Waiting for process: " << target_name << L"\n";
            target_pid = Injector::wait_for_process(target_name, 60000);  // 60秒超时
            
            if (target_pid == 0) {
                std::wcerr << L"[-] Timeout waiting for process\n";
                return 1;
            }
            
            std::wcout << L"[+] Process found! PID: " << target_pid << L"\n";
            
            // 等待进程初始化
            std::wcout << L"[*] Waiting for process initialization...\n";
            Sleep(2000);
        } else {
            target_pid = Injector::get_pid_by_name(target_name);
            if (target_pid == 0) {
                std::wcerr << L"[-] Process not found: " << target_name << L"\n";
                return 1;
            }
        }
    }
    
    if (target_pid == 0) {
        std::wcerr << L"[-] No target process specified\n";
        print_usage();
        return 1;
    }
    
    std::wcout << L"[*] Target PID: " << target_pid << L"\n";
    
    // 检查模式
    if (check_mode) {
        std::wstring dll_name = std::filesystem::path(dll_path).filename().wstring();
        bool injected = Injector::is_injected(target_pid, dll_name);
        
        if (injected) {
            std::wcout << L"[+] DLL is currently injected\n";
        } else {
            std::wcout << L"[-] DLL is not injected\n";
        }
        
        return injected ? 0 : 1;
    }
    
    // 卸载模式
    if (eject_mode) {
        std::wstring dll_name = std::filesystem::path(dll_path).filename().wstring();
        std::wcout << L"[*] Ejecting DLL: " << dll_name << L"\n";
        
        if (Injector::eject(target_pid, dll_name)) {
            std::wcout << L"[+] DLL ejected successfully!\n";
            return 0;
        } else {
            std::wcerr << L"[-] Failed to eject DLL\n";
            return 1;
        }
    }
    
    // 注入模式
    std::wcout << L"[*] Injecting DLL...\n";
    
    if (Injector::inject(target_pid, dll_path)) {
        std::wcout << L"\n[+] ═══════════════════════════════════════════════════\n";
        std::wcout << L"[+] DLL injected successfully!\n";
        std::wcout << L"[+] HWID Spoofer is now active in the target process.\n";
        std::wcout << L"[+] ═══════════════════════════════════════════════════\n\n";
        return 0;
    } else {
        std::wcerr << L"\n[-] ═══════════════════════════════════════════════════\n";
        std::wcerr << L"[-] Failed to inject DLL!\n";
        std::wcerr << L"[-] Make sure:\n";
        std::wcerr << L"[-]   1. The target process is running\n";
        std::wcerr << L"[-]   2. You have administrator privileges\n";
        std::wcerr << L"[-]   3. The DLL architecture matches the process (32/64-bit)\n";
        std::wcerr << L"[-] ═══════════════════════════════════════════════════\n\n";
        return 1;
    }
}

// 标准main入口（非Unicode）
int main(int argc, char* argv[]) {
    // 转换参数为宽字符
    wchar_t** wargv = new wchar_t*[argc];
    for (int i = 0; i < argc; i++) {
        int len = MultiByteToWideChar(CP_ACP, 0, argv[i], -1, nullptr, 0);
        wargv[i] = new wchar_t[len];
        MultiByteToWideChar(CP_ACP, 0, argv[i], -1, wargv[i], len);
    }
    
    int result = wmain(argc, wargv);
    
    // 清理
    for (int i = 0; i < argc; i++) {
        delete[] wargv[i];
    }
    delete[] wargv;
    
    return result;
}

