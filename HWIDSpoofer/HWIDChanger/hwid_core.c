/*
 * HWID Changer 核心实现
 * 纯C实现，最小依赖
 */

#include "hwid_changer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 内部状态
static int g_initialized = 0;
static HMODULE g_hook_dll = NULL;
static HANDLE g_driver_handle = INVALID_HANDLE_VALUE;

// 随机数生成器（使用更好的种子）
static uint64_t g_rng_state = 0;

static uint64_t xorshift64(void) {
    uint64_t x = g_rng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    g_rng_state = x;
    return x;
}

static void init_rng(void) {
    LARGE_INTEGER pc, freq;
    QueryPerformanceCounter(&pc);
    QueryPerformanceFrequency(&freq);
    g_rng_state = pc.QuadPart ^ (GetTickCount64() << 32) ^ 
                  (GetCurrentProcessId() * 0x5851F42D4C957F2DULL);
    // 预热
    for (int i = 0; i < 10; i++) xorshift64();
}

// 初始化
int hwid_init(void) {
    if (g_initialized) return 0;
    
    init_rng();
    g_initialized = 1;
    return 0;
}

void hwid_cleanup(void) {
    if (g_hook_dll) {
        FreeLibrary(g_hook_dll);
        g_hook_dll = NULL;
    }
    if (g_driver_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(g_driver_handle);
        g_driver_handle = INVALID_HANDLE_VALUE;
    }
    g_initialized = 0;
}

// 随机序列号生成
void hwid_random_serial(char* buf, int len) {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (int i = 0; i < len - 1; i++) {
        buf[i] = charset[xorshift64() % (sizeof(charset) - 1)];
    }
    buf[len - 1] = '\0';
}

void hwid_random_uuid(char* buf) {
    // 格式: XXXXXXXX-XXXX-4XXX-YXXX-XXXXXXXXXXXX
    uint64_t r1 = xorshift64();
    uint64_t r2 = xorshift64();
    
    sprintf(buf, "%08X-%04X-4%03X-%04X-%012llX",
            (uint32_t)(r1 >> 32),
            (uint16_t)(r1 >> 16),
            (uint16_t)(r1 & 0x0FFF),
            (uint16_t)((r2 & 0x3FFF) | 0x8000),
            (unsigned long long)(r2 >> 16) & 0xFFFFFFFFFFFFULL);
}

void hwid_random_mac(uint8_t* mac) {
    // 使用常见厂商前缀
    static const uint8_t oui[][3] = {
        {0x00, 0x1A, 0x2B},  // Ayecom
        {0x00, 0x50, 0x56},  // VMware
        {0xD4, 0xBE, 0xD9},  // Dell
        {0x54, 0xE1, 0xAD},  // Intel
        {0x00, 0x25, 0x90},  // Intel
        {0xF0, 0xDE, 0xF1},  // Intel
    };
    
    int idx = xorshift64() % (sizeof(oui) / sizeof(oui[0]));
    mac[0] = oui[idx][0];
    mac[1] = oui[idx][1];
    mac[2] = oui[idx][2];
    mac[3] = (uint8_t)xorshift64();
    mac[4] = (uint8_t)xorshift64();
    mac[5] = (uint8_t)xorshift64();
}

uint32_t hwid_random_volume_serial(void) {
    return (uint32_t)xorshift64();
}

// 生成完整随机配置
void hwid_generate_random(HwidConfig* config) {
    if (!g_initialized) hwid_init();
    
    memset(config, 0, sizeof(HwidConfig));
    config->flags = FLAG_ALL;
    
    // 磁盘
    static const char* disk_prefixes[] = {"WD-WMC", "ST", "SAMSUNG_", "INTEL_"};
    int idx = xorshift64() % 4;
    strcpy(config->disk_serial, disk_prefixes[idx]);
    hwid_random_serial(config->disk_serial + strlen(config->disk_serial), 12);
    
    strcpy(config->disk_model, "Virtual HD ATA Device");
    config->volume_serial = hwid_random_volume_serial();
    
    // BIOS
    hwid_random_serial(config->bios_serial, 16);
    strcpy(config->bios_vendor, "American Megatrends Inc.");
    strcpy(config->bios_version, "2.17.1246");
    strcpy(config->bios_date, "12/01/2023");
    
    // 主板
    hwid_random_serial(config->board_serial, 14);
    strcpy(config->board_product, "B460M-PLUS");
    strcpy(config->board_vendor, "ASUSTeK");
    
    // 系统
    hwid_random_uuid(config->system_uuid);
    hwid_random_uuid(config->machine_guid);
    
    sprintf(config->product_id, "%05d-%03d-%07d-%05d",
            (int)(xorshift64() % 100000), (int)(xorshift64() % 1000),
            (int)(xorshift64() % 10000000), (int)(xorshift64() % 100000));
    
    sprintf(config->computer_name, "PC-%08X", (uint32_t)xorshift64());
    
    // 网络
    hwid_random_mac(config->mac_address);
    hwid_random_mac(config->mac_address2);
    
    // CPU
    sprintf(config->cpu_id, "%016llX", (unsigned long long)xorshift64());
    
    // GPU
    hwid_random_serial(config->gpu_serial, 20);
}

// 简单配置文件解析
static char* trim(char* str) {
    while (*str == ' ' || *str == '\t') str++;
    char* end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    return str;
}

int hwid_load_config(const char* path, HwidConfig* config) {
    FILE* f = fopen(path, "r");
    if (!f) return -1;
    
    // 先生成随机配置作为默认值
    hwid_generate_random(config);
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char* p = trim(line);
        if (*p == '#' || *p == '\0' || *p == ';') continue;
        
        char* eq = strchr(p, '=');
        if (!eq) continue;
        
        *eq = '\0';
        char* key = trim(p);
        char* val = trim(eq + 1);
        
        // 移除引号
        if (*val == '"') {
            val++;
            char* end = strchr(val, '"');
            if (end) *end = '\0';
        }
        
        // 解析配置项
        if (strcmp(key, "disk_serial") == 0) strncpy(config->disk_serial, val, MAX_SERIAL_LEN-1);
        else if (strcmp(key, "disk_model") == 0) strncpy(config->disk_model, val, MAX_SERIAL_LEN-1);
        else if (strcmp(key, "volume_serial") == 0) config->volume_serial = strtoul(val, NULL, 0);
        else if (strcmp(key, "bios_serial") == 0) strncpy(config->bios_serial, val, MAX_SERIAL_LEN-1);
        else if (strcmp(key, "bios_vendor") == 0) strncpy(config->bios_vendor, val, MAX_STRING_LEN-1);
        else if (strcmp(key, "board_serial") == 0) strncpy(config->board_serial, val, MAX_SERIAL_LEN-1);
        else if (strcmp(key, "board_product") == 0) strncpy(config->board_product, val, MAX_STRING_LEN-1);
        else if (strcmp(key, "system_uuid") == 0) strncpy(config->system_uuid, val, MAX_UUID_LEN-1);
        else if (strcmp(key, "machine_guid") == 0) strncpy(config->machine_guid, val, MAX_UUID_LEN-1);
        else if (strcmp(key, "product_id") == 0) strncpy(config->product_id, val, 31);
        else if (strcmp(key, "computer_name") == 0) strncpy(config->computer_name, val, 15);
        else if (strcmp(key, "cpu_id") == 0) strncpy(config->cpu_id, val, 23);
        else if (strcmp(key, "mac_address") == 0) {
            sscanf(val, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                   &config->mac_address[0], &config->mac_address[1], &config->mac_address[2],
                   &config->mac_address[3], &config->mac_address[4], &config->mac_address[5]);
        }
        else if (strcmp(key, "flags") == 0) config->flags = strtoul(val, NULL, 0);
    }
    
    fclose(f);
    return 0;
}

int hwid_save_config(const char* path, const HwidConfig* config) {
    FILE* f = fopen(path, "w");
    if (!f) return -1;
    
    fprintf(f, "# HWID Changer Configuration\n");
    fprintf(f, "# Generated: %s\n\n", __DATE__);
    
    fprintf(f, "# Flags (bitmask): 0x%04X\n", config->flags);
    fprintf(f, "flags = 0x%04X\n\n", config->flags);
    
    fprintf(f, "# Disk\n");
    fprintf(f, "disk_serial = \"%s\"\n", config->disk_serial);
    fprintf(f, "disk_model = \"%s\"\n", config->disk_model);
    fprintf(f, "volume_serial = 0x%08X\n\n", config->volume_serial);
    
    fprintf(f, "# BIOS\n");
    fprintf(f, "bios_serial = \"%s\"\n", config->bios_serial);
    fprintf(f, "bios_vendor = \"%s\"\n", config->bios_vendor);
    fprintf(f, "bios_version = \"%s\"\n", config->bios_version);
    fprintf(f, "bios_date = \"%s\"\n\n", config->bios_date);
    
    fprintf(f, "# Baseboard\n");
    fprintf(f, "board_serial = \"%s\"\n", config->board_serial);
    fprintf(f, "board_product = \"%s\"\n", config->board_product);
    fprintf(f, "board_vendor = \"%s\"\n\n", config->board_vendor);
    
    fprintf(f, "# System\n");
    fprintf(f, "system_uuid = \"%s\"\n", config->system_uuid);
    fprintf(f, "machine_guid = \"%s\"\n", config->machine_guid);
    fprintf(f, "product_id = \"%s\"\n", config->product_id);
    fprintf(f, "computer_name = \"%s\"\n\n", config->computer_name);
    
    fprintf(f, "# Network\n");
    fprintf(f, "mac_address = \"%02X:%02X:%02X:%02X:%02X:%02X\"\n",
            config->mac_address[0], config->mac_address[1], config->mac_address[2],
            config->mac_address[3], config->mac_address[4], config->mac_address[5]);
    
    fprintf(f, "\n# CPU\n");
    fprintf(f, "cpu_id = \"%s\"\n", config->cpu_id);
    
    fclose(f);
    return 0;
}

