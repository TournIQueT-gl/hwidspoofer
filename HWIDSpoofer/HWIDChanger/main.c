/*
 * HWID Changer - 主程序
 * 单文件可执行程序
 */

#include "hwid_changer.h"
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <TlHelp32.h>

// 控制台颜色
#define COLOR_DEFAULT   7
#define COLOR_GREEN     10
#define COLOR_YELLOW    14
#define COLOR_RED       12
#define COLOR_CYAN      11
#define COLOR_WHITE     15

static void set_color(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color);
}

static void print_banner(void) {
    set_color(COLOR_CYAN);
    printf("\n");
    printf("  +======================================================+\n");
    printf("  |           HWID Changer v%s                         |\n", HWID_VERSION);
    printf("  |        Hardware ID Spoofing Tool                     |\n");
    printf("  +======================================================+\n");
    set_color(COLOR_DEFAULT);
    printf("\n");
}

static void print_config(const HwidConfig* config) {
    set_color(COLOR_WHITE);
    printf("  Current Configuration:\n");
    set_color(COLOR_DEFAULT);
    printf("  --------------------------------------------------------\n");
    
    printf("  Disk Serial:      %s\n", config->disk_serial);
    printf("  Volume Serial:    0x%08X\n", config->volume_serial);
    printf("  BIOS Serial:      %s\n", config->bios_serial);
    printf("  Board Serial:     %s\n", config->board_serial);
    printf("  System UUID:      %s\n", config->system_uuid);
    printf("  Machine GUID:     %s\n", config->machine_guid);
    printf("  MAC Address:      %02X:%02X:%02X:%02X:%02X:%02X\n",
           config->mac_address[0], config->mac_address[1], config->mac_address[2],
           config->mac_address[3], config->mac_address[4], config->mac_address[5]);
    printf("  CPU ID:           %s\n", config->cpu_id);
    printf("  --------------------------------------------------------\n");
}

static void print_menu(void) {
    set_color(COLOR_WHITE);
    printf("\n  Menu:\n");
    set_color(COLOR_DEFAULT);
    printf("  --------------------------------------------------------\n");
    printf("  [1] Generate random HWID\n");
    printf("  [2] Apply spoof (to current process)\n");
    printf("  [3] Load kernel driver\n");
    printf("  [4] Inject into process\n");
    printf("  [5] Show real hardware info\n");
    printf("  [6] Load config from file\n");
    printf("  [7] Save config to file\n");
    set_color(COLOR_CYAN);
    printf("  [8] MAC override / restore (Windows Sandbox)\n");
    printf("  [9] Apply registry changes (permanent)\n");
    set_color(COLOR_DEFAULT);
    printf("  [0] Exit\n");
    printf("  --------------------------------------------------------\n");
    set_color(COLOR_YELLOW);
    printf("  Enter choice: ");
    set_color(COLOR_DEFAULT);
}

// Run a bundled, read-only Windows PowerShell companion script.
// The script name and arguments are fixed by the application, not user input.
static int run_companion_script(const char* name, const char* args) {
    char script[MAX_PATH], powershell[MAX_PATH], command[2*MAX_PATH+160];
    DWORD n=GetModuleFileNameA(NULL,script,MAX_PATH);
    if (!n || n>=MAX_PATH) { fprintf(stderr,"[ERROR] Cannot locate application folder.\n"); return 1; }
    char* slash=strrchr(script,'\\');
    if (!slash || strlen(name)>=MAX_PATH-(size_t)(slash+1-script)) {
        fprintf(stderr,"[ERROR] Script path too long.\n"); return 1;
    }
    strcpy(slash+1,name);
    if (GetFileAttributesA(script)==INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr,"[ERROR] Required companion missing: %s\n",script);
        fprintf(stderr,"Rebuild with build.bat so the scripts are copied to bin.\n");
        return 1;
    }
    n=GetSystemDirectoryA(powershell,MAX_PATH);
    if (!n || n>=MAX_PATH ||
        strlen(powershell)+strlen("\\WindowsPowerShell\\v1.0\\powershell.exe")>=MAX_PATH) {
        fprintf(stderr,"[ERROR] Cannot locate Windows PowerShell.\n"); return 1;
    }
    strcat(powershell,"\\WindowsPowerShell\\v1.0\\powershell.exe");
    if (snprintf(command,sizeof(command),
                 "\"%s\" -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"%s\" %s",
                 powershell,script,args)<0 || strlen(command)>=sizeof(command)-1) {
        fprintf(stderr,"[ERROR] Script command too long.\n"); return 1;
    }
    STARTUPINFOA si={0};
    PROCESS_INFORMATION pi={0};
    si.cb=sizeof(si);
    if (!CreateProcessA(powershell,command,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)) {
        fprintf(stderr,"[ERROR] PowerShell launch failed: %lu\n",(unsigned long)GetLastError());
        return 1;
    }
    WaitForSingleObject(pi.hProcess,INFINITE);
    DWORD exitCode=1;
    if (!GetExitCodeProcess(pi.hProcess,&exitCode)) exitCode=1;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return exitCode==0?0:1;
}

static void show_real_hwid(void) {
    printf("\n  Real Windows hardware and identifier readings (read-only):\n\n");
    if (run_companion_script("Verify-HWID.ps1","-Phase Before")!=0) {
        fprintf(stderr,"  [ERROR] Hardware information query failed.\n");
    }
    printf("\n  Note: firmware serials are not physical hardware changes.\n");
}

static DWORD get_pid_by_name(const char* name) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    
    PROCESSENTRY32 pe = { sizeof(pe) };
    DWORD pid = 0;
    
    if (Process32First(snapshot, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, name) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &pe));
    }
    
    CloseHandle(snapshot);
    return pid;
}

// 命令行模式
static int run_cli(int argc, char* argv[], HwidConfig* config) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--random") == 0) {
            hwid_generate_random(config);
            printf("Generated random HWID\n");
        }
        else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--apply") == 0) {
            if (hwid_apply(config, MODE_AUTO) == 0) {
                printf("Spoof applied successfully\n");
            } else {
                printf("Failed to apply spoof\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--driver") == 0) {
            if (hwid_load_driver() == 0) {
                printf("Driver loaded\n");
            } else {
                printf("Failed to load driver\n");
                return 1;
            }
        }
        else if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--pid") == 0) && i + 1 < argc) {
            DWORD pid = atoi(argv[++i]);
            if (hwid_inject_process(pid, config) == 0) {
                printf("Injected into PID %d\n", pid);
            } else {
                printf("Failed to inject\n");
                return 1;
            }
        }
        else if ((strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--name") == 0) && i + 1 < argc) {
            if (hwid_inject_by_name(argv[++i], config) == 0) {
                printf("Injected into %s\n", argv[i]);
            } else {
                printf("Failed to inject\n");
                return 1;
            }
        }
        else if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) && i + 1 < argc) {
            if (hwid_load_config(argv[++i], config) == 0) {
                printf("Config loaded from %s\n", argv[i]);
            } else {
                printf("Failed to load config\n");
            }
        }
        else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--save") == 0) && i + 1 < argc) {
            if (hwid_save_config(argv[++i], config) == 0) {
                printf("Config saved to %s\n", argv[i]);
            } else {
                printf("Failed to save config\n");
            }
        }
        else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            // 静默模式
        }
        else if (strcmp(argv[i], "--enable-global") == 0) {
            if (hwid_enable_global_mode(config) == 0) {
                printf("Global mode enabled\n");
            } else {
                printf("Failed to enable global mode\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "--disable-global") == 0) {
            if (hwid_disable_global_mode() == 0) {
                printf("Global mode disabled\n");
            } else {
                printf("Failed to disable global mode\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "--mac-apply") == 0) {
            if (run_companion_script("MacOverride.ps1","-Action Apply") != 0) return 1;
        }
        else if (strcmp(argv[i], "--mac-restore") == 0) {
            if (run_companion_script("MacOverride.ps1","-Action Restore") != 0) return 1;
        }
        else if (strcmp(argv[i], "--mac-status") == 0) {
            if (run_companion_script("MacOverride.ps1","-Action Status") != 0) return 1;
        }
        else if (strcmp(argv[i], "--mac-test") == 0) {
            if (run_companion_script("MacOverride.ps1","-Action Test") != 0) return 1;
        }
        else if (strcmp(argv[i], "--show-hardware") == 0) {
            if (run_companion_script("Verify-HWID.ps1","-Phase Before") != 0) return 1;
        }
        else if (strcmp(argv[i], "--registry-global") == 0) {
            if (hwid_apply_registry_global(config) == 0) {
                printf("Registry modified\n");
            } else {
                printf("Failed to modify registry\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "--restore-registry") == 0) {
            if (hwid_restore_registry() == 0) {
                printf("Registry restored\n");
            } else {
                printf("Failed to restore registry\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "--enable-appinit") == 0) {
            char dll_path[MAX_PATH];
            GetModuleFileNameA(NULL, dll_path, MAX_PATH);
            char* p = strrchr(dll_path, '\\');
            if (p) strcpy(p + 1, "HWIDChanger.dll");
            if (hwid_enable_appinit_injection(dll_path) == 0) {
                printf("AppInit injection enabled (affects new processes)\n");
            } else {
                printf("Failed to enable AppInit injection\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "--disable-appinit") == 0) {
            if (hwid_disable_appinit_injection() == 0) {
                printf("AppInit injection disabled\n");
            } else {
                printf("Failed to disable AppInit injection\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "--full-usermode") == 0) {
            printf("Applying full usermode spoof...\n");
            if (hwid_apply_registry_global(config) != 0) {
                fprintf(stderr, "  [ERROR] Registry stage failed; AppInit not enabled.\n");
                return 1;
            }
            printf("  Registry: OK\n");
            char dll_path[MAX_PATH];
            DWORD dll_len = GetModuleFileNameA(NULL, dll_path, MAX_PATH);
            if (!dll_len || dll_len >= MAX_PATH) {
                fprintf(stderr, "  [ERROR] Cannot locate executable. Attempting rollback.\n");
                hwid_restore_registry();
                return 1;
            }
            char* p = strrchr(dll_path, '\\');
            if (!p || (size_t)(dll_path + MAX_PATH - (p + 1)) < sizeof("HWIDChanger.dll")) {
                fprintf(stderr, "  [ERROR] DLL path is too long. Attempting rollback.\n");
                hwid_restore_registry();
                return 1;
            }
            strcpy(p + 1, "HWIDChanger.dll");
            if (hwid_enable_appinit_injection(dll_path) != 0) {
                fprintf(stderr, "  [ERROR] AppInit stage failed. Attempting registry rollback.\n");
                if (hwid_restore_registry() != 0)
                    fprintf(stderr, "  [ERROR] Manual restoration required.\n");
                return 1;
            }
            printf("  AppInit: OK\nFull usermode spoof enabled!\n");
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("HWID Changer v%s\n\n", HWID_VERSION);
            printf("Usage: %s [options]\n\n", argv[0]);
            printf("Options:\n");
            printf("  -r, --random           Generate random HWID\n");
            printf("  -a, --apply            Apply spoof to current process\n");
            printf("  -d, --driver           Load kernel driver\n");
            printf("  -p, --pid <PID>        Inject into process by PID\n");
            printf("  -n, --name <NAME>      Inject into process by name\n");
            printf("  -c, --config <FILE>    Load config from file\n");
            printf("  -s, --save <FILE>      Save config to file\n");
            printf("  -q, --quiet            Quiet mode\n");
            printf("  -h, --help             Show this help\n");
            printf("  --show-hardware        Read-only Windows hardware report\n");
            printf("  --mac-apply            Apply temporary MAC override in Windows Sandbox\n");
            printf("  --mac-restore          Restore the saved original MAC in Sandbox\n");
            printf("  --mac-status           Display current MAC override status\n");
            printf("  --mac-test             Apply, verify, and immediately restore MAC\n");
            printf("\nGlobal Mode (affects ALL programs):\n");
            printf("  --enable-global        Enable global interception (kernel driver)\n");
            printf("  --disable-global       Disable global interception\n");
            printf("  --registry-global      Modify system registry (permanent)\n");
            printf("  --restore-registry     Restore original registry values\n");
            printf("\nUsermode Full System Spoof:\n");
            printf("  --full-usermode        Apply registry + AppInit (recommended)\n");
            printf("  --enable-appinit       Enable AppInit DLL injection\n");
            printf("  --disable-appinit      Disable AppInit DLL injection\n");
            return 0;
        }
    }
    return 0;
}

// 交互模式
static int run_interactive(HwidConfig* config) {
    print_banner();
    
    // 生成初始随机配置
    hwid_generate_random(config);
    
    while (1) {
        print_config(config);
        print_menu();
        
        int choice = _getch() - '0';
        printf("%d\n", choice);
        
        switch (choice) {
            case 1:  // 生成随机
                hwid_generate_random(config);
                set_color(COLOR_GREEN);
                printf("\n  [OK] Random HWID generated!\n");
                set_color(COLOR_DEFAULT);
                break;
                
            case 2:  // 应用伪装
                set_color(COLOR_YELLOW);
                printf("\n  Applying spoof...\n");
                set_color(COLOR_DEFAULT);
                if (hwid_apply(config, MODE_AUTO) == 0) {
                    set_color(COLOR_GREEN);
                    printf("  [OK] Spoof applied successfully!\n");
                } else {
                    set_color(COLOR_RED);
                    printf("  [ERROR] Failed to apply spoof\n");
                }
                set_color(COLOR_DEFAULT);
                break;
                
            case 3:  // 加载驱动
                set_color(COLOR_YELLOW);
                printf("\n  Loading kernel driver...\n");
                set_color(COLOR_DEFAULT);
                if (hwid_load_driver() == 0) {
                    hwid_driver_set_config(config);
                    set_color(COLOR_GREEN);
                    printf("  [OK] Driver loaded and configured!\n");
                } else {
                    set_color(COLOR_RED);
                    printf("  [ERROR] Failed to load driver (need admin rights and test mode)\n");
                }
                set_color(COLOR_DEFAULT);
                break;
                
            case 4: {  // 注入进程
                char name[128];
                printf("\n  Enter process name or PID: ");
                scanf("%127s", name);
                
                DWORD pid = atoi(name);
                if (pid == 0) {
                    pid = get_pid_by_name(name);
                }
                
                if (pid == 0) {
                    set_color(COLOR_RED);
                    printf("  [ERROR] Process not found\n");
                } else {
                    set_color(COLOR_YELLOW);
                    printf("  Injecting into PID %d...\n", pid);
                    if (hwid_inject_process(pid, config) == 0) {
                        set_color(COLOR_GREEN);
                        printf("  [OK] Injected successfully!\n");
                    } else {
                        set_color(COLOR_RED);
                        printf("  [ERROR] Injection failed\n");
                    }
                }
                set_color(COLOR_DEFAULT);
                break;
            }
                
            case 5:  // 显示真实硬件信息
                show_real_hwid();
                break;
                
            case 6: {  // 加载配置
                char path[256];
                printf("\n  Enter config file path: ");
                scanf("%255s", path);
                if (hwid_load_config(path, config) == 0) {
                    set_color(COLOR_GREEN);
                    printf("  [OK] Config loaded!\n");
                } else {
                    set_color(COLOR_RED);
                    printf("  [ERROR] Failed to load config\n");
                }
                set_color(COLOR_DEFAULT);
                break;
            }
                
            case 7: {  // 保存配置
                char path[256];
                printf("\n  Enter output file path [hwid.cfg]: ");
                if (scanf("%255s", path) != 1 || path[0] == '\n') {
                    strcpy(path, "hwid.cfg");
                }
                if (hwid_save_config(path, config) == 0) {
                    set_color(COLOR_GREEN);
                    printf("  [OK] Config saved to %s\n", path);
                } else {
                    set_color(COLOR_RED);
                    printf("  [ERROR] Failed to save config\n");
                }
                set_color(COLOR_DEFAULT);
                break;
            }
                
            case 8: {
                printf("\n  MAC override / recovery - Windows Sandbox only.\n");
                printf("  [A] Apply  [R] Restore original  [T] Test and restore  [S] Status\n");
                printf("  Choose action: ");
                int action=_getch();
                printf("%c\n",action);
                const char* operation=NULL;
                if (action=='A' || action=='a') operation="-Action Apply";
                else if (action=='R' || action=='r') operation="-Action Restore";
                else if (action=='T' || action=='t') operation="-Action Test";
                else if (action=='S' || action=='s') operation="-Action Status";
                if (operation && run_companion_script("MacOverride.ps1",operation)!=0)
                    fprintf(stderr,"  [ERROR] MAC action failed. Check backup/driver support.\n");
                break;
            }

            case 9: {  // 注册表修改
                set_color(COLOR_YELLOW);
                printf("\n  === REGISTRY MODIFICATION ===\n");
                printf("  This will permanently modify system registry values.\n");
                printf("  Affects: MachineGuid, ProductId, ComputerName registry values ONLY\n\n");
                set_color(COLOR_DEFAULT);
                
                printf("  Continue? [Y/N]: ");
                int confirm = _getch();
                printf("%c\n", confirm);
                
                if (confirm == 'Y' || confirm == 'y') {
                    set_color(COLOR_YELLOW);
                    printf("\n  Applying registry modifications...\n");
                    set_color(COLOR_DEFAULT);
                    
                    if (hwid_apply_registry_global(config) == 0) {
                        set_color(COLOR_GREEN);
                        printf("  [OK] Registry modified!\n");
                        printf("  A backup was saved. Use --restore-registry to revert.\n");
                    } else {
                        set_color(COLOR_RED);
                        printf("  [ERROR] Failed to modify registry\n");
                        printf("  Make sure you run as Administrator.\n");
                    }
                }
                set_color(COLOR_DEFAULT);
                break;
            }
                
            case 0:  // 退出
                printf("\n  Goodbye!\n\n");
                return 0;
                
            default:
                set_color(COLOR_RED);
                printf("\n  Invalid choice!\n");
                set_color(COLOR_DEFAULT);
                break;
        }
        
        printf("\n  Press any key to continue...");
        _getch();
        printf("\n");
    }
    
    return 0;
}

// 进程注入实现
int hwid_inject_process(DWORD pid, const HwidConfig* config) {
    // 获取当前EXE路径
    char dll_path[MAX_PATH];
    GetModuleFileNameA(NULL, dll_path, MAX_PATH);
    
    // 修改为DLL路径（假设DLL和EXE在同一目录）
    char* last_slash = strrchr(dll_path, '\\');
    if (last_slash) {
        strcpy(last_slash + 1, "HWIDChanger.dll");
    }
    
    // 检查DLL是否存在
    if (GetFileAttributesA(dll_path) == INVALID_FILE_ATTRIBUTES) {
        // 如果没有单独的DLL，使用驱动模式
        return hwid_load_driver() == 0 ? hwid_driver_set_config(config) : -1;
    }
    
    // 打开目标进程
    HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | 
                                  PROCESS_VM_WRITE | PROCESS_VM_READ | 
                                  PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) return -1;
    
    // 分配内存
    size_t path_size = strlen(dll_path) + 1;
    LPVOID pRemote = VirtualAllocEx(hProcess, NULL, path_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemote) {
        CloseHandle(hProcess);
        return -1;
    }
    
    // 写入DLL路径
    WriteProcessMemory(hProcess, pRemote, dll_path, path_size, NULL);
    
    // 获取LoadLibraryA地址
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    LPVOID pLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryA");
    
    // 创建远程线程
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, 
                                        (LPTHREAD_START_ROUTINE)pLoadLibrary,
                                        pRemote, 0, NULL);
    if (!hThread) {
        VirtualFreeEx(hProcess, pRemote, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return -1;
    }
    
    // 等待完成
    WaitForSingleObject(hThread, 5000);
    
    // 清理
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemote, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    
    return 0;
}

int hwid_inject_by_name(const char* process_name, const HwidConfig* config) {
    DWORD pid = get_pid_by_name(process_name);
    if (pid == 0) return -1;
    return hwid_inject_process(pid, config);
}

// 检查管理员权限
static int is_admin(void) {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

// 主函数
int main(int argc, char* argv[]) {
    // 初始化
    hwid_init();
    
    // 检查管理员权限
    if (!is_admin()) {
        set_color(COLOR_YELLOW);
        printf("\n  [!] Warning: Not running as Administrator.\n");
        printf("      Some features may not work properly.\n\n");
        set_color(COLOR_DEFAULT);
    }
    
    // 配置
    HwidConfig config = {0};
    hwid_generate_random(&config);
    
    int result;
    
    if (argc > 1) {
        // 命令行模式
        result = run_cli(argc, argv, &config);
    } else {
        // 交互模式
        result = run_interactive(&config);
    }
    
    // 清理
    hwid_cleanup();
    
    return result;
}

