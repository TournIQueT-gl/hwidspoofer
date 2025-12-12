#pragma once

#include <cstdint>
#include <Windows.h>

namespace hwid {

// 驱动设备名
#define DRIVER_DEVICE_NAME L"\\Device\\HWIDSpoofer"
#define DRIVER_SYMBOLIC_LINK L"\\DosDevices\\HWIDSpoofer"
#define DRIVER_USER_LINK L"\\\\.\\HWIDSpoofer"

// 服务名
#define DRIVER_SERVICE_NAME L"HWIDSpoofer"
#define DRIVER_DISPLAY_NAME L"HWID Spoofer Driver"

// 共享内存名
#define SHARED_MEMORY_NAME L"Global\\HWIDSpooferConfig"

// IOCTL定义
#define HWID_IOCTL_BASE 0x8000
#define IOCTL_HWID_SET_CONFIG       CTL_CODE(HWID_IOCTL_BASE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HWID_GET_CONFIG       CTL_CODE(HWID_IOCTL_BASE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HWID_ENABLE           CTL_CODE(HWID_IOCTL_BASE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HWID_DISABLE          CTL_CODE(HWID_IOCTL_BASE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HWID_SET_DISK         CTL_CODE(HWID_IOCTL_BASE, 0x810, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HWID_SET_SMBIOS       CTL_CODE(HWID_IOCTL_BASE, 0x811, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HWID_SET_MAC          CTL_CODE(HWID_IOCTL_BASE, 0x812, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HWID_SET_GPU          CTL_CODE(HWID_IOCTL_BASE, 0x813, METHOD_BUFFERED, FILE_ANY_ACCESS)

// 最大字符串长度
#define MAX_SERIAL_LENGTH 64
#define MAX_NAME_LENGTH 128
#define MAX_UUID_LENGTH 40
#define MAX_MAC_LENGTH 18
#define MAX_DISKS 16
#define MAX_ADAPTERS 16

#pragma pack(push, 1)

// 磁盘伪装配置（用于驱动通信）
struct DiskSpoofConfig {
    uint32_t disk_index;
    char serial_number[MAX_SERIAL_LENGTH];
    char product_id[MAX_SERIAL_LENGTH];
    char vendor_id[MAX_SERIAL_LENGTH];
    char revision[MAX_SERIAL_LENGTH];
    uint32_t volume_serial;
    uint8_t enabled;
};

// SMBIOS伪装配置
struct SmbiosSpoofConfig {
    // BIOS信息 (Type 0)
    char bios_vendor[MAX_NAME_LENGTH];
    char bios_version[MAX_SERIAL_LENGTH];
    char bios_date[MAX_SERIAL_LENGTH];
    
    // 系统信息 (Type 1)
    char system_manufacturer[MAX_NAME_LENGTH];
    char system_product[MAX_NAME_LENGTH];
    char system_version[MAX_SERIAL_LENGTH];
    char system_serial[MAX_SERIAL_LENGTH];
    char system_uuid[MAX_UUID_LENGTH];
    char system_sku[MAX_SERIAL_LENGTH];
    char system_family[MAX_NAME_LENGTH];
    
    // 主板信息 (Type 2)
    char baseboard_manufacturer[MAX_NAME_LENGTH];
    char baseboard_product[MAX_NAME_LENGTH];
    char baseboard_version[MAX_SERIAL_LENGTH];
    char baseboard_serial[MAX_SERIAL_LENGTH];
    char baseboard_asset_tag[MAX_SERIAL_LENGTH];
    
    // 机箱信息 (Type 3)
    char chassis_manufacturer[MAX_NAME_LENGTH];
    char chassis_serial[MAX_SERIAL_LENGTH];
    char chassis_asset_tag[MAX_SERIAL_LENGTH];
    
    uint8_t enabled;
};

// MAC地址伪装配置
struct MacSpoofConfig {
    uint32_t adapter_index;
    char adapter_name[MAX_NAME_LENGTH];
    uint8_t original_mac[6];
    uint8_t spoofed_mac[6];
    uint8_t enabled;
};

// GPU伪装配置
struct GpuSpoofConfig {
    uint32_t gpu_index;
    char serial_number[MAX_SERIAL_LENGTH];
    char uuid[MAX_UUID_LENGTH];
    uint8_t enabled;
};

// 主配置结构（用于驱动通信）
struct DriverConfig {
    uint32_t version;               // 配置版本号
    uint32_t flags;                 // 全局标志
    
    // 各项伪装开关
    uint8_t spoof_disk;
    uint8_t spoof_smbios;
    uint8_t spoof_mac;
    uint8_t spoof_gpu;
    
    // 磁盘配置
    uint32_t disk_count;
    DiskSpoofConfig disks[MAX_DISKS];
    
    // SMBIOS配置
    SmbiosSpoofConfig smbios;
    
    // MAC配置
    uint32_t adapter_count;
    MacSpoofConfig adapters[MAX_ADAPTERS];
    
    // GPU配置
    GpuSpoofConfig gpu;
};

// 状态信息
struct DriverStatus {
    uint8_t is_enabled;
    uint8_t driver_loaded;
    uint32_t hooks_active;
    uint32_t spoof_count;           // 伪装次数统计
    uint64_t last_spoof_time;       // 最后伪装时间
};

#pragma pack(pop)

// 共享配置标志
enum ConfigFlags : uint32_t {
    FLAG_NONE = 0,
    FLAG_RANDOMIZE_ON_BOOT = 1 << 0,    // 每次启动随机化
    FLAG_LOG_ORIGINAL = 1 << 1,          // 记录原始值
    FLAG_SILENT_MODE = 1 << 2,           // 静默模式
    FLAG_DRIVER_MODE = 1 << 3,           // 驱动模式
    FLAG_USERMODE_HOOK = 1 << 4,         // 用户态Hook模式
};

// 驱动通信辅助类
class DriverComm {
public:
    DriverComm();
    ~DriverComm();
    
    // 检查驱动是否已加载
    bool is_driver_loaded();
    
    // 加载/卸载驱动
    bool load_driver(const wchar_t* driver_path);
    bool unload_driver();
    
    // 打开驱动设备
    bool open();
    void close();
    bool is_open() const { return m_handle != INVALID_HANDLE_VALUE; }
    
    // 发送配置到驱动
    bool set_config(const DriverConfig& config);
    bool get_config(DriverConfig& config);
    
    // 启用/禁用伪装
    bool enable_spoofing();
    bool disable_spoofing();
    
    // 设置单项配置
    bool set_disk_spoof(const DiskSpoofConfig& config);
    bool set_smbios_spoof(const SmbiosSpoofConfig& config);
    bool set_mac_spoof(const MacSpoofConfig& config);
    bool set_gpu_spoof(const GpuSpoofConfig& config);
    
    // 获取状态
    bool get_status(DriverStatus& status);

private:
    HANDLE m_handle = INVALID_HANDLE_VALUE;
    
    bool send_ioctl(DWORD ioctl_code, void* in_buffer, DWORD in_size, 
                    void* out_buffer = nullptr, DWORD out_size = 0);
};

} // namespace hwid

