/*
 * HWID Test Program - 模拟其他程序调用系统API获取硬件信息
 * 用于验证伪装是否生效
 */

#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <windows.h>
#include <winioctl.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "advapi32.lib")

// ============== 测试函数 ==============

// 测试1: GetVolumeInformation - 获取卷序列号
void test_volume_serial(void) {
    printf("\n[TEST 1] GetVolumeInformationW - Volume Serial\n");
    printf("  ----------------------------------------\n");
    
    DWORD serial = 0;
    WCHAR volName[256] = {0};
    WCHAR fsName[256] = {0};
    
    if (GetVolumeInformationW(L"C:\\", volName, 256, &serial, NULL, NULL, fsName, 256)) {
        printf("  Volume Serial: 0x%08X\n", serial);
        printf("  Volume Name:   %ls\n", volName);
        printf("  File System:   %ls\n", fsName);
    } else {
        printf("  [ERROR] GetVolumeInformationW failed: %d\n", GetLastError());
    }
}

// 测试2: DeviceIoControl - 获取磁盘序列号
void test_disk_serial(void) {
    printf("\n[TEST 2] DeviceIoControl - Disk Serial (STORAGE_QUERY_PROPERTY)\n");
    printf("  ----------------------------------------\n");
    
    HANDLE hDisk = CreateFileW(L"\\\\.\\PhysicalDrive0", 
        0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    
    if (hDisk == INVALID_HANDLE_VALUE) {
        printf("  [ERROR] Cannot open PhysicalDrive0: %d\n", GetLastError());
        return;
    }
    
    STORAGE_PROPERTY_QUERY query = {0};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    
    BYTE buffer[1024] = {0};
    DWORD bytesReturned = 0;
    
    if (DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY,
        &query, sizeof(query), buffer, sizeof(buffer), &bytesReturned, NULL)) {
        
        STORAGE_DEVICE_DESCRIPTOR* desc = (STORAGE_DEVICE_DESCRIPTOR*)buffer;
        
        if (desc->SerialNumberOffset && desc->SerialNumberOffset < bytesReturned) {
            char* serial = (char*)buffer + desc->SerialNumberOffset;
            printf("  Disk Serial:   %s\n", serial);
        } else {
            printf("  Disk Serial:   (not available)\n");
        }
        
        if (desc->ProductIdOffset && desc->ProductIdOffset < bytesReturned) {
            char* product = (char*)buffer + desc->ProductIdOffset;
            printf("  Disk Product:  %s\n", product);
        }
        
        if (desc->VendorIdOffset && desc->VendorIdOffset < bytesReturned) {
            char* vendor = (char*)buffer + desc->VendorIdOffset;
            printf("  Disk Vendor:   %s\n", vendor);
        }
    } else {
        printf("  [ERROR] DeviceIoControl failed: %d\n", GetLastError());
    }
    
    CloseHandle(hDisk);
}

// 测试3: GetAdaptersInfo - 获取MAC地址
void test_mac_address(void) {
    printf("\n[TEST 3] GetAdaptersInfo - MAC Address\n");
    printf("  ----------------------------------------\n");
    
    IP_ADAPTER_INFO adapters[16];
    ULONG bufLen = sizeof(adapters);
    
    if (GetAdaptersInfo(adapters, &bufLen) == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO curr = adapters;
        int idx = 0;
        while (curr) {
            if (curr->AddressLength == 6) {
                printf("  Adapter %d:     %s\n", idx, curr->Description);
                printf("  MAC Address:   %02X:%02X:%02X:%02X:%02X:%02X\n",
                    curr->Address[0], curr->Address[1], curr->Address[2],
                    curr->Address[3], curr->Address[4], curr->Address[5]);
            }
            curr = curr->Next;
            idx++;
        }
    } else {
        printf("  [ERROR] GetAdaptersInfo failed: %d\n", GetLastError());
    }
}

// 测试4: GetAdaptersAddresses - 获取网卡地址
void test_adapter_addresses(void) {
    printf("\n[TEST 4] GetAdaptersAddresses - Network Adapters\n");
    printf("  ----------------------------------------\n");
    
    ULONG bufLen = 15000;
    PIP_ADAPTER_ADDRESSES addresses = (PIP_ADAPTER_ADDRESSES)malloc(bufLen);
    
    if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, addresses, &bufLen) == ERROR_SUCCESS) {
        PIP_ADAPTER_ADDRESSES curr = addresses;
        int idx = 0;
        while (curr) {
            if (curr->PhysicalAddressLength == 6) {
                printf("  Adapter %d:     %ls\n", idx, curr->FriendlyName);
                printf("  MAC Address:   %02X:%02X:%02X:%02X:%02X:%02X\n",
                    curr->PhysicalAddress[0], curr->PhysicalAddress[1],
                    curr->PhysicalAddress[2], curr->PhysicalAddress[3],
                    curr->PhysicalAddress[4], curr->PhysicalAddress[5]);
            }
            curr = curr->Next;
            idx++;
        }
    } else {
        printf("  [ERROR] GetAdaptersAddresses failed: %d\n", GetLastError());
    }
    
    free(addresses);
}

// 测试5: RegQueryValueEx - 获取MachineGuid
void test_machine_guid(void) {
    printf("\n[TEST 5] RegQueryValueExW - MachineGuid\n");
    printf("  ----------------------------------------\n");
    
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
        L"SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        WCHAR guid[128] = {0};
        DWORD size = sizeof(guid);
        DWORD type = REG_SZ;
        
        if (RegQueryValueExW(hKey, L"MachineGuid", NULL, &type, 
            (LPBYTE)guid, &size) == ERROR_SUCCESS) {
            printf("  MachineGuid:   %ls\n", guid);
        } else {
            printf("  [ERROR] Cannot read MachineGuid\n");
        }
        
        RegCloseKey(hKey);
    } else {
        printf("  [ERROR] Cannot open registry key\n");
    }
}

// 测试6: RegQueryValueEx - 获取ProductId
void test_product_id(void) {
    printf("\n[TEST 6] RegQueryValueExW - ProductId\n");
    printf("  ----------------------------------------\n");
    
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        WCHAR productId[128] = {0};
        DWORD size = sizeof(productId);
        DWORD type = REG_SZ;
        
        if (RegQueryValueExW(hKey, L"ProductId", NULL, &type,
            (LPBYTE)productId, &size) == ERROR_SUCCESS) {
            printf("  ProductId:     %ls\n", productId);
        } else {
            printf("  [ERROR] Cannot read ProductId\n");
        }
        
        RegCloseKey(hKey);
    } else {
        printf("  [ERROR] Cannot open registry key\n");
    }
}

// 测试7: GetComputerName - 获取计算机名
void test_computer_name(void) {
    printf("\n[TEST 7] GetComputerNameW - Computer Name\n");
    printf("  ----------------------------------------\n");
    
    WCHAR name[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    
    if (GetComputerNameW(name, &size)) {
        printf("  ComputerName:  %ls\n", name);
    } else {
        printf("  [ERROR] GetComputerNameW failed: %d\n", GetLastError());
    }
}

// SMBIOS 签名常量
#define RSMB_SIGNATURE 0x52534D42  // 'RSMB' = R(0x52) S(0x53) M(0x4D) B(0x42)

// 测试8: GetSystemFirmwareTable - 获取SMBIOS数据
void test_smbios(void) {
    printf("\n[TEST 8] GetSystemFirmwareTable - SMBIOS Data\n");
    printf("  ----------------------------------------\n");
    
    DWORD size = GetSystemFirmwareTable(RSMB_SIGNATURE, 0, NULL, 0);
    if (size == 0) {
        printf("  [ERROR] GetSystemFirmwareTable size failed: %d\n", GetLastError());
        return;
    }
    
    BYTE* buffer = (BYTE*)malloc(size);
    if (GetSystemFirmwareTable(RSMB_SIGNATURE, 0, buffer, size) == 0) {
        printf("  [ERROR] GetSystemFirmwareTable failed: %d\n", GetLastError());
        free(buffer);
        return;
    }
    
    // 解析SMBIOS结构
    printf("  SMBIOS Size:   %d bytes\n", size);
    
    // 简单解析 - 查找Type 1 (System Info)
    BYTE* data = buffer + 8;  // 跳过头部
    DWORD dataLen = *(DWORD*)(buffer + 4);
    BYTE* end = data + dataLen;
    
    while (data < end) {
        BYTE type = data[0];
        BYTE len = data[1];
        
        if (type == 127) break;  // End of table
        
        if (type == 1 && len >= 8) {  // System Information
            // 字符串表在结构后面
            char* strings = (char*)(data + len);
            BYTE serialIdx = data[7];
            
            // 找到序列号字符串
            if (serialIdx > 0) {
                char* s = strings;
                for (int i = 1; i < serialIdx && *s; i++) {
                    while (*s) s++;
                    s++;
                }
                printf("  System Serial: %s\n", s);
            }
        }
        
        if (type == 2 && len >= 8) {  // Baseboard
            char* strings = (char*)(data + len);
            BYTE serialIdx = data[7];
            
            if (serialIdx > 0) {
                char* s = strings;
                for (int i = 1; i < serialIdx && *s; i++) {
                    while (*s) s++;
                    s++;
                }
                printf("  Board Serial:  %s\n", s);
            }
        }
        
        // 移动到下一个结构
        data += len;
        while (data < end - 1 && !(data[0] == 0 && data[1] == 0)) data++;
        data += 2;
    }
    
    free(buffer);
}

// ============== 主函数 ==============

int main(int argc, char* argv[]) {
    printf("\n");
    printf("========================================\n");
    printf("  HWID Test Program - API Verification\n");
    printf("========================================\n");
    printf("  This program calls system APIs to\n");
    printf("  retrieve hardware information.\n");
    printf("  Use with HWIDChanger to verify spoof.\n");
    printf("========================================\n");
    
    // 运行所有测试
    test_volume_serial();
    test_disk_serial();
    test_mac_address();
    test_adapter_addresses();
    test_machine_guid();
    test_product_id();
    test_computer_name();
    test_smbios();
    
    printf("\n========================================\n");
    printf("  All tests completed!\n");
    printf("========================================\n\n");
    
    return 0;
}

