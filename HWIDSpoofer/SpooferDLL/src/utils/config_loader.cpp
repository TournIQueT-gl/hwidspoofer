#include "../Common/config.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <charconv>
#include <cctype>

// 简单的JSON解析（不依赖第三方库）
// 实际项目中建议使用nlohmann/json

namespace hwid {

// 简单的JSON值提取
std::string extract_json_string(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    
    pos = json.find(":", pos);
    if (pos == std::string::npos) return "";
    
    pos = json.find("\"", pos);
    if (pos == std::string::npos) return "";
    
    size_t start = pos + 1;
    size_t end = json.find("\"", start);
    if (end == std::string::npos) return "";
    
    return json.substr(start, end - start);
}

bool extract_json_bool(const std::string& json, const std::string& key, bool default_value = true) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_value;
    
    pos = json.find(":", pos);
    if (pos == std::string::npos) return default_value;
    
    // 跳过空白
    while (pos < json.length() && (json[pos] == ':' || json[pos] == ' ' || json[pos] == '\t')) {
        pos++;
    }
    
    if (json.substr(pos, 4) == "true") return true;
    if (json.substr(pos, 5) == "false") return false;
    
    return default_value;
}

uint32_t extract_json_uint(const std::string& json, const std::string& key, uint32_t default_value = 0) {
    const std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_value;

    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return default_value;
    ++pos;

    while (pos < json.size() &&
           std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }

    const size_t start = pos;
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') {
        ++pos;
    }
    if (start == pos) return default_value;

    uint32_t value = 0;
    const char* first = json.data() + start;
    const char* last = json.data() + pos;
    const auto parsed = std::from_chars(first, last, value, 10);
    if (parsed.ec != std::errc{} || parsed.ptr != last) {
        return default_value;
    }

    return value;
}

bool Config::load_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open config file: %s", filepath.c_str());
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    
    // 解析配置
    // 磁盘
    DiskInfo disk;
    disk.serial_number = extract_json_string(json, "disk_serial");
    disk.product_id = extract_json_string(json, "disk_product_id");
    disk.vendor_id = extract_json_string(json, "disk_vendor_id");
    disk.revision = extract_json_string(json, "disk_revision");
    disk.volume_serial = extract_json_uint(json, "volume_serial");
    
    if (!disk.serial_number.empty()) {
        m_config.disks.clear();
        m_config.disks.push_back(disk);
    }
    
    // CPU
    m_config.cpu.processor_id = extract_json_string(json, "processor_id");
    m_config.cpu.serial_number = extract_json_string(json, "cpu_serial");
    m_config.cpu.brand_string = extract_json_string(json, "cpu_brand");
    
    // 主板
    m_config.baseboard.serial_number = extract_json_string(json, "baseboard_serial");
    m_config.baseboard.product = extract_json_string(json, "baseboard_product");
    m_config.baseboard.manufacturer = extract_json_string(json, "baseboard_manufacturer");
    m_config.baseboard.version = extract_json_string(json, "baseboard_version");
    
    // BIOS
    m_config.bios.serial_number = extract_json_string(json, "bios_serial");
    m_config.bios.version = extract_json_string(json, "bios_version");
    m_config.bios.manufacturer = extract_json_string(json, "bios_manufacturer");
    m_config.bios.release_date = extract_json_string(json, "bios_release_date");
    m_config.bios.sm_bios_version = extract_json_string(json, "smbios_version");
    
    // 系统
    m_config.system.uuid = extract_json_string(json, "system_uuid");
    m_config.system.machine_guid = extract_json_string(json, "machine_guid");
    m_config.system.product_id = extract_json_string(json, "product_id");
    m_config.system.computer_name = extract_json_string(json, "computer_name");
    
    // 网络
    NetworkAdapterInfo net;
    net.mac_string = extract_json_string(json, "mac_address");
    if (!net.mac_string.empty()) {
        // 解析MAC地址字符串
        sscanf_s(net.mac_string.c_str(), "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
                &net.mac_address[0], &net.mac_address[1], &net.mac_address[2],
                &net.mac_address[3], &net.mac_address[4], &net.mac_address[5]);
        m_config.network_adapters.clear();
        m_config.network_adapters.push_back(net);
    }
    
    // 伪装开关
    m_config.spoof_disk = extract_json_bool(json, "spoof_disk");
    m_config.spoof_cpu = extract_json_bool(json, "spoof_cpu");
    m_config.spoof_baseboard = extract_json_bool(json, "spoof_baseboard");
    m_config.spoof_bios = extract_json_bool(json, "spoof_bios");
    m_config.spoof_system = extract_json_bool(json, "spoof_system");
    m_config.spoof_network = extract_json_bool(json, "spoof_network");
    m_config.spoof_smbios = extract_json_bool(json, "spoof_smbios");
    m_config.spoof_registry = extract_json_bool(json, "spoof_registry");
    m_config.spoof_wmi = extract_json_bool(json, "spoof_wmi");
    
    m_loaded = true;
    LOG_INFO("Configuration loaded from: %s", filepath.c_str());
    
    return true;
}

bool Config::load_from_file(const std::wstring& filepath) {
    std::string path(filepath.begin(), filepath.end());
    return load_from_file(path);
}

bool Config::save_to_file(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("Failed to create config file: %s", filepath.c_str());
        return false;
    }
    
    file << "{\n";
    file << "  \"_comment\": \"HWID Spoofer Configuration File\",\n";
    file << "  \"version\": \"1.0\",\n\n";
    
    // 伪装开关
    file << "  \"spoof_disk\": " << (m_config.spoof_disk ? "true" : "false") << ",\n";
    file << "  \"spoof_cpu\": " << (m_config.spoof_cpu ? "true" : "false") << ",\n";
    file << "  \"spoof_baseboard\": " << (m_config.spoof_baseboard ? "true" : "false") << ",\n";
    file << "  \"spoof_bios\": " << (m_config.spoof_bios ? "true" : "false") << ",\n";
    file << "  \"spoof_system\": " << (m_config.spoof_system ? "true" : "false") << ",\n";
    file << "  \"spoof_network\": " << (m_config.spoof_network ? "true" : "false") << ",\n";
    file << "  \"spoof_smbios\": " << (m_config.spoof_smbios ? "true" : "false") << ",\n";
    file << "  \"spoof_registry\": " << (m_config.spoof_registry ? "true" : "false") << ",\n";
    file << "  \"spoof_wmi\": " << (m_config.spoof_wmi ? "true" : "false") << ",\n\n";
    
    // 磁盘
    if (!m_config.disks.empty()) {
        auto& disk = m_config.disks[0];
        file << "  \"disk_serial\": \"" << disk.serial_number << "\",\n";
        file << "  \"disk_product_id\": \"" << disk.product_id << "\",\n";
        file << "  \"disk_vendor_id\": \"" << disk.vendor_id << "\",\n";
        file << "  \"disk_revision\": \"" << disk.revision << "\",\n";
        file << "  \"volume_serial\": " << disk.volume_serial << ",\n\n";
    }
    
    // CPU
    file << "  \"processor_id\": \"" << m_config.cpu.processor_id << "\",\n";
    file << "  \"cpu_serial\": \"" << m_config.cpu.serial_number << "\",\n";
    file << "  \"cpu_brand\": \"" << m_config.cpu.brand_string << "\",\n\n";
    
    // 主板
    file << "  \"baseboard_serial\": \"" << m_config.baseboard.serial_number << "\",\n";
    file << "  \"baseboard_product\": \"" << m_config.baseboard.product << "\",\n";
    file << "  \"baseboard_manufacturer\": \"" << m_config.baseboard.manufacturer << "\",\n";
    file << "  \"baseboard_version\": \"" << m_config.baseboard.version << "\",\n\n";
    
    // BIOS
    file << "  \"bios_serial\": \"" << m_config.bios.serial_number << "\",\n";
    file << "  \"bios_version\": \"" << m_config.bios.version << "\",\n";
    file << "  \"bios_manufacturer\": \"" << m_config.bios.manufacturer << "\",\n";
    file << "  \"bios_release_date\": \"" << m_config.bios.release_date << "\",\n";
    file << "  \"smbios_version\": \"" << m_config.bios.sm_bios_version << "\",\n\n";
    
    // 系统
    file << "  \"system_uuid\": \"" << m_config.system.uuid << "\",\n";
    file << "  \"machine_guid\": \"" << m_config.system.machine_guid << "\",\n";
    file << "  \"product_id\": \"" << m_config.system.product_id << "\",\n";
    file << "  \"computer_name\": \"" << m_config.system.computer_name << "\",\n\n";
    
    // 网络
    if (!m_config.network_adapters.empty()) {
        file << "  \"mac_address\": \"" << m_config.network_adapters[0].mac_string << "\"\n";
    } else {
        file << "  \"mac_address\": \"\"\n";
    }
    
    file << "}\n";
    
    LOG_INFO("Configuration saved to: %s", filepath.c_str());
    return true;
}

void Config::generate_random_config() {
    LOG_INFO("Generating random hardware configuration...");
    
    // 生成随机磁盘信息
    DiskInfo disk;
    disk.serial_number = random::generate_disk_serial();
    disk.product_id = "VBOX HARDDISK";
    disk.vendor_id = "ATA";
    disk.revision = "1.0";
    disk.volume_serial = random::generate_volume_serial();
    m_config.disks.clear();
    m_config.disks.push_back(disk);
    
    // 生成随机CPU信息
    m_config.cpu.processor_id = random::generate_serial(16);
    m_config.cpu.serial_number = random::generate_serial(12);
    m_config.cpu.brand_string = "Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz";
    
    // 生成随机主板信息
    m_config.baseboard.serial_number = random::generate_serial(12);
    m_config.baseboard.product = "B460M-PLUS";
    m_config.baseboard.manufacturer = "ASUSTeK COMPUTER INC.";
    m_config.baseboard.version = "Rev 1.xx";
    
    // 生成随机BIOS信息
    m_config.bios.serial_number = random::generate_serial(16);
    m_config.bios.version = "American Megatrends Inc. 2.17.1246";
    m_config.bios.manufacturer = "American Megatrends Inc.";
    m_config.bios.release_date = "12/01/2023";
    m_config.bios.sm_bios_version = "2.8";
    
    // 生成随机系统信息
    m_config.system.uuid = random::generate_uuid();
    m_config.system.machine_guid = random::generate_guid();
    m_config.system.product_id = random::generate_product_id();
    m_config.system.computer_name = random::generate_computer_name();
    
    // 生成随机网络信息
    NetworkAdapterInfo net;
    random::generate_mac_bytes(net.mac_address);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
            net.mac_address[0], net.mac_address[1], net.mac_address[2],
            net.mac_address[3], net.mac_address[4], net.mac_address[5]);
    net.mac_string = mac_str;
    m_config.network_adapters.clear();
    m_config.network_adapters.push_back(net);
    
    // 默认启用所有伪装
    m_config.spoof_disk = true;
    m_config.spoof_cpu = true;
    m_config.spoof_baseboard = true;
    m_config.spoof_bios = true;
    m_config.spoof_system = true;
    m_config.spoof_network = true;
    m_config.spoof_smbios = true;
    m_config.spoof_registry = true;
    m_config.spoof_wmi = true;
    
    m_loaded = true;
    LOG_INFO("Random configuration generated successfully");
}

std::string Config::get_spoofed_value(HwidType type) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    switch (type) {
        case HwidType::DISK_SERIAL:
            return m_config.disks.empty() ? "" : m_config.disks[0].serial_number;
        case HwidType::CPU_ID:
            return m_config.cpu.processor_id;
        case HwidType::BASEBOARD_SERIAL:
            return m_config.baseboard.serial_number;
        case HwidType::BIOS_SERIAL:
            return m_config.bios.serial_number;
        case HwidType::SYSTEM_UUID:
            return m_config.system.uuid;
        case HwidType::MACHINE_GUID:
            return m_config.system.machine_guid;
        case HwidType::PRODUCT_ID:
            return m_config.system.product_id;
        case HwidType::MAC_ADDRESS:
            return m_config.network_adapters.empty() ? "" : m_config.network_adapters[0].mac_string;
        default:
            return "";
    }
}

std::wstring Config::get_spoofed_value_wide(HwidType type) const {
    std::string value = get_spoofed_value(type);
    return std::wstring(value.begin(), value.end());
}

const DiskInfo& Config::get_disk_info(size_t index) const {
    static DiskInfo empty;
    if (index < m_config.disks.size()) {
        return m_config.disks[index];
    }
    return empty;
}

const NetworkAdapterInfo& Config::get_network_adapter(size_t index) const {
    static NetworkAdapterInfo empty;
    if (index < m_config.network_adapters.size()) {
        return m_config.network_adapters[index];
    }
    return empty;
}

// ==================== 随机生成函数 ====================

namespace random {

static std::mt19937_64 g_rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());

void set_seed(uint64_t seed) {
    g_rng.seed(seed);
}

std::string generate_serial(size_t length) {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<int> dist(0, sizeof(charset) - 2);
    
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; i++) {
        result += charset[dist(g_rng)];
    }
    return result;
}

std::string generate_uuid() {
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
    
    char uuid[40];
    snprintf(uuid, sizeof(uuid), "%08X-%04X-%04X-%04X-%012llX",
            dist(g_rng),
            dist(g_rng) & 0xFFFF,
            (dist(g_rng) & 0x0FFF) | 0x4000,  // Version 4
            (dist(g_rng) & 0x3FFF) | 0x8000,  // Variant
            ((uint64_t)dist(g_rng) << 16) | (dist(g_rng) & 0xFFFF));
    
    return uuid;
}

std::string generate_mac() {
    uint8_t mac[6];
    generate_mac_bytes(mac);
    
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    return mac_str;
}

void generate_mac_bytes(uint8_t* mac) {
    std::uniform_int_distribution<int> dist(0, 255);
    
    // 使用常见厂商OUI前缀
    static const uint8_t oui_prefixes[][3] = {
        {0x00, 0x1A, 0x2B},  // Dell
        {0x00, 0x50, 0x56},  // VMware
        {0x00, 0x0C, 0x29},  // VMware
        {0x00, 0x1C, 0x42},  // Parallels
        {0xD4, 0xBE, 0xD9},  // Dell
        {0x00, 0x25, 0xB5},  // Dell
        {0x54, 0xE1, 0xAD},  // Intel
    };
    
    std::uniform_int_distribution<int> oui_dist(0, sizeof(oui_prefixes) / sizeof(oui_prefixes[0]) - 1);
    int oui_idx = oui_dist(g_rng);
    
    mac[0] = oui_prefixes[oui_idx][0];
    mac[1] = oui_prefixes[oui_idx][1];
    mac[2] = oui_prefixes[oui_idx][2];
    mac[3] = dist(g_rng);
    mac[4] = dist(g_rng);
    mac[5] = dist(g_rng);
}

std::string generate_guid() {
    return generate_uuid();  // GUID和UUID格式相同
}

std::string generate_disk_serial() {
    // 常见硬盘序列号格式：厂商前缀 + 随机字符
    static const char* prefixes[] = {
        "WD-WMC", "ST", "TOSHIBA_", "SAMSUNG_", "INTEL_", "KINGSTON_"
    };
    
    std::uniform_int_distribution<int> prefix_dist(0, sizeof(prefixes) / sizeof(prefixes[0]) - 1);
    std::string serial = prefixes[prefix_dist(g_rng)];
    serial += generate_serial(10);
    
    return serial;
}

uint32_t generate_volume_serial() {
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
    return dist(g_rng);
}

std::string generate_computer_name() {
    static const char* prefixes[] = {
        "DESKTOP-", "LAPTOP-", "PC-", "WORKSTATION-"
    };
    
    std::uniform_int_distribution<int> prefix_dist(0, sizeof(prefixes) / sizeof(prefixes[0]) - 1);
    return std::string(prefixes[prefix_dist(g_rng)]) + generate_serial(7);
}

std::string generate_product_id() {
    // Windows产品ID格式: XXXXX-XXX-XXXXXXX-XXXXX
    char product_id[30];
    snprintf(product_id, sizeof(product_id), "%05d-%03d-%07d-%05d",
            rand() % 100000, rand() % 1000, rand() % 10000000, rand() % 100000);
    return product_id;
}

std::string hash_based_serial(const std::string& original, size_t length) {
    // 简单的哈希函数生成伪装序列号
    uint64_t hash = 5381;
    for (char c : original) {
        hash = ((hash << 5) + hash) + c;
    }
    
    set_seed(hash);
    return generate_serial(length);
}

} // namespace random

} // namespace hwid

