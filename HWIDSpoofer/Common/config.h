#pragma once

#include "hwid_types.h"
#include <string>
#include <map>
#include <mutex>

namespace hwid {

// 配置管理器 - 单例模式
class Config {
public:
    static Config& instance() {
        static Config instance;
        return instance;
    }

    // 从JSON文件加载配置
    bool load_from_file(const std::string& filepath);
    bool load_from_file(const std::wstring& filepath);
    
    // 保存配置到文件
    bool save_to_file(const std::string& filepath);
    
    // 生成随机配置
    void generate_random_config();
    
    // 获取硬件配置
    HardwareConfig& get_hardware_config() { return m_config; }
    const HardwareConfig& get_hardware_config() const { return m_config; }
    
    // 获取特定类型的伪装值
    std::string get_spoofed_value(HwidType type) const;
    std::wstring get_spoofed_value_wide(HwidType type) const;
    
    // 设置特定类型的伪装值
    void set_spoofed_value(HwidType type, const std::string& value);
    
    // 检查是否启用特定类型的伪装
    bool is_type_enabled(HwidType type) const;
    void set_type_enabled(HwidType type, bool enabled);
    
    // 磁盘相关
    const DiskInfo& get_disk_info(size_t index = 0) const;
    void set_disk_info(const DiskInfo& info, size_t index = 0);
    
    // 网络相关
    const NetworkAdapterInfo& get_network_adapter(size_t index = 0) const;
    void set_network_adapter(const NetworkAdapterInfo& info, size_t index = 0);
    std::string get_spoofed_mac(size_t index = 0) const;
    
    // 日志级别
    enum class LogLevel { NONE, ERROR, WARNING, INFO, DEBUG, TRACE };
    LogLevel log_level = LogLevel::INFO;
    
    // 配置文件路径
    std::wstring config_path;
    
    // 是否已加载
    bool is_loaded() const { return m_loaded; }

private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    
    HardwareConfig m_config;
    std::map<HwidType, std::string> m_spoofed_values;
    std::map<HwidType, bool> m_type_enabled;
    bool m_loaded = false;
    mutable std::mutex m_mutex;
};

// 随机生成工具函数
namespace random {
    // 生成随机序列号（字母数字）
    std::string generate_serial(size_t length);
    
    // 生成随机UUID
    std::string generate_uuid();
    
    // 生成随机MAC地址
    std::string generate_mac();
    void generate_mac_bytes(uint8_t* mac);
    
    // 生成随机GUID
    std::string generate_guid();
    
    // 生成随机硬盘序列号（特定格式）
    std::string generate_disk_serial();
    
    // 生成随机卷序列号
    uint32_t generate_volume_serial();
    
    // 生成随机计算机名
    std::string generate_computer_name();
    
    // 生成随机产品ID
    std::string generate_product_id();
    
    // 基于种子生成（可重复）
    void set_seed(uint64_t seed);
    
    // 哈希函数（用于基于原始值生成伪装值）
    std::string hash_based_serial(const std::string& original, size_t length);
}

} // namespace hwid

