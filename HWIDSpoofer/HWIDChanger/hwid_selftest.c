/*
 * HWID Changer 完整自测试程序
 * 
 * 测试场景：
 * 1. 用户态Hook - 在当前进程内应用Hook后验证API返回值
 * 2. 配置加载 - 验证配置文件被正确加载
 * 3. 每个API的Hook效果
 * 4. 恢复功能
 */

#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <windows.h>
#include <winioctl.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <string.h>
#include "hwid_changer.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "advapi32.lib")

// 测试结果统计
static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_START(name) \
    printf("\n[TEST %d] %s\n", ++g_tests_run, name); \
    printf("  ----------------------------------------\n");

#define TEST_PASS(msg) do { \
    printf("  [PASS] %s\n", msg); \
    g_tests_passed++; \
} while(0)

#define TEST_FAIL(msg) do { \
    printf("  [FAIL] %s\n", msg); \
    g_tests_failed++; \
} while(0)

#define TEST_CHECK(cond, pass_msg, fail_msg) do { \
    if (cond) { TEST_PASS(pass_msg); } \
    else { TEST_FAIL(fail_msg); } \
} while(0)

// ============== 辅助函数 ==============

// 获取卷序列号
DWORD get_volume_serial(void) {
    DWORD serial = 0;
    GetVolumeInformationW(L"C:\\", NULL, 0, &serial, NULL, NULL, NULL, 0);
    return serial;
}

// 获取磁盘序列号
int get_disk_serial(char* buf, int len) {
    HANDLE hDisk = CreateFileW(L"\\\\.\\PhysicalDrive0",
        0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDisk == INVALID_HANDLE_VALUE) return -1;
    
    STORAGE_PROPERTY_QUERY query = {0};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    
    BYTE buffer[1024] = {0};
    DWORD ret = 0;
    
    if (DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY,
        &query, sizeof(query), buffer, sizeof(buffer), &ret, NULL)) {
        STORAGE_DEVICE_DESCRIPTOR* desc = (STORAGE_DEVICE_DESCRIPTOR*)buffer;
        if (desc->SerialNumberOffset && desc->SerialNumberOffset < ret) {
            strncpy(buf, (char*)buffer + desc->SerialNumberOffset, len - 1);
            buf[len-1] = 0;
            CloseHandle(hDisk);
            return 0;
        }
    }
    CloseHandle(hDisk);
    return -1;
}

// 获取MAC地址
int get_mac_address(BYTE* mac) {
    IP_ADAPTER_INFO adapters[16];
    ULONG bufLen = sizeof(adapters);
    if (GetAdaptersInfo(adapters, &bufLen) == ERROR_SUCCESS) {
        if (adapters[0].AddressLength == 6) {
            memcpy(mac, adapters[0].Address, 6);
            return 0;
        }
    }
    return -1;
}

// 获取MachineGuid
int get_machine_guid(char* buf, int len) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = len;
        DWORD type = REG_SZ;
        if (RegQueryValueExA(hKey, "MachineGuid", NULL, &type, (LPBYTE)buf, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return 0;
        }
        RegCloseKey(hKey);
    }
    return -1;
}

// ============== 测试函数 ==============

// 测试1: 配置生成
void test_config_generation(void) {
    TEST_START("Config Generation");
    
    HwidConfig config = {0};
    hwid_generate_random(&config);
    
    TEST_CHECK(config.disk_serial[0] != 0, 
        "Disk serial generated", "Disk serial empty");
    TEST_CHECK(config.system_uuid[0] != 0,
        "System UUID generated", "System UUID empty");
    TEST_CHECK(config.machine_guid[0] != 0,
        "Machine GUID generated", "Machine GUID empty");
    TEST_CHECK(config.mac_address[0] != 0 || config.mac_address[1] != 0,
        "MAC address generated", "MAC address empty");
    TEST_CHECK(config.volume_serial != 0,
        "Volume serial generated", "Volume serial is zero");
    TEST_CHECK(config.flags == FLAG_ALL,
        "Flags set to ALL", "Flags not set correctly");
    
    printf("  Generated values:\n");
    printf("    Disk Serial:  %s\n", config.disk_serial);
    printf("    Volume Serial: 0x%08X\n", config.volume_serial);
    printf("    System UUID:  %s\n", config.system_uuid);
    printf("    Machine GUID: %s\n", config.machine_guid);
    printf("    MAC Address:  %02X:%02X:%02X:%02X:%02X:%02X\n",
        config.mac_address[0], config.mac_address[1], config.mac_address[2],
        config.mac_address[3], config.mac_address[4], config.mac_address[5]);
}

// 测试2: 配置保存和加载
void test_config_save_load(void) {
    TEST_START("Config Save/Load");
    
    HwidConfig original = {0};
    hwid_generate_random(&original);
    
    // 保存
    int save_result = hwid_save_config("_selftest_config.cfg", &original);
    TEST_CHECK(save_result == 0, "Config saved successfully", "Config save failed");
    
    // 加载
    HwidConfig loaded = {0};
    int load_result = hwid_load_config("_selftest_config.cfg", &loaded);
    TEST_CHECK(load_result == 0, "Config loaded successfully", "Config load failed");
    
    // 比较
    TEST_CHECK(strcmp(original.disk_serial, loaded.disk_serial) == 0,
        "Disk serial matches", "Disk serial mismatch");
    TEST_CHECK(strcmp(original.system_uuid, loaded.system_uuid) == 0,
        "System UUID matches", "System UUID mismatch");
    TEST_CHECK(original.volume_serial == loaded.volume_serial,
        "Volume serial matches", "Volume serial mismatch");
    TEST_CHECK(memcmp(original.mac_address, loaded.mac_address, 6) == 0,
        "MAC address matches", "MAC address mismatch");
    
    // 清理
    DeleteFileA("_selftest_config.cfg");
}

// 测试3: 用户态Hook - 卷序列号
void test_hook_volume_serial(HwidConfig* config) {
    TEST_START("Hook - Volume Serial (GetVolumeInformation)");
    
    // 获取原始值
    DWORD original = get_volume_serial();
    printf("  Original Volume Serial: 0x%08X\n", original);
    printf("  Config Volume Serial:   0x%08X\n", config->volume_serial);
    
    // 应用Hook
    printf("  Applying usermode hooks...\n");
    hwid_apply(config, MODE_USERMODE);
    
    // 获取Hook后的值
    DWORD hooked = get_volume_serial();
    printf("  After Hook:             0x%08X\n", hooked);
    
    TEST_CHECK(hooked == config->volume_serial,
        "Volume serial spoofed correctly",
        "Volume serial NOT spoofed (hook may have failed)");
    
    // 恢复
    hwid_revert();
    DWORD reverted = get_volume_serial();
    printf("  After Revert:           0x%08X\n", reverted);
    
    TEST_CHECK(reverted == original,
        "Volume serial restored correctly",
        "Volume serial NOT restored");
}

// 测试4: 用户态Hook - 磁盘序列号
void test_hook_disk_serial(HwidConfig* config) {
    TEST_START("Hook - Disk Serial (DeviceIoControl)");
    
    char original[64] = {0};
    get_disk_serial(original, sizeof(original));
    printf("  Original Disk Serial: %s\n", original);
    printf("  Config Disk Serial:   %s\n", config->disk_serial);
    
    // 应用Hook
    printf("  Applying usermode hooks...\n");
    hwid_apply(config, MODE_USERMODE);
    
    char hooked[64] = {0};
    get_disk_serial(hooked, sizeof(hooked));
    printf("  After Hook:           %s\n", hooked);
    
    // 检查是否包含配置的序列号（可能有前缀/后缀）
    int matched = (strstr(hooked, config->disk_serial) != NULL) ||
                  (strncmp(hooked, config->disk_serial, strlen(config->disk_serial)) == 0);
    
    TEST_CHECK(matched || strcmp(hooked, original) != 0,
        "Disk serial modified by hook",
        "Disk serial NOT modified (hook may have failed)");
    
    hwid_revert();
}

// 测试5: 用户态Hook - MAC地址
void test_hook_mac_address(HwidConfig* config) {
    TEST_START("Hook - MAC Address (GetAdaptersInfo)");
    
    BYTE original[6] = {0};
    get_mac_address(original);
    printf("  Original MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
        original[0], original[1], original[2], original[3], original[4], original[5]);
    printf("  Config MAC:   %02X:%02X:%02X:%02X:%02X:%02X\n",
        config->mac_address[0], config->mac_address[1], config->mac_address[2],
        config->mac_address[3], config->mac_address[4], config->mac_address[5]);
    
    printf("  Applying usermode hooks...\n");
    hwid_apply(config, MODE_USERMODE);
    
    BYTE hooked[6] = {0};
    get_mac_address(hooked);
    printf("  After Hook:   %02X:%02X:%02X:%02X:%02X:%02X\n",
        hooked[0], hooked[1], hooked[2], hooked[3], hooked[4], hooked[5]);
    
    TEST_CHECK(memcmp(hooked, config->mac_address, 6) == 0,
        "MAC address spoofed correctly",
        "MAC address NOT spoofed (hook may have failed)");
    
    hwid_revert();
    
    BYTE reverted[6] = {0};
    get_mac_address(reverted);
    printf("  After Revert: %02X:%02X:%02X:%02X:%02X:%02X\n",
        reverted[0], reverted[1], reverted[2], reverted[3], reverted[4], reverted[5]);
    
    TEST_CHECK(memcmp(reverted, original, 6) == 0,
        "MAC address restored correctly",
        "MAC address NOT restored");
}

// 测试6: 用户态Hook - 注册表 (MachineGuid)
void test_hook_registry(HwidConfig* config) {
    TEST_START("Hook - Registry (RegQueryValueEx)");
    
    char original[64] = {0};
    get_machine_guid(original, sizeof(original));
    printf("  Original MachineGuid: %s\n", original);
    printf("  Config MachineGuid:   %s\n", config->machine_guid);
    
    printf("  Applying usermode hooks...\n");
    hwid_apply(config, MODE_USERMODE);
    
    char hooked[64] = {0};
    get_machine_guid(hooked, sizeof(hooked));
    printf("  After Hook:           %s\n", hooked);
    
    TEST_CHECK(strcmp(hooked, config->machine_guid) == 0,
        "MachineGuid spoofed correctly",
        "MachineGuid NOT spoofed (hook may have failed)");
    
    hwid_revert();
    
    char reverted[64] = {0};
    get_machine_guid(reverted, sizeof(reverted));
    printf("  After Revert:         %s\n", reverted);
    
    // 注意：注册表Hook恢复后读取的是实际注册表值
    printf("  (Note: Registry reads real value after hook removal)\n");
}

// 测试7: 注册表永久修改
void test_registry_modification(HwidConfig* config) {
    TEST_START("Registry Permanent Modification");
    
    char original[64] = {0};
    get_machine_guid(original, sizeof(original));
    printf("  Original MachineGuid: %s\n", original);
    
    // 备份
    printf("  Backing up registry...\n");
    hwid_backup_registry();
    
    // 修改
    printf("  Applying registry modification...\n");
    int result = hwid_apply_registry_global(config);
    TEST_CHECK(result == 0, "Registry modification succeeded", "Registry modification failed");
    
    // 验证
    char modified[64] = {0};
    get_machine_guid(modified, sizeof(modified));
    printf("  After Modification:   %s\n", modified);
    
    TEST_CHECK(strcmp(modified, config->machine_guid) == 0,
        "Registry value changed to config value",
        "Registry value did NOT change");
    
    // 恢复
    printf("  Restoring registry...\n");
    result = hwid_restore_registry();
    TEST_CHECK(result == 0, "Registry restore succeeded", "Registry restore failed");
    
    char restored[64] = {0};
    get_machine_guid(restored, sizeof(restored));
    printf("  After Restore:        %s\n", restored);
    
    TEST_CHECK(strcmp(restored, original) == 0,
        "Registry value restored to original",
        "Registry value NOT restored");
}

// 测试8: 同时应用多个Hook
void test_multiple_hooks(HwidConfig* config) {
    TEST_START("Multiple Hooks Simultaneously");
    
    // 获取所有原始值
    DWORD orig_vol = get_volume_serial();
    BYTE orig_mac[6] = {0};
    get_mac_address(orig_mac);
    
    printf("  Original values:\n");
    printf("    Volume Serial: 0x%08X\n", orig_vol);
    printf("    MAC Address:   %02X:%02X:%02X:%02X:%02X:%02X\n",
        orig_mac[0], orig_mac[1], orig_mac[2], 
        orig_mac[3], orig_mac[4], orig_mac[5]);
    
    // 应用所有Hook
    printf("  Applying all hooks...\n");
    hwid_apply(config, MODE_USERMODE);
    
    // 验证所有值
    DWORD hooked_vol = get_volume_serial();
    BYTE hooked_mac[6] = {0};
    get_mac_address(hooked_mac);
    
    printf("  Hooked values:\n");
    printf("    Volume Serial: 0x%08X\n", hooked_vol);
    printf("    MAC Address:   %02X:%02X:%02X:%02X:%02X:%02X\n",
        hooked_mac[0], hooked_mac[1], hooked_mac[2],
        hooked_mac[3], hooked_mac[4], hooked_mac[5]);
    
    int vol_ok = (hooked_vol == config->volume_serial);
    int mac_ok = (memcmp(hooked_mac, config->mac_address, 6) == 0);
    
    TEST_CHECK(vol_ok && mac_ok,
        "All hooks working simultaneously",
        "Some hooks not working");
    
    hwid_revert();
}

// 测试9: 边界情况 - 空配置
void test_empty_config(void) {
    TEST_START("Edge Case - Empty Config");
    
    HwidConfig empty = {0};
    
    // 应用空配置不应该崩溃
    printf("  Applying empty config...\n");
    int result = hwid_apply(&empty, MODE_USERMODE);
    
    TEST_CHECK(result == 0, "Empty config handled gracefully", "Crashed on empty config");
    
    hwid_revert();
}

// 测试10: 边界情况 - 多次应用
void test_multiple_apply(HwidConfig* config) {
    TEST_START("Edge Case - Multiple Apply/Revert");
    
    printf("  Applying hooks multiple times...\n");
    
    for (int i = 0; i < 3; i++) {
        hwid_apply(config, MODE_USERMODE);
        printf("    Apply #%d: OK\n", i + 1);
    }
    
    DWORD vol = get_volume_serial();
    TEST_CHECK(vol == config->volume_serial,
        "Hook still works after multiple applies",
        "Hook broken after multiple applies");
    
    for (int i = 0; i < 3; i++) {
        hwid_revert();
        printf("    Revert #%d: OK\n", i + 1);
    }
    
    TEST_PASS("No crash during multiple apply/revert cycles");
}

// ============== 主函数 ==============

int main(int argc, char* argv[]) {
    printf("\n");
    printf("========================================================\n");
    printf("  HWID Changer - Complete Self-Test Suite\n");
    printf("========================================================\n");
    printf("  This test verifies ALL spoof functionality by:\n");
    printf("  1. Applying hooks to the CURRENT process\n");
    printf("  2. Calling system APIs to verify spoofed values\n");
    printf("  3. Testing revert functionality\n");
    printf("========================================================\n");
    
    // 初始化
    hwid_init();
    
    // 生成测试配置
    HwidConfig test_config = {0};
    hwid_generate_random(&test_config);
    
    // ============== 运行测试 ==============
    
    // 基础功能测试
    test_config_generation();
    test_config_save_load();
    
    // 用户态Hook测试
    test_hook_volume_serial(&test_config);
    test_hook_disk_serial(&test_config);
    test_hook_mac_address(&test_config);
    test_hook_registry(&test_config);
    
    // 组合测试
    test_multiple_hooks(&test_config);
    
    // 注册表永久修改测试
    test_registry_modification(&test_config);
    
    // 边界情况测试
    test_empty_config();
    test_multiple_apply(&test_config);
    
    // ============== 测试结果 ==============
    
    printf("\n========================================================\n");
    printf("  TEST RESULTS\n");
    printf("========================================================\n");
    printf("  Total Tests:  %d\n", g_tests_run);
    printf("  Passed:       %d\n", g_tests_passed);
    printf("  Failed:       %d\n", g_tests_failed);
    printf("========================================================\n");
    
    if (g_tests_failed == 0) {
        printf("  [SUCCESS] All tests passed!\n");
    } else {
        printf("  [WARNING] Some tests failed!\n");
    }
    printf("========================================================\n\n");
    
    // 清理
    hwid_cleanup();
    
    return g_tests_failed > 0 ? 1 : 0;
}

