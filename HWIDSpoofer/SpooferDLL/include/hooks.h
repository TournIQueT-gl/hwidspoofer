#pragma once

#include <Windows.h>
#include <winioctl.h>
#include <iphlpapi.h>
#include <comdef.h>
#include <Wbemidl.h>

namespace hwid {
namespace hooks {

// ==================== 初始化函数 ====================
bool install_all_hooks();
void uninstall_all_hooks();

// ==================== 注册表Hook ====================
namespace registry {
    bool install();
    void uninstall();
}

// ==================== WMI Hook ====================
namespace wmi {
    bool install();
    void uninstall();
}

// ==================== 磁盘Hook ====================
namespace disk {
    bool install();
    void uninstall();
}

// ==================== 网络Hook ====================
namespace network {
    bool install();
    void uninstall();
}

// ==================== Setup API Hook ====================
namespace setupapi {
    bool install();
    void uninstall();
}

// ==================== 固件Hook ====================
namespace firmware {
    bool install();
    void uninstall();
}

// ==================== CPUID Hook ====================
namespace cpuid {
    bool install();
    void uninstall();
}

// ==================== 原始函数声明 ====================

// 注册表相关
typedef LSTATUS (WINAPI *RegQueryValueExW_t)(
    HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved,
    LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
typedef LSTATUS (WINAPI *RegQueryValueExA_t)(
    HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved,
    LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
typedef LSTATUS (WINAPI *RegGetValueW_t)(
    HKEY hkey, LPCWSTR lpSubKey, LPCWSTR lpValue,
    DWORD dwFlags, LPDWORD pdwType, PVOID pvData, LPDWORD pcbData);
typedef LSTATUS (WINAPI *RegGetValueA_t)(
    HKEY hkey, LPCSTR lpSubKey, LPCSTR lpValue,
    DWORD dwFlags, LPDWORD pdwType, PVOID pvData, LPDWORD pcbData);

extern RegQueryValueExW_t Original_RegQueryValueExW;
extern RegQueryValueExA_t Original_RegQueryValueExA;
extern RegGetValueW_t Original_RegGetValueW;
extern RegGetValueA_t Original_RegGetValueA;

// 磁盘相关
typedef BOOL (WINAPI *DeviceIoControl_t)(
    HANDLE hDevice, DWORD dwIoControlCode,
    LPVOID lpInBuffer, DWORD nInBufferSize,
    LPVOID lpOutBuffer, DWORD nOutBufferSize,
    LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);
typedef BOOL (WINAPI *GetVolumeInformationW_t)(
    LPCWSTR lpRootPathName, LPWSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags,
    LPWSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize);
typedef BOOL (WINAPI *GetVolumeInformationA_t)(
    LPCSTR lpRootPathName, LPSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags,
    LPSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize);

extern DeviceIoControl_t Original_DeviceIoControl;
extern GetVolumeInformationW_t Original_GetVolumeInformationW;
extern GetVolumeInformationA_t Original_GetVolumeInformationA;

// 网络相关
typedef DWORD (WINAPI *GetAdaptersInfo_t)(PIP_ADAPTER_INFO AdapterInfo, PULONG SizePointer);
typedef ULONG (WINAPI *GetAdaptersAddresses_t)(
    ULONG Family, ULONG Flags, PVOID Reserved,
    PIP_ADAPTER_ADDRESSES AdapterAddresses, PULONG SizePointer);

extern GetAdaptersInfo_t Original_GetAdaptersInfo;
extern GetAdaptersAddresses_t Original_GetAdaptersAddresses;

// 固件相关
typedef UINT (WINAPI *GetSystemFirmwareTable_t)(
    DWORD FirmwareTableProviderSignature,
    DWORD FirmwareTableID,
    PVOID pFirmwareTableBuffer,
    DWORD BufferSize);
typedef UINT (WINAPI *EnumSystemFirmwareTables_t)(
    DWORD FirmwareTableProviderSignature,
    PVOID pFirmwareTableEnumBuffer,
    DWORD BufferSize);

extern GetSystemFirmwareTable_t Original_GetSystemFirmwareTable;
extern EnumSystemFirmwareTables_t Original_EnumSystemFirmwareTables;

// WMI相关 - IWbemClassObject::Get
typedef HRESULT (STDMETHODCALLTYPE *IWbemClassObject_Get_t)(
    IWbemClassObject* This,
    LPCWSTR wszName,
    long lFlags,
    VARIANT* pVal,
    long* pType,
    long* plFlavor);

extern IWbemClassObject_Get_t Original_IWbemClassObject_Get;

// Setup API相关
typedef BOOL (WINAPI *SetupDiGetDeviceRegistryPropertyW_t)(
    HDEVINFO DeviceInfoSet,
    PSP_DEVINFO_DATA DeviceInfoData,
    DWORD Property,
    PDWORD PropertyRegDataType,
    PBYTE PropertyBuffer,
    DWORD PropertyBufferSize,
    PDWORD RequiredSize);

extern SetupDiGetDeviceRegistryPropertyW_t Original_SetupDiGetDeviceRegistryPropertyW;

} // namespace hooks
} // namespace hwid

