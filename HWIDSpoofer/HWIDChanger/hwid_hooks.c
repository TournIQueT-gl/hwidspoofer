/*
 * HWID Changer 用户态Hook实现
 * 完整版 - 包含所有主要Hook
 */

#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hwid_readback.h"
#include "hwid_changer.h"

#pragma comment(lib, "iphlpapi.lib")

// 全局配置
static HwidConfig g_config = {0};
static int g_hooks_installed = 0;
static CRITICAL_SECTION g_cs;  // 线程安全

// ============== 热补丁Hook实现 ==============

typedef struct {
    void* target;
    void* detour; 
    uint8_t original[16];
    int enabled;
} HotpatchHook;

#define MAX_HOOKS 16
static HotpatchHook g_hooks[MAX_HOOKS];
static int g_hook_count = 0;

static int hotpatch_install(HotpatchHook* h) {
    if (!h->target) return -1;
    DWORD old;
    if (!VirtualProtect(h->target, 16, PAGE_EXECUTE_READWRITE, &old)) return -1;
    memcpy(h->original, h->target, 16);
    
    uint8_t* p = (uint8_t*)h->target;
#ifdef _WIN64
    p[0] = 0x48; p[1] = 0xB8;
    *(uint64_t*)(p + 2) = (uint64_t)h->detour;
    p[10] = 0xFF; p[11] = 0xE0;
#else
    p[0] = 0xE9;
    *(int32_t*)(p + 1) = (int32_t)((uint8_t*)h->detour - p - 5);
#endif
    
    VirtualProtect(h->target, 16, old, &old);
    FlushInstructionCache(GetCurrentProcess(), h->target, 16);
    h->enabled = 1;
    return 0;
}

static int hotpatch_remove(HotpatchHook* h) {
    if (!h->enabled || !h->target) return 0;
    DWORD old;
    if (!VirtualProtect(h->target, 16, PAGE_EXECUTE_READWRITE, &old)) return -1;
    memcpy(h->target, h->original, 16);
    VirtualProtect(h->target, 16, old, &old);
    FlushInstructionCache(GetCurrentProcess(), h->target, 16);
    h->enabled = 0;
    return 0;
}

// 线程安全的原始函数调用
#define CALL_ORIGINAL_SAFE(hook, type, ...) ({ \
    EnterCriticalSection(&g_cs); \
    hotpatch_remove(&hook); \
    type _result = ((type)hook.target)(__VA_ARGS__); \
    hotpatch_install(&hook); \
    LeaveCriticalSection(&g_cs); \
    _result; \
})

// ============== Hook定义 ==============

static HotpatchHook hook_GetVolumeInfoW;
static HotpatchHook hook_GetVolumeInfoA;
static HotpatchHook hook_GetAdaptersInfo;
static HotpatchHook hook_GetAdaptersAddresses;
static HotpatchHook hook_RegQueryValueExW;
static HotpatchHook hook_RegQueryValueExA;
static HotpatchHook hook_DeviceIoControl;
static HotpatchHook hook_GetSystemFirmwareTable;

// Storage结构
#include <winioctl.h>
#ifndef IOCTL_STORAGE_QUERY_PROPERTY
#define IOCTL_STORAGE_QUERY_PROPERTY 0x2D1400
#endif
#define RSMB 0x52534D42  // 'RSMB'

// 使用系统定义的 STORAGE_DEVICE_DESCRIPTOR
typedef STORAGE_DEVICE_DESCRIPTOR MY_STORAGE_DEVICE_DESCRIPTOR;

// SMBIOS结构
#pragma pack(push, 1)
typedef struct {
    BYTE Used20CallingMethod;
    BYTE SMBIOSMajorVersion;
    BYTE SMBIOSMinorVersion;
    BYTE DmiRevision;
    DWORD Length;
    BYTE SMBIOSTableData[1];
} RawSMBIOSData;

typedef struct {
    BYTE Type;
    BYTE Length;
    WORD Handle;
} SMBIOS_HEADER;
#pragma pack(pop)

// 辅助：替换字符串
static void replace_string(char* dst, size_t dst_max, const char* src) {
    if (!src || !src[0]) return;
    size_t old_len = strlen(dst);
    size_t new_len = strlen(src);
    size_t copy_len = (new_len < old_len) ? new_len : old_len;
    memcpy(dst, src, copy_len);
    if (copy_len < old_len) memset(dst + copy_len, ' ', old_len - copy_len);
}

// 辅助：获取SMBIOS字符串
static char* get_smbios_string(BYTE* data, BYTE idx) {
    if (idx == 0) return NULL;
    SMBIOS_HEADER* h = (SMBIOS_HEADER*)data;
    char* s = (char*)(data + h->Length);
    for (BYTE i = 1; i < idx; i++) { while (*s) s++; s++; }
    return s;
}

// ============== Hook函数实现 ==============

// GetVolumeInformationW
typedef BOOL (WINAPI *GetVolumeInformationW_t)(LPCWSTR, LPWSTR, DWORD, LPDWORD, LPDWORD, LPDWORD, LPWSTR, DWORD);

static BOOL WINAPI Hooked_GetVolumeInformationW(
    LPCWSTR lpRootPathName, LPWSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags,
    LPWSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize)
{
    EnterCriticalSection(&g_cs);
    hotpatch_remove(&hook_GetVolumeInfoW);
    BOOL result = ((GetVolumeInformationW_t)hook_GetVolumeInfoW.target)(
        lpRootPathName, lpVolumeNameBuffer, nVolumeNameSize, lpVolumeSerialNumber,
        lpMaximumComponentLength, lpFileSystemFlags, lpFileSystemNameBuffer, nFileSystemNameSize);
    hotpatch_install(&hook_GetVolumeInfoW);
    LeaveCriticalSection(&g_cs);
    
    if (result && lpVolumeSerialNumber && (g_config.flags & FLAG_SPOOF_DISK))
        *lpVolumeSerialNumber = g_config.volume_serial;
    return result;
}

// GetVolumeInformationA
typedef BOOL (WINAPI *GetVolumeInformationA_t)(LPCSTR, LPSTR, DWORD, LPDWORD, LPDWORD, LPDWORD, LPSTR, DWORD);

static BOOL WINAPI Hooked_GetVolumeInformationA(
    LPCSTR lpRootPathName, LPSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags,
    LPSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize)
{
    EnterCriticalSection(&g_cs);
    hotpatch_remove(&hook_GetVolumeInfoA);
    BOOL result = ((GetVolumeInformationA_t)hook_GetVolumeInfoA.target)(
        lpRootPathName, lpVolumeNameBuffer, nVolumeNameSize, lpVolumeSerialNumber,
        lpMaximumComponentLength, lpFileSystemFlags, lpFileSystemNameBuffer, nFileSystemNameSize);
    hotpatch_install(&hook_GetVolumeInfoA);
    LeaveCriticalSection(&g_cs);
    
    if (result && lpVolumeSerialNumber && (g_config.flags & FLAG_SPOOF_DISK))
        *lpVolumeSerialNumber = g_config.volume_serial;
    return result;
}

// DeviceIoControl - 磁盘序列号
typedef BOOL (WINAPI *DeviceIoControl_t)(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);

static BOOL WINAPI Hooked_DeviceIoControl(
    HANDLE hDevice, DWORD dwIoControlCode,
    LPVOID lpInBuffer, DWORD nInBufferSize,
    LPVOID lpOutBuffer, DWORD nOutBufferSize,
    LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
{
    EnterCriticalSection(&g_cs);
    hotpatch_remove(&hook_DeviceIoControl);
    BOOL result = ((DeviceIoControl_t)hook_DeviceIoControl.target)(
        hDevice, dwIoControlCode, lpInBuffer, nInBufferSize,
        lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
    hotpatch_install(&hook_DeviceIoControl);
    LeaveCriticalSection(&g_cs);
    
    if (!result || !lpOutBuffer) return result;
    
    if (dwIoControlCode == IOCTL_STORAGE_QUERY_PROPERTY && (g_config.flags & FLAG_SPOOF_DISK)) {
        PSTORAGE_DEVICE_DESCRIPTOR desc = (PSTORAGE_DEVICE_DESCRIPTOR)lpOutBuffer;
        if (desc->Size >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
            if (desc->SerialNumberOffset && desc->SerialNumberOffset < nOutBufferSize)
                replace_string((char*)lpOutBuffer + desc->SerialNumberOffset, 64, g_config.disk_serial);
            if (desc->ProductIdOffset && desc->ProductIdOffset < nOutBufferSize)
                replace_string((char*)lpOutBuffer + desc->ProductIdOffset, 64, g_config.disk_model);
        }
    }
    return result;
}

// GetSystemFirmwareTable - SMBIOS
typedef UINT (WINAPI *GetSystemFirmwareTable_t)(DWORD, DWORD, PVOID, DWORD);

static UINT WINAPI Hooked_GetSystemFirmwareTable(
    DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID,
    PVOID pFirmwareTableBuffer, DWORD BufferSize)
{
    EnterCriticalSection(&g_cs);
    hotpatch_remove(&hook_GetSystemFirmwareTable);
    UINT result = ((GetSystemFirmwareTable_t)hook_GetSystemFirmwareTable.target)(
        FirmwareTableProviderSignature, FirmwareTableID, pFirmwareTableBuffer, BufferSize);
    hotpatch_install(&hook_GetSystemFirmwareTable);
    LeaveCriticalSection(&g_cs);
    
    if (result == 0 || !pFirmwareTableBuffer) return result;
    
    // 处理SMBIOS表
    if (FirmwareTableProviderSignature == RSMB && (g_config.flags & FLAG_SPOOF_BIOS)) {
        RawSMBIOSData* smbios = (RawSMBIOSData*)pFirmwareTableBuffer;
        BYTE* cur = smbios->SMBIOSTableData;
        BYTE* end = cur + smbios->Length;
        
        while (cur < end) {
            SMBIOS_HEADER* hdr = (SMBIOS_HEADER*)cur;
            if (hdr->Type == 127) break;
            
            // Type 1: System Information
            if (hdr->Type == 1 && hdr->Length >= 0x08) {
                char* serial = get_smbios_string(cur, cur[0x07]);
                if (serial) replace_string(serial, 32, g_config.bios_serial);
                // UUID at offset 0x08 (16 bytes)
                if (g_config.system_uuid[0] && hdr->Length >= 0x18) {
                    // 简化：不修改UUID结构，需要解析GUID格式
                }
            }
            // Type 2: Baseboard
            if (hdr->Type == 2 && hdr->Length >= 0x08) {
                char* serial = get_smbios_string(cur, cur[0x07]);
                if (serial) replace_string(serial, 32, g_config.board_serial);
                char* product = get_smbios_string(cur, cur[0x05]);
                if (product) replace_string(product, 64, g_config.board_product);
            }
            // Type 3: Chassis
            if (hdr->Type == 3 && hdr->Length >= 0x08) {
                char* serial = get_smbios_string(cur, cur[0x07]);
                if (serial) replace_string(serial, 32, g_config.bios_serial);
            }
            
            // 跳到下一个表
            cur += hdr->Length;
            while (cur < end - 1 && !(cur[0] == 0 && cur[1] == 0)) cur++;
            cur += 2;
        }
    }
    return result;
}

// GetAdaptersInfo - MAC地址
typedef DWORD (WINAPI *GetAdaptersInfo_t)(PIP_ADAPTER_INFO, PULONG);

static DWORD WINAPI Hooked_GetAdaptersInfo(PIP_ADAPTER_INFO AdapterInfo, PULONG SizePointer) {
    EnterCriticalSection(&g_cs);
    hotpatch_remove(&hook_GetAdaptersInfo);
    DWORD result = ((GetAdaptersInfo_t)hook_GetAdaptersInfo.target)(AdapterInfo, SizePointer);
    hotpatch_install(&hook_GetAdaptersInfo);
    LeaveCriticalSection(&g_cs);
    
    if (result == ERROR_SUCCESS && AdapterInfo && (g_config.flags & FLAG_SPOOF_NETWORK)) {
        PIP_ADAPTER_INFO curr = AdapterInfo;
        int idx = 0;
        while (curr) {
            if (curr->AddressLength == 6) {
                memcpy(curr->Address, idx == 0 ? g_config.mac_address : g_config.mac_address2, 6);
            }
            curr = curr->Next;
            idx++;
        }
    }
    return result;
}

// GetAdaptersAddresses
typedef ULONG (WINAPI *GetAdaptersAddresses_t)(ULONG, ULONG, PVOID, PIP_ADAPTER_ADDRESSES, PULONG);

static ULONG WINAPI Hooked_GetAdaptersAddresses(
    ULONG Family, ULONG Flags, PVOID Reserved,
    PIP_ADAPTER_ADDRESSES AdapterAddresses, PULONG SizePointer)
{
    EnterCriticalSection(&g_cs);
    hotpatch_remove(&hook_GetAdaptersAddresses);
    ULONG result = ((GetAdaptersAddresses_t)hook_GetAdaptersAddresses.target)(
        Family, Flags, Reserved, AdapterAddresses, SizePointer);
    hotpatch_install(&hook_GetAdaptersAddresses);
    LeaveCriticalSection(&g_cs);
    
    if (result == ERROR_SUCCESS && AdapterAddresses && (g_config.flags & FLAG_SPOOF_NETWORK)) {
        PIP_ADAPTER_ADDRESSES curr = AdapterAddresses;
        int idx = 0;
        while (curr) {
            if (curr->PhysicalAddressLength == 6) {
                memcpy(curr->PhysicalAddress, idx == 0 ? g_config.mac_address : g_config.mac_address2, 6);
            }
            curr = curr->Next;
            idx++;
        }
    }
    return result;
}

// RegQueryValueExW - MachineGuid等
typedef LSTATUS (WINAPI *RegQueryValueExW_t)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);

static LSTATUS WINAPI Hooked_RegQueryValueExW(
    HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved,
    LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    EnterCriticalSection(&g_cs);
    hotpatch_remove(&hook_RegQueryValueExW);
    LSTATUS result = ((RegQueryValueExW_t)hook_RegQueryValueExW.target)(
        hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    hotpatch_install(&hook_RegQueryValueExW);
    LeaveCriticalSection(&g_cs);
    
    if (result == ERROR_SUCCESS && lpValueName && lpData && lpcbData && (g_config.flags & FLAG_SPOOF_SYSTEM)) {
        if (wcscmp(lpValueName, L"MachineGuid") == 0 && g_config.machine_guid[0]) {
            wchar_t wguid[64];
            MultiByteToWideChar(CP_UTF8, 0, g_config.machine_guid, -1, wguid, 64);
            size_t len = (wcslen(wguid) + 1) * sizeof(wchar_t);
            if (*lpcbData >= len) { wcscpy((wchar_t*)lpData, wguid); *lpcbData = (DWORD)len; }
        }
        else if (wcscmp(lpValueName, L"ProductId") == 0 && g_config.product_id[0]) {
            wchar_t wpid[64];
            MultiByteToWideChar(CP_UTF8, 0, g_config.product_id, -1, wpid, 64);
            size_t len = (wcslen(wpid) + 1) * sizeof(wchar_t);
            if (*lpcbData >= len) { wcscpy((wchar_t*)lpData, wpid); *lpcbData = (DWORD)len; }
        }
    }
    return result;
}

// RegQueryValueExA
typedef LSTATUS (WINAPI *RegQueryValueExA_t)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);

static LSTATUS WINAPI Hooked_RegQueryValueExA(
    HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved,
    LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    EnterCriticalSection(&g_cs);
    hotpatch_remove(&hook_RegQueryValueExA);
    LSTATUS result = ((RegQueryValueExA_t)hook_RegQueryValueExA.target)(
        hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    hotpatch_install(&hook_RegQueryValueExA);
    LeaveCriticalSection(&g_cs);
    
    if (result == ERROR_SUCCESS && lpValueName && lpData && lpcbData && (g_config.flags & FLAG_SPOOF_SYSTEM)) {
        if (strcmp(lpValueName, "MachineGuid") == 0 && g_config.machine_guid[0]) {
            size_t len = strlen(g_config.machine_guid) + 1;
            if (*lpcbData >= len) { strcpy((char*)lpData, g_config.machine_guid); *lpcbData = (DWORD)len; }
        }
    }
    return result;
}

// ============== 安装/卸载 ==============

static int install_hooks(void) {
    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    HMODULE iphlp = LoadLibraryW(L"iphlpapi.dll");
    HMODULE advapi = GetModuleHandleW(L"advapi32.dll");
    
    InitializeCriticalSection(&g_cs);
    
    hook_GetVolumeInfoW.target = GetProcAddress(k32, "GetVolumeInformationW");
    hook_GetVolumeInfoW.detour = Hooked_GetVolumeInformationW;
    hotpatch_install(&hook_GetVolumeInfoW);
    
    hook_GetVolumeInfoA.target = GetProcAddress(k32, "GetVolumeInformationA");
    hook_GetVolumeInfoA.detour = Hooked_GetVolumeInformationA;
    hotpatch_install(&hook_GetVolumeInfoA);
    
    hook_DeviceIoControl.target = GetProcAddress(k32, "DeviceIoControl");
    hook_DeviceIoControl.detour = Hooked_DeviceIoControl;
    hotpatch_install(&hook_DeviceIoControl);
    
    hook_GetSystemFirmwareTable.target = GetProcAddress(k32, "GetSystemFirmwareTable");
    hook_GetSystemFirmwareTable.detour = Hooked_GetSystemFirmwareTable;
    hotpatch_install(&hook_GetSystemFirmwareTable);
    
    if (iphlp) {
        hook_GetAdaptersInfo.target = GetProcAddress(iphlp, "GetAdaptersInfo");
        hook_GetAdaptersInfo.detour = Hooked_GetAdaptersInfo;
        hotpatch_install(&hook_GetAdaptersInfo);
        
        hook_GetAdaptersAddresses.target = GetProcAddress(iphlp, "GetAdaptersAddresses");
        hook_GetAdaptersAddresses.detour = Hooked_GetAdaptersAddresses;
        hotpatch_install(&hook_GetAdaptersAddresses);
    }
    
    if (advapi) {
        hook_RegQueryValueExW.target = GetProcAddress(advapi, "RegQueryValueExW");
        hook_RegQueryValueExW.detour = Hooked_RegQueryValueExW;
        hotpatch_install(&hook_RegQueryValueExW);
        
        hook_RegQueryValueExA.target = GetProcAddress(advapi, "RegQueryValueExA");
        hook_RegQueryValueExA.detour = Hooked_RegQueryValueExA;
        hotpatch_install(&hook_RegQueryValueExA);
    }
    
    return 0;
}

static void remove_hooks(void) {
    hotpatch_remove(&hook_GetVolumeInfoW);
    hotpatch_remove(&hook_GetVolumeInfoA);
    hotpatch_remove(&hook_DeviceIoControl);
    hotpatch_remove(&hook_GetSystemFirmwareTable);
    hotpatch_remove(&hook_GetAdaptersInfo);
    hotpatch_remove(&hook_GetAdaptersAddresses);
    hotpatch_remove(&hook_RegQueryValueExW);
    hotpatch_remove(&hook_RegQueryValueExA);
    DeleteCriticalSection(&g_cs);
}

// ============== 公开API ==============

void hwid_set_config(const HwidConfig* config) {
    memcpy(&g_config, config, sizeof(HwidConfig));
}

int hwid_apply(const HwidConfig* config, RunMode mode) {
    if (config) memcpy(&g_config, config, sizeof(HwidConfig));
    if (g_hooks_installed) return 0;
    
    if (mode == MODE_KERNEL || mode == MODE_HYBRID) {
        if (hwid_load_driver() == 0) {
            hwid_driver_set_config(config);
            if (mode == MODE_KERNEL) { g_hooks_installed = 1; return 0; }
        }
    }
    
    if (mode == MODE_USERMODE || mode == MODE_HYBRID || mode == MODE_AUTO) {
        install_hooks();
        g_hooks_installed = 1;
    }
    return 0;
}

int hwid_revert(void) {
    if (g_hooks_installed) {
        remove_hooks();
        hwid_unload_driver();
        g_hooks_installed = 0;
    }
    return 0;
}

int hwid_load_driver(void) {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* p = strrchr(path, '\\');
    if (p) strcpy(p + 1, "HWIDChanger.sys");
    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) return -1;
    
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm) return -1;
    SC_HANDLE svc = OpenServiceA(scm, "HWIDChanger", SERVICE_ALL_ACCESS);
    if (!svc) svc = CreateServiceA(scm, "HWIDChanger", "HWID Changer",
        SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL, path, NULL, NULL, NULL, NULL, NULL);
    if (!svc) { CloseServiceHandle(scm); return -1; }
    StartService(svc, 0, NULL);
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return 0;
}

int hwid_unload_driver(void) {
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm) return -1;
    SC_HANDLE svc = OpenServiceA(scm, "HWIDChanger", SERVICE_ALL_ACCESS);
    if (svc) { SERVICE_STATUS st; ControlService(svc, SERVICE_CONTROL_STOP, &st); DeleteService(svc); CloseServiceHandle(svc); }
    CloseServiceHandle(scm);
    return 0;
}

int hwid_driver_set_config(const HwidConfig* config) {
    HANDLE h = CreateFileW(L"\\\\.\\HWIDChanger", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;
    DWORD ret;
    BOOL ok = DeviceIoControl(h, 0x80000800, (LPVOID)config, sizeof(HwidConfig), NULL, 0, &ret, NULL);
    if (ok) DeviceIoControl(h, 0x80000802, NULL, 0, NULL, 0, &ret, NULL);
    CloseHandle(h);
    return ok ? 0 : -1;
}

/* Non-mutating storage serial inspection.  Windows virtual disks may expose
   no serial; failure must not be reported as a changed identifier. */
int hwid_get_real_disk_serial(char* buf, int len) {
    if (!buf || len < 2) return -1;
    buf[0] = 0;
    HANDLE device = CreateFileW(L"\\\\.\\PhysicalDrive0", 0,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL, OPEN_EXISTING, 0, NULL);
    if (device == INVALID_HANDLE_VALUE) return -1;
    STORAGE_PROPERTY_QUERY query = {0};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    BYTE result_buffer[4096] = {0};
    DWORD bytes_returned = 0;
    int was = hook_DeviceIoControl.enabled;
    if (was) EnterCriticalSection(&g_cs);
    if (was) hotpatch_remove(&hook_DeviceIoControl);
    BOOL success = DeviceIoControl(device, IOCTL_STORAGE_QUERY_PROPERTY,
                         &query, sizeof(query), result_buffer,
                         sizeof(result_buffer), &bytes_returned, NULL);
    if (was) hotpatch_install(&hook_DeviceIoControl);
    if (was) LeaveCriticalSection(&g_cs);
    CloseHandle(device);
    if (!success) return -1;
    return hwid_read_storage_serial(result_buffer, bytes_returned, buf, (size_t)len) == 0 ? 0 : -1;
}

int hwid_get_real_mac(uint8_t* mac) {
    IP_ADAPTER_INFO a[16]; ULONG sz = sizeof(a);
    int was = hook_GetAdaptersInfo.enabled;
    if (was) EnterCriticalSection(&g_cs);
    if (was) hotpatch_remove(&hook_GetAdaptersInfo);
    DWORD r = GetAdaptersInfo(a, &sz);
    if (was) hotpatch_install(&hook_GetAdaptersInfo);
    if (was) LeaveCriticalSection(&g_cs);
    if (r == ERROR_SUCCESS && a[0].AddressLength == 6) { memcpy(mac, a[0].Address, 6); return 0; }
    return -1;
}

/* Type 1 is the system serial displayed by many BIOS/WMI providers.
   It is read from firmware, not changed or fabricated. */
int hwid_get_real_bios_serial(char* buf, int len) {
    if (!buf || len < 2) return -1;
    buf[0] = 0;
    UINT bytes = GetSystemFirmwareTable(RSMB, 0, NULL, 0);
    if (bytes < 8 || bytes > 4u * 1024u * 1024u) return -1;
    BYTE *memory = (BYTE*)malloc(bytes);
    if (!memory) return -1;
    UINT received = GetSystemFirmwareTable(RSMB, 0, memory, bytes);
    HwidReadback info = {0};
    int result = -1;
    if (received >= 8 && received <= bytes &&
        hwid_read_smbios(memory, received, &info) == 0 &&
        info.system_serial[0]) {
        size_t amount = strlen(info.system_serial);
        if (amount < (size_t)len) {
            memcpy(buf, info.system_serial, amount + 1);
            result = 0;
        }
    }
    free(memory);
    return result;
}
int hwid_get_real_uuid(char* buf, int len) { buf[0] = 0; return -1; }

// ============== 全局模式（内核级别）==============

// IOCTL定义（需要与驱动端一致）
#define IOCTL_ENABLE_GLOBAL     0x80000820
#define IOCTL_DISABLE_GLOBAL    0x80000821
#define IOCTL_SET_CONFIG        0x80000800
#define IOCTL_ENABLE            0x80000802

static int g_global_mode_active = 0;

int hwid_enable_global_mode(const HwidConfig* config) {
    // 首先确保驱动已加载
    if (hwid_load_driver() != 0) {
        return -1;
    }
    
    // 打开驱动设备
    HANDLE h = CreateFileW(L"\\\\.\\HWIDSpoofer", 
        GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        return -1;
    }
    
    DWORD ret;
    BOOL ok;
    
    // 1. 设置配置
    ok = DeviceIoControl(h, IOCTL_SET_CONFIG, (LPVOID)config, 
        sizeof(HwidConfig), NULL, 0, &ret, NULL);
    if (!ok) {
        CloseHandle(h);
        return -1;
    }
    
    // 2. 启用伪装
    DeviceIoControl(h, IOCTL_ENABLE, NULL, 0, NULL, 0, &ret, NULL);
    
    // 3. 启用全局拦截模式
    ok = DeviceIoControl(h, IOCTL_ENABLE_GLOBAL, NULL, 0, NULL, 0, &ret, NULL);
    
    CloseHandle(h);
    
    if (ok) {
        g_global_mode_active = 1;
    }
    
    return ok ? 0 : -1;
}

int hwid_disable_global_mode(void) {
    HANDLE h = CreateFileW(L"\\\\.\\HWIDSpoofer",
        GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        return -1;
    }
    
    DWORD ret;
    BOOL ok = DeviceIoControl(h, IOCTL_DISABLE_GLOBAL, NULL, 0, NULL, 0, &ret, NULL);
    
    CloseHandle(h);
    
    if (ok) {
        g_global_mode_active = 0;
    }
    
    return ok ? 0 : -1;
}

int hwid_is_global_mode_active(void) {
    return g_global_mode_active;
}
