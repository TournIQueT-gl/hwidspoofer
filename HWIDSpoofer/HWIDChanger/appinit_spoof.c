/*
 * AppInit_DLLs 全局注入模块
 * 
 * 通过设置 AppInit_DLLs 注册表项，让 DLL 自动注入到所有加载 user32.dll 的进程
 * 这样可以实现用户态的全系统伪装
 */

#include "hwid_changer.h"
#include <stdio.h>
#include <string.h>

#define REG_APPINIT_32 "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows"
#define REG_APPINIT_64 "SOFTWARE\\WOW6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Windows"

// 备份原始 AppInit 设置
typedef struct {
    char original_dlls[512];
    DWORD original_load_appinit;
    DWORD original_require_signed;
    BOOL has_backup;
} AppInitBackup;

static AppInitBackup g_appinit_backup = {0};

// 读取 DWORD 值
static DWORD reg_read_dword(HKEY root, const char* subkey, const char* name, DWORD default_val) {
    HKEY hKey;
    DWORD value = default_val;
    DWORD size = sizeof(DWORD);
    DWORD type = REG_DWORD;
    
    if (RegOpenKeyExA(root, subkey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, name, NULL, &type, (LPBYTE)&value, &size);
        RegCloseKey(hKey);
    }
    return value;
}

// 写入 DWORD 值
static int reg_write_dword(HKEY root, const char* subkey, const char* name, DWORD value) {
    HKEY hKey;
    if (RegOpenKeyExA(root, subkey, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return -1;
    LONG ret = RegSetValueExA(hKey, name, 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD));
    RegCloseKey(hKey);
    return (ret == ERROR_SUCCESS) ? 0 : -1;
}

// 读取字符串值
static int reg_read_str(HKEY root, const char* subkey, const char* name, char* buf, DWORD size) {
    HKEY hKey;
    DWORD type = REG_SZ;
    if (RegOpenKeyExA(root, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return -1;
    LONG ret = RegQueryValueExA(hKey, name, NULL, &type, (LPBYTE)buf, &size);
    RegCloseKey(hKey);
    return (ret == ERROR_SUCCESS) ? 0 : -1;
}

// 写入字符串值
static int reg_write_str(HKEY root, const char* subkey, const char* name, const char* value) {
    HKEY hKey;
    if (RegOpenKeyExA(root, subkey, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return -1;
    LONG ret = RegSetValueExA(hKey, name, 0, REG_SZ, (LPBYTE)value, (DWORD)strlen(value) + 1);
    RegCloseKey(hKey);
    return (ret == ERROR_SUCCESS) ? 0 : -1;
}

// 备份当前 AppInit 设置
int hwid_backup_appinit(void) {
    if (g_appinit_backup.has_backup)
        return 0;
    
    reg_read_str(HKEY_LOCAL_MACHINE, REG_APPINIT_32, "AppInit_DLLs",
                 g_appinit_backup.original_dlls, sizeof(g_appinit_backup.original_dlls));
    
    g_appinit_backup.original_load_appinit = 
        reg_read_dword(HKEY_LOCAL_MACHINE, REG_APPINIT_32, "LoadAppInit_DLLs", 0);
    
    g_appinit_backup.original_require_signed = 
        reg_read_dword(HKEY_LOCAL_MACHINE, REG_APPINIT_32, "RequireSignedAppInit_DLLs", 1);
    
    g_appinit_backup.has_backup = TRUE;
    return 0;
}

// 启用 AppInit_DLLs 全局注入
int hwid_enable_appinit_injection(const char* dll_path) {
    printf("  Enabling AppInit_DLLs injection...\n");
    
    // 备份
    hwid_backup_appinit();
    
    // 检查 DLL 是否存在
    if (GetFileAttributesA(dll_path) == INVALID_FILE_ATTRIBUTES) {
        printf("    [ERROR] DLL not found: %s\n", dll_path);
        return -1;
    }
    
    // 设置 AppInit_DLLs
    if (reg_write_str(HKEY_LOCAL_MACHINE, REG_APPINIT_32, "AppInit_DLLs", dll_path) != 0) {
        printf("    [ERROR] Failed to set AppInit_DLLs (need admin)\n");
        return -1;
    }
    printf("    [OK] AppInit_DLLs = %s\n", dll_path);
    
    // 启用加载
    if (reg_write_dword(HKEY_LOCAL_MACHINE, REG_APPINIT_32, "LoadAppInit_DLLs", 1) != 0) {
        printf("    [ERROR] Failed to enable LoadAppInit_DLLs\n");
        return -1;
    }
    printf("    [OK] LoadAppInit_DLLs = 1\n");
    
    // 禁用签名要求 (Windows 8+)
    // 注意：这可能需要禁用安全启动才能生效
    if (reg_write_dword(HKEY_LOCAL_MACHINE, REG_APPINIT_32, "RequireSignedAppInit_DLLs", 0) == 0) {
        printf("    [OK] RequireSignedAppInit_DLLs = 0\n");
    } else {
        printf("    [WARN] Could not disable signature requirement\n");
    }
    
    printf("  AppInit injection enabled!\n");
    printf("  Note: New processes will automatically load the spoofer DLL.\n");
    printf("  Note: Existing processes are not affected until restarted.\n");
    
    return 0;
}

// 禁用 AppInit_DLLs 注入
int hwid_disable_appinit_injection(void) {
    printf("  Disabling AppInit_DLLs injection...\n");
    
    // 恢复原始值
    if (g_appinit_backup.has_backup) {
        reg_write_str(HKEY_LOCAL_MACHINE, REG_APPINIT_32, "AppInit_DLLs", 
                      g_appinit_backup.original_dlls);
        reg_write_dword(HKEY_LOCAL_MACHINE, REG_APPINIT_32, "LoadAppInit_DLLs",
                        g_appinit_backup.original_load_appinit);
        reg_write_dword(HKEY_LOCAL_MACHINE, REG_APPINIT_32, "RequireSignedAppInit_DLLs",
                        g_appinit_backup.original_require_signed);
        printf("    [OK] Restored original AppInit settings\n");
    } else {
        // 没有备份，直接禁用
        reg_write_str(HKEY_LOCAL_MACHINE, REG_APPINIT_32, "AppInit_DLLs", "");
        reg_write_dword(HKEY_LOCAL_MACHINE, REG_APPINIT_32, "LoadAppInit_DLLs", 0);
        printf("    [OK] AppInit injection disabled\n");
    }
    
    return 0;
}

// 检查 AppInit 注入状态
int hwid_is_appinit_enabled(void) {
    DWORD enabled = reg_read_dword(HKEY_LOCAL_MACHINE, REG_APPINIT_32, "LoadAppInit_DLLs", 0);
    return enabled != 0;
}

// 获取当前 AppInit_DLLs 路径
int hwid_get_appinit_dlls(char* buf, int len) {
    return reg_read_str(HKEY_LOCAL_MACHINE, REG_APPINIT_32, "AppInit_DLLs", buf, len);
}

