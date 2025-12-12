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
    DWORD type = REG_SZ;
    
    if (RegOpenKeyExA(root, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return -1;
    
    LONG ret = RegQueryValueExA(hKey, name, NULL, &type, (LPBYTE)buf, &size);
    RegCloseKey(hKey);
    
    return (ret == ERROR_SUCCESS) ? 0 : -1;
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

// 备份当前注册表值
int hwid_backup_registry(void)
{
    if (g_backup.has_backup)
        return 0;  // 已有备份
    
    // 备份 MachineGuid
    reg_read_string(HKEY_LOCAL_MACHINE, REG_MACHINE_GUID, "MachineGuid",
                    g_backup.machine_guid, sizeof(g_backup.machine_guid));
    
    // 备份 ProductId
    reg_read_string(HKEY_LOCAL_MACHINE, REG_PRODUCT_ID, "ProductId",
                    g_backup.product_id, sizeof(g_backup.product_id));
    
    // 备份 ComputerName
    reg_read_string(HKEY_LOCAL_MACHINE, REG_COMPUTER_NAME, "ComputerName",
                    g_backup.computer_name, sizeof(g_backup.computer_name));
    
    g_backup.has_backup = TRUE;
    
    printf("  Registry values backed up\n");
    return 0;
}

// 恢复注册表值
int hwid_restore_registry(void)
{
    // 如果内存中没有备份，尝试从文件加载
    if (!g_backup.has_backup) {
        if (hwid_load_backup(BACKUP_FILE) != 0) {
            printf("  No backup found (neither in memory nor in file)\n");
            return -1;
        }
        printf("  Loaded backup from file\n");
    }
    
    if (g_backup.machine_guid[0])
        reg_write_string(HKEY_LOCAL_MACHINE, REG_MACHINE_GUID, "MachineGuid", g_backup.machine_guid);
    
    if (g_backup.product_id[0])
        reg_write_string(HKEY_LOCAL_MACHINE, REG_PRODUCT_ID, "ProductId", g_backup.product_id);
    
    if (g_backup.computer_name[0]) {
        reg_write_string(HKEY_LOCAL_MACHINE, REG_COMPUTER_NAME, "ComputerName", g_backup.computer_name);
        reg_write_string(HKEY_LOCAL_MACHINE, REG_ACTIVE_NAME, "ComputerName", g_backup.computer_name);
    }
    
    printf("  Registry values restored\n");
    return 0;
}

// 修改网卡MAC地址（注册表级别）
static int spoof_network_adapters(const uint8_t* mac)
{
    HKEY hClassKey;
    char subkeyPath[256];
    int modified = 0;
    
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_NETWORK_CARDS, 0, KEY_READ, &hClassKey) != ERROR_SUCCESS)
        return -1;
    
    // 遍历所有网卡
    for (DWORD i = 0; ; i++)
    {
        char subkeyName[32];
        DWORD nameLen = sizeof(subkeyName);
        
        if (RegEnumKeyExA(hClassKey, i, subkeyName, &nameLen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;
        
        // 跳过Properties子键
        if (strcmp(subkeyName, "Properties") == 0)
            continue;
        
        snprintf(subkeyPath, sizeof(subkeyPath), "%s\\%s", REG_NETWORK_CARDS, subkeyName);
        
        HKEY hAdapterKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkeyPath, 0, KEY_READ | KEY_SET_VALUE, &hAdapterKey) == ERROR_SUCCESS)
        {
            // 检查是否是物理网卡
            char characteristics[16] = {0};
            DWORD charSize = sizeof(characteristics);
            DWORD charType = REG_DWORD;
            DWORD charValue = 0;
            
            if (RegQueryValueExA(hAdapterKey, "Characteristics", NULL, &charType, 
                (LPBYTE)&charValue, &charSize) == ERROR_SUCCESS)
            {
                // NCF_PHYSICAL = 0x4
                if (charValue & 0x4)
                {
                    // 设置NetworkAddress值
                    char macStr[18];
                    snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X",
                             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                    
                    if (RegSetValueExA(hAdapterKey, "NetworkAddress", 0, REG_SZ,
                        (LPBYTE)macStr, (DWORD)strlen(macStr) + 1) == ERROR_SUCCESS)
                    {
                        modified++;
                    }
                }
            }
            
            RegCloseKey(hAdapterKey);
        }
    }
    
    RegCloseKey(hClassKey);
    return modified;
}

// 应用全局注册表伪装
int hwid_apply_registry_global(const HwidConfig* config)
{
    int success = 0;
    
    // 先备份
    hwid_backup_registry();
    
    // 自动保存备份到文件
    hwid_save_backup(BACKUP_FILE);
    
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
    
    // 4. 修改 HwProfileGuid
    if (config->system_uuid[0]) {
        char hwProfileGuid[48];
        snprintf(hwProfileGuid, sizeof(hwProfileGuid), "{%s}", config->system_uuid);
        if (reg_write_string(HKEY_LOCAL_MACHINE, REG_HWPROFILE,
            "HwProfileGuid", hwProfileGuid) == 0) {
            printf("    [OK] HwProfileGuid modified\n");
            success++;
        }
    }
    
    // 5. 修改网卡MAC地址
    int macModified = spoof_network_adapters(config->mac_address);
    if (macModified > 0) {
        printf("    [OK] Modified %d network adapter(s)\n", macModified);
        success++;
    }
    
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

// 保存备份到文件
int hwid_save_backup(const char* path)
{
    if (!g_backup.has_backup)
        return -1;
    
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    
    fwrite(&g_backup, sizeof(g_backup), 1, f);
    fclose(f);
    
    return 0;
}

// 从文件加载备份
int hwid_load_backup(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    
    fread(&g_backup, sizeof(g_backup), 1, f);
    fclose(f);
    
    return 0;
}

