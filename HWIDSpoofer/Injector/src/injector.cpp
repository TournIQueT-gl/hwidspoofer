#include "injector.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>

#pragma comment(lib, "psapi.lib")

namespace hwid {

bool Injector::enable_debug_privilege() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }
    
    if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid)) {
        CloseHandle(hToken);
        return false;
    }
    
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr)) {
        CloseHandle(hToken);
        return false;
    }
    
    CloseHandle(hToken);
    return GetLastError() != ERROR_NOT_ALL_ASSIGNED;
}

std::vector<ProcessInfo> Injector::get_process_list() {
    std::vector<ProcessInfo> processes;
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processes;
    }
    
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            ProcessInfo info;
            info.pid = pe.th32ProcessID;
            info.name = pe.szExeFile;
            
            // 尝试获取完整路径
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
            if (hProcess) {
                wchar_t path[MAX_PATH];
                DWORD size = MAX_PATH;
                if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
                    info.path = path;
                }
                CloseHandle(hProcess);
            }
            
            processes.push_back(info);
        } while (Process32NextW(hSnapshot, &pe));
    }
    
    CloseHandle(hSnapshot);
    return processes;
}

DWORD Injector::get_pid_by_name(const std::wstring& process_name) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    
    DWORD pid = 0;
    
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, process_name.c_str()) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    
    CloseHandle(hSnapshot);
    return pid;
}

DWORD Injector::get_pid_by_window(const std::wstring& window_title) {
    HWND hWnd = FindWindowW(nullptr, window_title.c_str());
    if (!hWnd) {
        return 0;
    }
    
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    return pid;
}

DWORD Injector::wait_for_process(const std::wstring& process_name, DWORD timeout_ms) {
    DWORD start_time = GetTickCount();
    
    while (true) {
        DWORD pid = get_pid_by_name(process_name);
        if (pid != 0) {
            return pid;
        }
        
        if (timeout_ms != INFINITE) {
            if (GetTickCount() - start_time >= timeout_ms) {
                return 0;
            }
        }
        
        Sleep(100);
    }
}

bool Injector::inject(DWORD pid, const std::wstring& dll_path, Method method) {
    if (pid == 0) {
        std::wcerr << L"Invalid PID" << std::endl;
        return false;
    }
    
    // 检查DLL是否存在
    if (GetFileAttributesW(dll_path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::wcerr << L"DLL file not found: " << dll_path << std::endl;
        return false;
    }
    
    // 打开目标进程
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | 
        PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
        FALSE, pid);
    
    if (!hProcess) {
        std::wcerr << L"Failed to open process. Error: " << GetLastError() << std::endl;
        return false;
    }
    
    bool result = false;
    
    switch (method) {
        case Method::LOADLIBRARY:
            result = inject_loadlibrary(hProcess, dll_path);
            break;
        case Method::MANUAL_MAP:
            result = inject_manual_map(hProcess, dll_path);
            break;
        case Method::THREAD_HIJACK:
            // 未实现
            std::wcerr << L"Thread hijack injection not implemented" << std::endl;
            break;
    }
    
    CloseHandle(hProcess);
    return result;
}

bool Injector::inject_by_name(const std::wstring& process_name, const std::wstring& dll_path, Method method) {
    DWORD pid = get_pid_by_name(process_name);
    if (pid == 0) {
        std::wcerr << L"Process not found: " << process_name << std::endl;
        return false;
    }
    
    return inject(pid, dll_path, method);
}

bool Injector::inject_loadlibrary(HANDLE hProcess, const std::wstring& dll_path) {
    // 计算需要的内存大小
    size_t path_size = (dll_path.length() + 1) * sizeof(wchar_t);
    
    // 在目标进程中分配内存
    LPVOID pRemotePath = VirtualAllocEx(hProcess, nullptr, path_size, 
                                        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemotePath) {
        std::wcerr << L"Failed to allocate memory in target process" << std::endl;
        return false;
    }
    
    // 写入DLL路径
    if (!WriteProcessMemory(hProcess, pRemotePath, dll_path.c_str(), path_size, nullptr)) {
        std::wcerr << L"Failed to write DLL path to target process" << std::endl;
        VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
        return false;
    }
    
    // 获取LoadLibraryW地址
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    LPVOID pLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryW");
    
    if (!pLoadLibrary) {
        std::wcerr << L"Failed to get LoadLibraryW address" << std::endl;
        VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
        return false;
    }
    
    // 创建远程线程执行LoadLibraryW
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                        (LPTHREAD_START_ROUTINE)pLoadLibrary,
                                        pRemotePath, 0, nullptr);
    
    if (!hThread) {
        std::wcerr << L"Failed to create remote thread. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
        return false;
    }
    
    // 等待线程完成
    WaitForSingleObject(hThread, 5000);
    
    // 获取返回值（加载的模块句柄）
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    
    // 清理
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
    
    if (exitCode == 0) {
        std::wcerr << L"LoadLibraryW failed in target process" << std::endl;
        return false;
    }
    
    std::wcout << L"DLL injected successfully. Module handle: 0x" << std::hex << exitCode << std::endl;
    return true;
}

bool Injector::inject_manual_map(HANDLE hProcess, const std::wstring& dll_path) {
    // 手动映射注入需要更复杂的实现
    // 包括：PE解析、重定位、导入表处理、TLS回调等
    // 这里暂时使用LoadLibrary方式
    
    std::wcout << L"Manual map injection not fully implemented, falling back to LoadLibrary" << std::endl;
    return inject_loadlibrary(hProcess, dll_path);
}

bool Injector::eject(DWORD pid, const std::wstring& dll_name) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    MODULEENTRY32W me;
    me.dwSize = sizeof(me);
    
    HMODULE hModule = nullptr;
    
    if (Module32FirstW(hSnapshot, &me)) {
        do {
            if (_wcsicmp(me.szModule, dll_name.c_str()) == 0) {
                hModule = me.hModule;
                break;
            }
        } while (Module32NextW(hSnapshot, &me));
    }
    
    CloseHandle(hSnapshot);
    
    if (!hModule) {
        std::wcerr << L"Module not found in target process" << std::endl;
        return false;
    }
    
    // 打开进程
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
        FALSE, pid);
    
    if (!hProcess) {
        return false;
    }
    
    // 获取FreeLibrary地址
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    LPVOID pFreeLibrary = GetProcAddress(hKernel32, "FreeLibrary");
    
    // 创建远程线程执行FreeLibrary
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                        (LPTHREAD_START_ROUTINE)pFreeLibrary,
                                        hModule, 0, nullptr);
    
    if (!hThread) {
        CloseHandle(hProcess);
        return false;
    }
    
    WaitForSingleObject(hThread, 5000);
    
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    
    CloseHandle(hThread);
    CloseHandle(hProcess);
    
    return exitCode != 0;
}

bool Injector::is_injected(DWORD pid, const std::wstring& dll_name) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    MODULEENTRY32W me;
    me.dwSize = sizeof(me);
    
    bool found = false;
    
    if (Module32FirstW(hSnapshot, &me)) {
        do {
            if (_wcsicmp(me.szModule, dll_name.c_str()) == 0) {
                found = true;
                break;
            }
        } while (Module32NextW(hSnapshot, &me));
    }
    
    CloseHandle(hSnapshot);
    return found;
}

namespace process {

HANDLE open_process(DWORD pid, DWORD access) {
    HANDLE hProcess = OpenProcess(access, FALSE, pid);
    
    if (!hProcess) {
        // 尝试提升权限后重试
        Injector::enable_debug_privilege();
        hProcess = OpenProcess(access, FALSE, pid);
    }
    
    return hProcess;
}

HMODULE get_module_base(DWORD pid, const std::wstring& module_name) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return nullptr;
    }
    
    MODULEENTRY32W me;
    me.dwSize = sizeof(me);
    
    HMODULE hModule = nullptr;
    
    if (Module32FirstW(hSnapshot, &me)) {
        do {
            if (_wcsicmp(me.szModule, module_name.c_str()) == 0) {
                hModule = me.hModule;
                break;
            }
        } while (Module32NextW(hSnapshot, &me));
    }
    
    CloseHandle(hSnapshot);
    return hModule;
}

bool is_process_64bit(HANDLE hProcess) {
    BOOL isWow64 = FALSE;
    
    // 检查是否在WOW64下运行（32位进程在64位系统上）
    typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)
        GetProcAddress(GetModuleHandleW(L"kernel32"), "IsWow64Process");
    
    if (fnIsWow64Process) {
        if (!fnIsWow64Process(hProcess, &isWow64)) {
            return false;
        }
    }
    
    // 如果系统是64位且进程不是WOW64，则进程是64位
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
        si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {
        return !isWow64;
    }
    
    return false;
}

bool is_current_64bit() {
#ifdef _WIN64
    return true;
#else
    return false;
#endif
}

bool is_process_running(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) {
        return false;
    }
    
    DWORD exitCode = 0;
    GetExitCodeProcess(hProcess, &exitCode);
    CloseHandle(hProcess);
    
    return exitCode == STILL_ACTIVE;
}

bool wait_for_input_idle(DWORD pid, DWORD timeout_ms) {
    HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!hProcess) {
        return false;
    }
    
    // WaitForInputIdle只适用于GUI进程
    // 对于非GUI进程，简单等待一段时间
    Sleep(500);
    
    CloseHandle(hProcess);
    return true;
}

} // namespace process

} // namespace hwid

