#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace hwid {

// 硬件ID类型枚举
enum class HwidType {
    // 磁盘相关
    DISK_SERIAL,           // 硬盘序列号
    DISK_VOLUME_ID,        // 卷序列号
    DISK_PRODUCT_ID,       // 磁盘产品ID
    
    // CPU相关
    CPU_ID,                // CPUID
    CPU_SERIAL,            // CPU序列号
    CPU_BRAND,             // CPU品牌字符串
    
    // 主板相关
    BASEBOARD_SERIAL,      // 主板序列号
    BASEBOARD_PRODUCT,     // 主板产品名
    BASEBOARD_MANUFACTURER,// 主板制造商
    
    // BIOS相关
    BIOS_SERIAL,           // BIOS序列号
    BIOS_VERSION,          // BIOS版本
    BIOS_MANUFACTURER,     // BIOS制造商
    BIOS_RELEASE_DATE,     // BIOS发布日期
    
    // 系统相关
    SYSTEM_UUID,           // 系统UUID
    MACHINE_GUID,          // Windows机器GUID
    PRODUCT_ID,            // Windows产品ID
    COMPUTER_NAME,         // 计算机名
    
    // 网络相关
    MAC_ADDRESS,           // MAC地址
    
    // 内存相关
    MEMORY_SERIAL,         // 内存条序列号
    
    // GPU相关
    GPU_SERIAL,            // 显卡序列号
    GPU_UUID,              // 显卡UUID
    
    // TPM相关
    TPM_ID,                // TPM标识
    
    // SMBIOS相关
    SMBIOS_UUID,           // SMBIOS UUID
    
    MAX_TYPE
};

// 硬件信息结构
struct HwidInfo {
    HwidType type;
    std::string original_value;
    std::string spoofed_value;
    bool enabled;
};

// 磁盘信息
struct DiskInfo {
    std::string serial_number;      // 序列号
    std::string product_id;         // 产品ID
    std::string vendor_id;          // 厂商ID
    std::string revision;           // 固件版本
    uint32_t volume_serial;         // 卷序列号
};

// CPU信息
struct CpuInfo {
    std::string processor_id;       // 处理器ID
    std::string serial_number;      // 序列号（部分CPU支持）
    std::string brand_string;       // 品牌字符串
    uint32_t cpuid_eax;
    uint32_t cpuid_ebx;
    uint32_t cpuid_ecx;
    uint32_t cpuid_edx;
};

// 主板信息
struct BaseboardInfo {
    std::string serial_number;
    std::string product;
    std::string manufacturer;
    std::string version;
};

// BIOS信息
struct BiosInfo {
    std::string serial_number;
    std::string version;
    std::string manufacturer;
    std::string release_date;
    std::string sm_bios_version;
};

// 系统信息
struct SystemInfo {
    std::string uuid;               // 系统UUID
    std::string machine_guid;       // MachineGuid
    std::string product_id;         // Windows产品ID
    std::string computer_name;      // 计算机名
    std::string computer_hw_id;     // 计算机硬件ID
};

// 网络适配器信息
struct NetworkAdapterInfo {
    std::string adapter_name;
    std::string description;
    uint8_t mac_address[6];
    std::string mac_string;
    bool is_physical;
};

// SMBIOS表类型
enum class SmbiosTableType : uint8_t {
    BIOS_INFO = 0,
    SYSTEM_INFO = 1,
    BASEBOARD_INFO = 2,
    CHASSIS_INFO = 3,
    PROCESSOR_INFO = 4,
    MEMORY_CONTROLLER = 5,
    MEMORY_MODULE = 6,
    CACHE_INFO = 7,
    SYSTEM_SLOTS = 9,
    PHYSICAL_MEMORY_ARRAY = 16,
    MEMORY_DEVICE = 17,
    MEMORY_ARRAY_MAPPED_ADDRESS = 19,
    SYSTEM_BOOT_INFO = 32,
};

// 完整的硬件配置
struct HardwareConfig {
    std::vector<DiskInfo> disks;
    CpuInfo cpu;
    BaseboardInfo baseboard;
    BiosInfo bios;
    SystemInfo system;
    std::vector<NetworkAdapterInfo> network_adapters;
    
    // 是否启用各项伪装
    bool spoof_disk = true;
    bool spoof_cpu = true;
    bool spoof_baseboard = true;
    bool spoof_bios = true;
    bool spoof_system = true;
    bool spoof_network = true;
    bool spoof_smbios = true;
    bool spoof_registry = true;
    bool spoof_wmi = true;
};

// WMI类名映射
constexpr const wchar_t* WMI_CLASS_DISK = L"Win32_DiskDrive";
constexpr const wchar_t* WMI_CLASS_BIOS = L"Win32_BIOS";
constexpr const wchar_t* WMI_CLASS_BASEBOARD = L"Win32_BaseBoard";
constexpr const wchar_t* WMI_CLASS_CPU = L"Win32_Processor";
constexpr const wchar_t* WMI_CLASS_NETWORK = L"Win32_NetworkAdapter";
constexpr const wchar_t* WMI_CLASS_NETWORK_CONFIG = L"Win32_NetworkAdapterConfiguration";
constexpr const wchar_t* WMI_CLASS_SYSTEM = L"Win32_ComputerSystemProduct";
constexpr const wchar_t* WMI_CLASS_PHYSICAL_MEMORY = L"Win32_PhysicalMemory";
constexpr const wchar_t* WMI_CLASS_VIDEO = L"Win32_VideoController";
constexpr const wchar_t* WMI_CLASS_OS = L"Win32_OperatingSystem";

// 注册表路径
constexpr const wchar_t* REG_PATH_MACHINE_GUID = L"SOFTWARE\\Microsoft\\Cryptography";
constexpr const wchar_t* REG_PATH_PRODUCT_ID = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
constexpr const wchar_t* REG_PATH_BIOS = L"HARDWARE\\DESCRIPTION\\System\\BIOS";
constexpr const wchar_t* REG_PATH_HARDWARE_PROFILE = L"SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001";
constexpr const wchar_t* REG_PATH_NETWORK_CARDS = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";
constexpr const wchar_t* REG_PATH_COMPUTER_NAME = L"SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName";

// DeviceIoControl控制码
constexpr DWORD IOCTL_STORAGE_QUERY_PROPERTY = 0x2D1400;
constexpr DWORD IOCTL_DISK_GET_DRIVE_GEOMETRY = 0x70000;
constexpr DWORD IOCTL_SMART_RCV_DRIVE_DATA = 0x7C088;
constexpr DWORD IOCTL_ATA_PASS_THROUGH = 0x4D02C;
constexpr DWORD IOCTL_SCSI_PASS_THROUGH = 0x4D004;
constexpr DWORD IOCTL_SCSI_MINIPORT = 0x4D008;

} // namespace hwid

