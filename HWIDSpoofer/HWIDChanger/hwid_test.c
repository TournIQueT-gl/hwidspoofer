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
#include <stdlib.h>
#include "hwid_readback.h"

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
    printf("\n[TEST 2] STORAGE_DEVICE_DESCRIPTOR - disk serial\n");
    HANDLE disk = CreateFileW(L"\\\\.\\PhysicalDrive0", 0,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, NULL);
    if (disk == INVALID_HANDLE_VALUE) {
        printf("  NOT VERIFIED: PhysicalDrive0 could not be opened (%lu)\n",
               (unsigned long)GetLastError());
        return;
    }
    STORAGE_PROPERTY_QUERY query = {0};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    BYTE bytes[4096] = {0};
    DWORD returned = 0;
    BOOL ok = DeviceIoControl(disk, IOCTL_STORAGE_QUERY_PROPERTY,
                              &query, sizeof(query), bytes, sizeof(bytes), &returned, NULL);
    CloseHandle(disk);
    if (!ok) {
        printf("  NOT VERIFIED: storage query failed (%lu)\n",
               (unsigned long)GetLastError());
        return;
    }
    char serial[HWID_SERIAL_CAP] = {0};
    int result = hwid_read_storage_serial(bytes, returned, serial, sizeof(serial));
    if (result == 0)
        printf("  Disk serial: %s\n", serial);
    else if (result == 1)
        printf("  NOT VERIFIED: storage device exposes no serial number\n");
    else
        printf("  NOT VERIFIED: invalid/truncated storage descriptor\n");
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
    printf("\n[TEST 8] GetSystemFirmwareTable - SMBIOS readback\n");
    DWORD capacity = GetSystemFirmwareTable(RSMB_SIGNATURE, 0, NULL, 0);
    if (capacity < 8 || capacity > 4 * 1024 * 1024) {
        printf("  NOT VERIFIED: SMBIOS response unavailable or invalid (%lu)\n",
               (unsigned long)capacity);
        return;
    }
    BYTE* bytes = (BYTE*)malloc(capacity);
    if (!bytes) {
        printf("  NOT VERIFIED: out of memory\n");
        return;
    }
    DWORD received = GetSystemFirmwareTable(RSMB_SIGNATURE, 0, bytes, capacity);
    if (received < 8 || received > capacity) {
        printf("  NOT VERIFIED: SMBIOS query failed or grew unexpectedly (%lu)\n",
               (unsigned long)GetLastError());
        free(bytes);
        return;
    }
    HwidReadback hwid = {0};
    if (hwid_read_smbios(bytes, received, &hwid) != 0) {
        printf("  NOT VERIFIED: invalid SMBIOS table layout\n");
    } else {
        printf("  System serial: %s\n", hwid.system_serial[0] ? hwid.system_serial : "(not exposed)");
        printf("  Board serial:  %s\n", hwid.board_serial[0] ? hwid.board_serial : "(not exposed)");
        printf("  BIOS vendor:   %s\n", hwid.bios_vendor[0] ? hwid.bios_vendor : "(not exposed)");
        printf("  BIOS version:  %s\n", hwid.bios_version[0] ? hwid.bios_version : "(not exposed)");
    }
    free(bytes);
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

