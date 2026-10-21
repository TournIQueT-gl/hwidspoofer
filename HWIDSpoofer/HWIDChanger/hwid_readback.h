#ifndef HWID_READBACK_H
#define HWID_READBACK_H
#include <stddef.h>
#define HWID_SERIAL_CAP 128
typedef struct HwidReadback {
    char system_serial[HWID_SERIAL_CAP]; /* SMBIOS type 1, often Win32_BIOS.SerialNumber */
    char board_serial[HWID_SERIAL_CAP];  /* SMBIOS type 2 */
    char bios_vendor[HWID_SERIAL_CAP];   /* SMBIOS type 0 */
    char bios_version[HWID_SERIAL_CAP];  /* SMBIOS type 0 */
} HwidReadback;
/* Parse a Windows RawSMBIOSData response without changing any fields.
 * Returns 0 for structurally valid data; -1 for malformed input.
 * Missing fields of otherwise valid data are empty strings. */
int hwid_read_smbios(const void *response, size_t bytes, HwidReadback *out);
/* Parse STORAGE_DEVICE_DESCRIPTOR returned for StorageDeviceProperty.
 * Returns 0 for valid serial, 1 if absent, -1 if malformed. */
int hwid_read_storage_serial(const void *response, size_t bytes,
                             char *out, size_t output_size);
#endif
