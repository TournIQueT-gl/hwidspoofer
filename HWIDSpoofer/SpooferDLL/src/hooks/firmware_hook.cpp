#include "hooks.h"
#include "hook_manager.h"
#include "logger.h"
#include "../Common/config.h"
#include <string>
#include <cstring>

namespace hwid {
namespace hooks {
namespace firmware {

// 原始函数指针
GetSystemFirmwareTable_t Original_GetSystemFirmwareTable = nullptr;
EnumSystemFirmwareTables_t Original_EnumSystemFirmwareTables = nullptr;

// SMBIOS表签名
constexpr DWORD RSMB_SIGNATURE = 'RSMB';  // Raw SMBIOS
constexpr DWORD FIRM_SIGNATURE = 'FIRM';  // Firmware
constexpr DWORD ACPI_SIGNATURE = 'ACPI';  // ACPI

// SMBIOS表头结构
#pragma pack(push, 1)
struct RawSMBIOSData {
    BYTE Used20CallingMethod;
    BYTE SMBIOSMajorVersion;
    BYTE SMBIOSMinorVersion;
    BYTE DmiRevision;
    DWORD Length;
    BYTE SMBIOSTableData[1];
};

struct SMBIOSHeader {
    BYTE Type;
    BYTE Length;
    WORD Handle;
};
#pragma pack(pop)

// SMBIOS类型
enum SMBIOSType {
    SMBIOS_BIOS_INFO = 0,
    SMBIOS_SYSTEM_INFO = 1,
    SMBIOS_BASEBOARD_INFO = 2,
    SMBIOS_CHASSIS_INFO = 3,
    SMBIOS_PROCESSOR_INFO = 4,
    SMBIOS_MEMORY_DEVICE = 17,
};

// 获取SMBIOS字符串
char* get_smbios_string(BYTE* data, BYTE string_index) {
    if (string_index == 0) return nullptr;
    
    SMBIOSHeader* header = (SMBIOSHeader*)data;
    char* strings = (char*)(data + header->Length);
    
    for (BYTE i = 1; i < string_index; i++) {
        while (*strings) strings++;
        strings++;
    }
    
    return strings;
}

// 修改SMBIOS字符串
void modify_smbios_string(BYTE* data, BYTE string_index, const std::string& new_value) {
    if (string_index == 0 || new_value.empty()) return;
    
    char* str = get_smbios_string(data, string_index);
    if (str) {
        size_t old_len = strlen(str);
        size_t new_len = std::min(new_value.length(), old_len);
        memcpy(str, new_value.c_str(), new_len);
        if (new_len < old_len) {
            memset(str + new_len, ' ', old_len - new_len);  // 填充空格
        }
    }
}

// 处理SMBIOS表
void process_smbios_table(BYTE* table_data, DWORD length) {
    if (!Config::instance().get_hardware_config().spoof_smbios) {
        return;
    }
    
    auto& config = Config::instance().get_hardware_config();
    BYTE* current = table_data;
    BYTE* end = table_data + length;
    
    while (current < end) {
        SMBIOSHeader* header = (SMBIOSHeader*)current;
        
        if (header->Type == 127) {  // End of table
            break;
        }
        
        switch (header->Type) {
            case SMBIOS_BIOS_INFO: {
                // Type 0: BIOS Information
                if (header->Length >= 0x18 && config.spoof_bios) {
                    // Offset 0x04: Vendor (string index)
                    // Offset 0x05: BIOS Version (string index)
                    // Offset 0x08: BIOS Release Date (string index)
                    
                    BYTE vendor_idx = current[0x04];
                    BYTE version_idx = current[0x05];
                    BYTE date_idx = current[0x08];
                    
                    if (!config.bios.manufacturer.empty()) {
                        modify_smbios_string(current, vendor_idx, config.bios.manufacturer);
                    }
                    if (!config.bios.version.empty()) {
                        modify_smbios_string(current, version_idx, config.bios.version);
                    }
                    if (!config.bios.release_date.empty()) {
                        modify_smbios_string(current, date_idx, config.bios.release_date);
                    }
                    
                    LOG_DEBUG("[SMBIOS] Spoofed BIOS Info");
                }
                break;
            }
            
            case SMBIOS_SYSTEM_INFO: {
                // Type 1: System Information
                if (header->Length >= 0x1B && config.spoof_system) {
                    // Offset 0x04: Manufacturer (string index)
                    // Offset 0x05: Product Name (string index)
                    // Offset 0x06: Version (string index)
                    // Offset 0x07: Serial Number (string index)
                    // Offset 0x08-0x17: UUID (16 bytes)
                    
                    BYTE serial_idx = current[0x07];
                    
                    // 修改序列号
                    if (!config.bios.serial_number.empty()) {
                        modify_smbios_string(current, serial_idx, config.bios.serial_number);
                    }
                    
                    // 修改UUID
                    if (!config.system.uuid.empty() && config.system.uuid.length() >= 32) {
                        // UUID格式: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
                        // 转换为16字节
                        BYTE* uuid_bytes = current + 0x08;
                        std::string uuid_hex = config.system.uuid;
                        uuid_hex.erase(std::remove(uuid_hex.begin(), uuid_hex.end(), '-'), uuid_hex.end());
                        
                        if (uuid_hex.length() >= 32) {
                            for (int i = 0; i < 16; i++) {
                                char hex[3] = {uuid_hex[i*2], uuid_hex[i*2+1], 0};
                                uuid_bytes[i] = (BYTE)strtol(hex, nullptr, 16);
                            }
                        }
                    }
                    
                    LOG_DEBUG("[SMBIOS] Spoofed System Info");
                }
                break;
            }
            
            case SMBIOS_BASEBOARD_INFO: {
                // Type 2: Baseboard Information
                if (header->Length >= 0x0F && config.spoof_baseboard) {
                    // Offset 0x04: Manufacturer (string index)
                    // Offset 0x05: Product (string index)
                    // Offset 0x06: Version (string index)
                    // Offset 0x07: Serial Number (string index)
                    
                    BYTE mfg_idx = current[0x04];
                    BYTE product_idx = current[0x05];
                    BYTE version_idx = current[0x06];
                    BYTE serial_idx = current[0x07];
                    
                    if (!config.baseboard.manufacturer.empty()) {
                        modify_smbios_string(current, mfg_idx, config.baseboard.manufacturer);
                    }
                    if (!config.baseboard.product.empty()) {
                        modify_smbios_string(current, product_idx, config.baseboard.product);
                    }
                    if (!config.baseboard.version.empty()) {
                        modify_smbios_string(current, version_idx, config.baseboard.version);
                    }
                    if (!config.baseboard.serial_number.empty()) {
                        modify_smbios_string(current, serial_idx, config.baseboard.serial_number);
                    }
                    
                    LOG_DEBUG("[SMBIOS] Spoofed Baseboard Info");
                }
                break;
            }
            
            case SMBIOS_CHASSIS_INFO: {
                // Type 3: Chassis Information
                if (header->Length >= 0x0D) {
                    BYTE serial_idx = current[0x07];
                    if (!config.bios.serial_number.empty()) {
                        modify_smbios_string(current, serial_idx, config.bios.serial_number);
                    }
                    LOG_DEBUG("[SMBIOS] Spoofed Chassis Info");
                }
                break;
            }
        }
        
        // 移动到下一个表
        // 跳过头部
        current += header->Length;
        // 跳过字符串（以双NULL结尾）
        while (current < end - 1 && !(current[0] == 0 && current[1] == 0)) {
            current++;
        }
        current += 2;
    }
}

// Hook: GetSystemFirmwareTable
UINT WINAPI Hooked_GetSystemFirmwareTable(
    DWORD FirmwareTableProviderSignature,
    DWORD FirmwareTableID,
    PVOID pFirmwareTableBuffer,
    DWORD BufferSize)
{
    // 调用原始函数
    UINT result = Original_GetSystemFirmwareTable(
        FirmwareTableProviderSignature, FirmwareTableID,
        pFirmwareTableBuffer, BufferSize);
    
    if (result == 0 || pFirmwareTableBuffer == nullptr) {
        return result;
    }
    
    // 检查是否启用SMBIOS伪装
    if (!Config::instance().get_hardware_config().spoof_smbios) {
        return result;
    }
    
    // 处理RSMB（Raw SMBIOS）表
    if (FirmwareTableProviderSignature == RSMB_SIGNATURE) {
        RawSMBIOSData* smbios = (RawSMBIOSData*)pFirmwareTableBuffer;
        
        LOG_DEBUG("[Firmware] SMBIOS Table: Version %d.%d, Length %d",
                 smbios->SMBIOSMajorVersion, smbios->SMBIOSMinorVersion,
                 smbios->Length);
        
        // 处理SMBIOS表数据
        process_smbios_table(smbios->SMBIOSTableData, smbios->Length);
    }
    
    return result;
}

// Hook: EnumSystemFirmwareTables
UINT WINAPI Hooked_EnumSystemFirmwareTables(
    DWORD FirmwareTableProviderSignature,
    PVOID pFirmwareTableEnumBuffer,
    DWORD BufferSize)
{
    // 这个函数只是枚举可用的表，不需要修改
    return Original_EnumSystemFirmwareTables(
        FirmwareTableProviderSignature,
        pFirmwareTableEnumBuffer,
        BufferSize);
}

bool install() {
    LOG_INFO("Installing Firmware hooks...");
    
    bool success = true;
    
    // GetSystemFirmwareTable
    if (!HookManager::instance().create_hook_api(
            "GetSystemFirmwareTable", "kernel32.dll", "GetSystemFirmwareTable",
            (void*)&Hooked_GetSystemFirmwareTable, (void**)&Original_GetSystemFirmwareTable)) {
        LOG_ERROR("Failed to hook GetSystemFirmwareTable");
        success = false;
    }
    
    // EnumSystemFirmwareTables（可选）
    if (!HookManager::instance().create_hook_api(
            "EnumSystemFirmwareTables", "kernel32.dll", "EnumSystemFirmwareTables",
            (void*)&Hooked_EnumSystemFirmwareTables, (void**)&Original_EnumSystemFirmwareTables)) {
        LOG_WARNING("Failed to hook EnumSystemFirmwareTables (optional)");
    }
    
    LOG_INFO("Firmware hooks installed: %s", success ? "OK" : "PARTIAL");
    return success;
}

void uninstall() {
    LOG_INFO("Uninstalling Firmware hooks...");
    
    HookManager::instance().remove_hook("GetSystemFirmwareTable");
    HookManager::instance().remove_hook("EnumSystemFirmwareTables");
}

} // namespace firmware
} // namespace hooks
} // namespace hwid

