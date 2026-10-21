/*
 * HWID Changer - 单文件硬件信息伪装工具
 * 精简高效版本
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 版本信息
#define HWID_VERSION        "2.0"
#define HWID_BUILD_DATE     __DATE__

// 配置选项
#define MAX_SERIAL_LEN      64
#define MAX_STRING_LEN      128
#define MAX_UUID_LEN        40
#define MAX_MAC_LEN         18

// 运行模式
typedef enum {
    MODE_AUTO,          // 自动选择最佳模式
    MODE_USERMODE,      // 仅用户态Hook（单进程）
    MODE_KERNEL,        // 仅内核驱动
    MODE_HYBRID,        // 混合模式
    MODE_GLOBAL,        // 全局模式（影响所有进程）
} RunMode;

// 硬件配置结构（紧凑）
#pragma pack(push, 1)
typedef struct {
    // 标志位
    uint32_t flags;
    
    // 磁盘
    char disk_serial[MAX_SERIAL_LEN];
    char disk_model[MAX_SERIAL_LEN];
    uint32_t volume_serial;
    
    // SMBIOS/BIOS
    char bios_serial[MAX_SERIAL_LEN];
    char bios_vendor[MAX_STRING_LEN];
    char bios_version[MAX_SERIAL_LEN];
    char bios_date[16];
    
    // 主板
    char board_serial[MAX_SERIAL_LEN];
    char board_product[MAX_STRING_LEN];
    char board_vendor[MAX_STRING_LEN];
    
    // 系统
    char system_uuid[MAX_UUID_LEN];
    char machine_guid[MAX_UUID_LEN];
    char product_id[32];
    char computer_name[16];
    
    // 网络
    uint8_t mac_address[6];
    uint8_t mac_address2[6];  // 备用网卡
    
    // CPU
    char cpu_id[24];
    
    // GPU
    char gpu_serial[MAX_SERIAL_LEN];
    
} HwidConfig;
#pragma pack(pop)

// 配置标志
#define FLAG_SPOOF_DISK     (1 << 0)
#define FLAG_SPOOF_BIOS     (1 << 1)
#define FLAG_SPOOF_BOARD    (1 << 2)
#define FLAG_SPOOF_SYSTEM   (1 << 3)
#define FLAG_SPOOF_NETWORK  (1 << 4)
#define FLAG_SPOOF_CPU      (1 << 5)
#define FLAG_SPOOF_GPU      (1 << 6)
#define FLAG_SPOOF_WMI      (1 << 7)
#define FLAG_SPOOF_REGISTRY (1 << 8)
#define FLAG_SPOOF_SMBIOS   (1 << 9)
#define FLAG_RANDOMIZE      (1 << 15)
#define FLAG_ALL            0xFFFF

// 核心API
int hwid_init(void);
void hwid_cleanup(void);

// 配置管理
void hwid_generate_random(HwidConfig* config);
int hwid_load_config(const char* path, HwidConfig* config);
int hwid_save_config(const char* path, const HwidConfig* config);

// 应用伪装
int hwid_apply(const HwidConfig* config, RunMode mode);
int hwid_revert(void);

// 进程注入
int hwid_inject_process(DWORD pid, const HwidConfig* config);
int hwid_inject_by_name(const char* process_name, const HwidConfig* config);

// 驱动操作
int hwid_load_driver(void);
int hwid_unload_driver(void);
int hwid_driver_set_config(const HwidConfig* config);

// 全局伪装模式（影响所有进程）
int hwid_enable_global_mode(const HwidConfig* config);
int hwid_disable_global_mode(void);
int hwid_is_global_mode_active(void);

// 注册表级别修改（永久性）
int hwid_apply_registry_global(const HwidConfig* config);
int hwid_backup_registry(void);
int hwid_restore_registry(void);
int hwid_save_backup(const char* path);
int hwid_load_backup(const char* path);

// AppInit_DLLs 全局注入（用户态全系统伪装）
int hwid_enable_appinit_injection(const char* dll_path);
int hwid_disable_appinit_injection(void);
int hwid_is_appinit_enabled(void);
int hwid_get_appinit_dlls(char* buf, int len);

// 工具函数
void hwid_random_serial(char* buf, int len);
void hwid_random_uuid(char* buf);
void hwid_random_mac(uint8_t* mac);
uint32_t hwid_random_volume_serial(void);

// 信息获取（用于测试）
int hwid_get_real_disk_serial(char* buf, int len);
int hwid_get_real_mac(uint8_t* mac);
int hwid_get_real_bios_serial(char* buf, int len);
int hwid_get_real_board_serial(char* buf, int len);
int hwid_get_real_uuid(char* buf, int len);

#ifdef __cplusplus
}
#endif

