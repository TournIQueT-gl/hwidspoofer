/*
 * Native Windows hardware READBACK only.
 * The API calls here never replace data, install hooks, or modify hardware.
 */
#define _WIN32_WINNT 0x0601
#include "hwid_changer.h"
#include "hwid_readback.h"
#include <windows.h>
#include <winioctl.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define RSMB_PROVIDER 0x52534D42u
#define MAX_FIRMWARE_BYTES (4u * 1024u * 1024u)
#define MAX_STORAGE_DESCRIPTOR_BYTES (64u * 1024u)

static int copy_serial(char *out, int length, const char *source) {
    size_t count;
    if (!out || length < 2) return -1;
    out[0] = '\0';
    if (!source || !source[0]) return -1;
    count = strlen(source);
    if (count >= (size_t)length) return -1;
    memcpy(out, source, count + 1);
    return 0;
}

static int get_firmware_readback(HwidReadback *out) {
    UINT required, returned;
    unsigned char *response;
    int result = -1;
    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    required = GetSystemFirmwareTable(RSMB_PROVIDER, 0, NULL, 0);
    if (required < 8 || required > MAX_FIRMWARE_BYTES) return -1;
    response = (unsigned char *)malloc((size_t)required);
    if (!response) return -1;
    returned = GetSystemFirmwareTable(RSMB_PROVIDER, 0, response, required);
    if (returned >= 8 && returned <= required &&
        hwid_read_smbios(response, (size_t)returned, out) == 0)
        result = 0;
    free(response);
    return result;
}

int hwid_get_real_bios_serial(char *out, int length) {
    HwidReadback data = {0};
    if (!out || length < 2) return -1;
    out[0] = '\0';
    if (get_firmware_readback(&data) != 0) return -1;
    return copy_serial(out, length, data.system_serial);
}

int hwid_get_real_board_serial(char *out, int length) {
    HwidReadback data = {0};
    if (!out || length < 2) return -1;
    out[0] = '\0';
    if (get_firmware_readback(&data) != 0) return -1;
    return copy_serial(out, length, data.board_serial);
}

int hwid_get_real_disk_serial(char *out, int length) {
    HANDLE disk;
    STORAGE_PROPERTY_QUERY query = {0};
    STORAGE_DESCRIPTOR_HEADER header = {0};
    DWORD returned = 0;
    unsigned char *descriptor = NULL;
    int result = -1;
    if (!out || length < 2) return -1;
    out[0] = '\0';
    disk = CreateFileW(L"\\\\.\\PhysicalDrive0", 0,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (disk == INVALID_HANDLE_VALUE) return -1;
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    if (!DeviceIoControl(disk, IOCTL_STORAGE_QUERY_PROPERTY,
        &query, sizeof(query), &header, sizeof(header), &returned, NULL))
        goto cleanup;
    if (returned < sizeof(header) ||
        header.Size < sizeof(STORAGE_DEVICE_DESCRIPTOR) ||
        header.Size > MAX_STORAGE_DESCRIPTOR_BYTES)
        goto cleanup;
    descriptor = (unsigned char *)calloc((size_t)header.Size, 1);
    if (!descriptor) goto cleanup;
    returned = 0;
    if (!DeviceIoControl(disk, IOCTL_STORAGE_QUERY_PROPERTY,
        &query, sizeof(query), descriptor, header.Size, &returned, NULL))
        goto cleanup;
    if (hwid_read_storage_serial(descriptor, (size_t)returned,
                                 out, (size_t)length) == 0)
        result = 0;
cleanup:
    free(descriptor);
    CloseHandle(disk);
    return result;
}

int hwid_get_real_mac(uint8_t *mac) {
    ULONG length = 0;
    DWORD code;
    PIP_ADAPTER_INFO adapters, current;
    int result = -1;
    if (!mac) return -1;
    memset(mac, 0, 6);
    code = GetAdaptersInfo(NULL, &length);
    if (code != ERROR_BUFFER_OVERFLOW || !length || length > 65536u)
        return -1;
    adapters = (PIP_ADAPTER_INFO)malloc(length);
    if (!adapters) return -1;
    code = GetAdaptersInfo(adapters, &length);
    if (code == NO_ERROR) {
        for (current = adapters; current; current = current->Next) {
            if (current->AddressLength == 6) {
                memcpy(mac, current->Address, 6);
                result = 0;
                break;
            }
        }
    }
    free(adapters);
    return result;
}
