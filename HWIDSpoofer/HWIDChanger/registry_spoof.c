/*
 * 注册表永久修改模块
 * 
 * 直接修改系统注册表中存储的硬件信息
 * 这会影响所有程序读取到的值
 */

#include "hwid_changer.h"
#include <stdio.h>
#include <string.h>

// 备份文件路径
static const char* BACKUP_FILE = "hwid_backup.dat";

// 注册表路径定义
#define REG_MACHINE_GUID    "SOFTWARE\\Microsoft\\Cryptography"
#define REG_PRODUCT_ID      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"
#define REG_COMPUTER_NAME   "SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName"
#define REG_ACTIVE_NAME     "SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName"
#define REG_HWPROFILE       "SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001"
#define REG_NETWORK_CARDS   "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}"
#define REG_DISK_ENUM       "SYSTEM\\CurrentControlSet\\Enum\\IDE"
#define REG_SCSI_ENUM       "SYSTEM\\CurrentControlSet\\Enum\\SCSI"

// 备份结构
typedef struct {
    char machine_guid[MAX_UUID_LEN];
    char product_id[32];
    char computer_name[16];
    uint8_t mac_addresses[8][6];
    int mac_count;
    BOOL has_backup;
} RegistryBackup;

static RegistryBackup g_backup = {0};

// 读取字符串值
static int reg_read_string(HKEY root, const char* subkey, const char* name, char* buf, DWORD size)
{
    HKEY hKey;
    DWORD type = 0;
    if (!buf || size == 0) return -1;
    memset(buf, 0, size);
    
    if (RegOpenKeyExA(root, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return -1;
    
    LONG ret = RegQueryValueExA(hKey, name, NULL, &type, (LPBYTE)buf, &size);
    RegCloseKey(hKey);
    
    /* Never treat missing, malformed or unterminated registry data as a valid backup. */
    return (ret == ERROR_SUCCESS && type == REG_SZ && memchr(buf, '\0', size)) ? 0 : -1;
}

// 写入字符串值
static int reg_write_string(HKEY root, const char* subkey, const char* name, const char* value)
{
    HKEY hKey;
    
    if (RegOpenKeyExA(root, subkey, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return -1;
    
    LONG ret = RegSetValueExA(hKey, name, 0, REG_SZ, (LPBYTE)value, (DWORD)strlen(value) + 1);
    RegCloseKey(hKey);
    
    return (ret == ERROR_SUCCESS) ? 0 : -1;
}

// 写入二进制值
static int reg_write_binary(HKEY root, const char* subkey, const char* name, const void* data, DWORD size)
{
    HKEY hKey;
    
    if (RegOpenKeyExA(root, subkey, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return -1;
    
    LONG ret = RegSetValueExA(hKey, name, 0, REG_BINARY, (LPBYTE)data, size);
    RegCloseKey(hKey);
    
    return (ret == ERROR_SUCCESS) ? 0 : -1;
}

// Snapshot is deliberately limited to the values that safe global mode writes.
// Older releases also modified HwProfileGuid and adapter NetworkAddress values
// without saving their originals. Safe global mode no longer writes those keys.
int hwid_backup_registry(void)
{
    if (g_backup.has_backup)
        return 0;

    FILE* existing = fopen(BACKUP_FILE, "rb");
    if (existing) {
        fclose(existing);
        if (hwid_load_backup(BACKUP_FILE) != 0) {
            fprintf(stderr, "  [ERROR] Backup exists but is corrupt/incompatible; refusing to overwrite it.\n");
            return -1;
        }
        printf("  Loaded existing backup (preserving original values)\n");
        return 0;
    }

    /* A missing backup is only safe to recreate when all original values can
       be read. Never snapshot already-changed values after a read failure. */
    RegistryBackup snapshot = {0};
    if (reg_read_string(HKEY_LOCAL_MACHINE, REG_MACHINE_GUID, "MachineGuid",
                        snapshot.machine_guid, sizeof(snapshot.machine_guid)) != 0 ||
        reg_read_string(HKEY_LOCAL_MACHINE, REG_PRODUCT_ID, "ProductId",
                        snapshot.product_id, sizeof(snapshot.product_id)) != 0 ||
        reg_read_string(HKEY_LOCAL_MACHINE, REG_COMPUTER_NAME, "ComputerName",
                        snapshot.computer_name, sizeof(snapshot.computer_name)) != 0) {
        fprintf(stderr, "  [ERROR] Cannot read all original registry values. No changes made.\n");
        return -1;
    }

    snapshot.has_backup = TRUE;
    g_backup = snapshot;
    printf("  Original registry values captured in memory\n");
    return 0;
}

int hwid_restore_registry(void)
{
    if (!g_backup.has_backup) {
        if (hwid_load_backup(BACKUP_FILE) != 0) {
            fprintf(stderr, "  [ERROR] No valid registry backup available.\n");
            return -1;
        }
    }

    int failures = 0;
    if (reg_write_string(HKEY_LOCAL_MACHINE, REG_MACHINE_GUID,
                         "MachineGuid", g_backup.machine_guid) != 0) {
        fprintf(stderr, "  [FAIL] MachineGuid restore\n");
        failures++;
    }
    if (reg_write_string(HKEY_LOCAL_MACHINE, REG_PRODUCT_ID,
                         "ProductId", g_backup.product_id) != 0) {
        fprintf(stderr, "  [FAIL] ProductId restore\n");
        failures++;
    }
    if (reg_write_string(HKEY_LOCAL_MACHINE, REG_COMPUTER_NAME,
                         "ComputerName", g_backup.computer_name) != 0) {
        fprintf(stderr, "  [FAIL] ComputerName restore\n");
        failures++;
    }
    if (reg_write_string(HKEY_LOCAL_MACHINE, REG_ACTIVE_NAME,
                         "ComputerName", g_backup.computer_name) != 0) {
        fprintf(stderr, "  [FAIL] ActiveComputerName restore\n");
        failures++;
    }

    if (failures) {
        fprintf(stderr, "  [ERROR] Registry restore incomplete: %d write(s) failed.\n", failures);
        return -1;
    }
    printf("  Registry values restored (reboot may be required)\n");
    return 0;
}

// 应用全局注册表伪装
int hwid_apply_registry_global(const HwidConfig* config)
{
    int success = 0;
    
    /* Fail closed: no permanent writes unless a validated, durable backup exists. */
    if (hwid_backup_registry() != 0 || hwid_save_backup(BACKUP_FILE) != 0) {
        fprintf(stderr, "  [ERROR] Could not securely preserve originals; refusing registry changes.\n");
        return -1;
    }
    
    printf("  Applying global registry spoof...\n");
    
    // 1. 修改 MachineGuid
    if (config->machine_guid[0]) {
        if (reg_write_string(HKEY_LOCAL_MACHINE, REG_MACHINE_GUID, 
            "MachineGuid", config->machine_guid) == 0) {
            printf("    [OK] MachineGuid modified\n");
            success++;
        } else {
            printf("    [FAIL] MachineGuid (need admin)\n");
        }
    }
    
    // 2. 修改 ProductId
    if (config->product_id[0]) {
        if (reg_write_string(HKEY_LOCAL_MACHINE, REG_PRODUCT_ID,
            "ProductId", config->product_id) == 0) {
            printf("    [OK] ProductId modified\n");
            success++;
        }
    }
    
    // 3. 修改 ComputerName
    if (config->computer_name[0]) {
        if (reg_write_string(HKEY_LOCAL_MACHINE, REG_COMPUTER_NAME,
            "ComputerName", config->computer_name) == 0) {
            reg_write_string(HKEY_LOCAL_MACHINE, REG_ACTIVE_NAME,
                "ComputerName", config->computer_name);
            printf("    [OK] ComputerName modified\n");
            success++;
        }
    }
    
    /* Previously modified HwProfileGuid and network adapter addresses without
       saving their original values. Until a per-key backup format is added,
       do not perform these irreversible-by-this-tool writes. */
    printf("    [SKIP] HwProfileGuid / adapter MAC registry changes (not safely backed up)\n");
    
    printf("  Applied %d registry modifications\n", success);
    
    return success > 0 ? 0 : -1;
}

// 获取当前注册表中的硬件信息
int hwid_get_registry_info(HwidConfig* config)
{
    reg_read_string(HKEY_LOCAL_MACHINE, REG_MACHINE_GUID, "MachineGuid",
                    config->machine_guid, sizeof(config->machine_guid));
    
    reg_read_string(HKEY_LOCAL_MACHINE, REG_PRODUCT_ID, "ProductId",
                    config->product_id, sizeof(config->product_id));
    
    reg_read_string(HKEY_LOCAL_MACHINE, REG_COMPUTER_NAME, "ComputerName",
                    config->computer_name, sizeof(config->computer_name));
    
    return 0;
}

// Do not overwrite the only copy of original identifiers.
// Use an exclusive temporary file and an atomic no-replace rename.
int hwid_save_backup(const char* path)
{
    if (!path || !*path || !g_backup.has_backup)
        return -1;

    DWORD attrs = GetFileAttributesA(path);
    if (attrs != INVALID_FILE_ATTRIBUTES) {
        /* Existing files must validate; never silently reset originals. */
        RegistryBackup memory_copy = g_backup;
        int valid = hwid_load_backup(path);
        g_backup = memory_copy;
        return valid == 0 ? 0 : -1;
    }
    if (GetLastError() != ERROR_FILE_NOT_FOUND &&
        GetLastError() != ERROR_PATH_NOT_FOUND)
        return -1;

    char temporary[MAX_PATH];
    int count = snprintf(temporary, sizeof(temporary), "%s.tmp.%lu.%lu",
                         path, (unsigned long)GetCurrentProcessId(),
                         (unsigned long)GetTickCount());
    if (count < 0 || count >= (int)sizeof(temporary))
        return -1;

    HANDLE file = CreateFileA(temporary, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return -1;

    DWORD written = 0;
    BOOL ok = WriteFile(file, &g_backup, (DWORD)sizeof(g_backup), &written, NULL) &&
              written == sizeof(g_backup) && FlushFileBuffers(file);
    if (!CloseHandle(file))
        ok = FALSE;

    if (!ok || !MoveFileExA(temporary, path, MOVEFILE_WRITE_THROUGH)) {
        DeleteFileA(temporary);
        return -1;
    }
    return 0;
}

// Strictly validate size, flag and null termination before accepting backup.
int hwid_load_backup(const char* path)
{
    if (!path || !*path)
        return -1;

    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return -1;

    LARGE_INTEGER size;
    RegistryBackup snapshot = {0};
    DWORD read = 0;
    BOOL ok = GetFileSizeEx(file, &size) &&
              size.QuadPart == sizeof(snapshot) &&
              ReadFile(file, &snapshot, (DWORD)sizeof(snapshot), &read, NULL) &&
              read == sizeof(snapshot);
    CloseHandle(file);

    if (!ok || snapshot.has_backup != TRUE ||
        !memchr(snapshot.machine_guid, '\0', sizeof(snapshot.machine_guid)) ||
        !snapshot.machine_guid[0] ||
        !memchr(snapshot.product_id, '\0', sizeof(snapshot.product_id)) ||
        !snapshot.product_id[0] ||
        !memchr(snapshot.computer_name, '\0', sizeof(snapshot.computer_name)) ||
        !snapshot.computer_name[0])
        return -1;

    g_backup = snapshot;
    return 0;
}
