/*
 * AppInit configuration management.
 * Back up settings durably before touching system-wide DLL loading.
 * Never clear another application's configuration without a valid backup.
 */
#include "hwid_changer.h"
#include <stdio.h>
#include <string.h>

#define APPINIT_KEY "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows"
#define APPINIT_BACKUP_FILE "hwid_appinit_backup.dat"
#define APPINIT_BACKUP_VERSION 1
#define HAS_DLLS    0x01
#define HAS_LOAD    0x02
#define HAS_SIGNED  0x04

typedef struct {
    char magic[8];
    DWORD version;
    DWORD present_flags;
    char original_dlls[512];
    DWORD original_load;
    DWORD original_signed;
} AppInitBackup;

static AppInitBackup g_backup = {0};
static BOOL g_has_backup = FALSE;

static int read_string(HKEY key, const char* name, char* out, DWORD capacity, DWORD* present)
{
    DWORD type = 0, size = capacity;
    LONG rc;
    memset(out, 0, capacity);
    rc = RegQueryValueExA(key, name, NULL, &type, (LPBYTE)out, &size);
    if (rc == ERROR_FILE_NOT_FOUND) { *present = 0; return 0; }
    if (rc != ERROR_SUCCESS || type != REG_SZ ||
        !memchr(out, '\0', size)) return -1;
    *present = 1;
    return 0;
}

static int read_dword(HKEY key, const char* name, DWORD* out, DWORD* present)
{
    DWORD type = 0, size = sizeof(DWORD);
    LONG rc = RegQueryValueExA(key, name, NULL, &type, (LPBYTE)out, &size);
    if (rc == ERROR_FILE_NOT_FOUND) { *present = 0; return 0; }
    if (rc != ERROR_SUCCESS || type != REG_DWORD || size != sizeof(DWORD)) return -1;
    *present = 1;
    return 0;
}

static int apply_value(const char* name, DWORD type, const void* data, DWORD length, BOOL exists)
{
    HKEY key;
    LONG rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, APPINIT_KEY, 0, KEY_SET_VALUE, &key);
    if (rc != ERROR_SUCCESS) return -1;
    if (exists)
        rc = RegSetValueExA(key, name, 0, type, (const BYTE*)data, length);
    else
        rc = RegDeleteValueA(key, name);
    RegCloseKey(key);
    return (rc == ERROR_SUCCESS || (!exists && rc == ERROR_FILE_NOT_FOUND)) ? 0 : -1;
}

static int capture_appinit(AppInitBackup* out)
{
    HKEY key;
    DWORD exists = 0;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, APPINIT_KEY, 0, KEY_READ, &key) != ERROR_SUCCESS)
        return -1;
    memset(out, 0, sizeof(*out));
    memcpy(out->magic, "APPBK001", 8);
    out->version = APPINIT_BACKUP_VERSION;

    if (read_string(key, "AppInit_DLLs", out->original_dlls,
                    sizeof(out->original_dlls), &exists) != 0) goto fail;
    if (exists) out->present_flags |= HAS_DLLS;
    if (read_dword(key, "LoadAppInit_DLLs", &out->original_load, &exists) != 0) goto fail;
    if (exists) out->present_flags |= HAS_LOAD;
    if (read_dword(key, "RequireSignedAppInit_DLLs", &out->original_signed, &exists) != 0) goto fail;
    if (exists) out->present_flags |= HAS_SIGNED;
    RegCloseKey(key);
    return 0;
fail:
    RegCloseKey(key);
    return -1;
}

static int load_appinit_backup(void)
{
    HANDLE file = CreateFileA(APPINIT_BACKUP_FILE, GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return -1;

    AppInitBackup snapshot = {0};
    LARGE_INTEGER file_size;
    DWORD read = 0;
    BOOL valid = GetFileSizeEx(file, &file_size) &&
                 file_size.QuadPart == sizeof(snapshot) &&
                 ReadFile(file, &snapshot, sizeof(snapshot), &read, NULL) &&
                 read == sizeof(snapshot);
    CloseHandle(file);
    if (!valid || memcmp(snapshot.magic, "APPBK001", 8) != 0 ||
        snapshot.version != APPINIT_BACKUP_VERSION ||
        (snapshot.present_flags & ~(HAS_DLLS | HAS_LOAD | HAS_SIGNED)) != 0 ||
        !memchr(snapshot.original_dlls, '\0', sizeof(snapshot.original_dlls)))
        return -1;
    g_backup = snapshot;
    g_has_backup = TRUE;
    return 0;
}

static int persist_appinit_backup(const AppInitBackup* snapshot)
{
    DWORD attrs = GetFileAttributesA(APPINIT_BACKUP_FILE);
    if (attrs != INVALID_FILE_ATTRIBUTES)
        return load_appinit_backup();
    if (GetLastError() != ERROR_FILE_NOT_FOUND) return -1;

    char temp[MAX_PATH];
    int count = snprintf(temp, sizeof(temp), "%s.tmp.%lu.%lu",
                         APPINIT_BACKUP_FILE, (unsigned long)GetCurrentProcessId(),
                         (unsigned long)GetTickCount());
    if (count < 0 || count >= (int)sizeof(temp)) return -1;

    HANDLE file = CreateFileA(temp, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
    if (file == INVALID_HANDLE_VALUE) return -1;
    DWORD written = 0;
    BOOL ok = WriteFile(file, snapshot, sizeof(*snapshot), &written, NULL) &&
              written == sizeof(*snapshot) && FlushFileBuffers(file);
    if (!CloseHandle(file)) ok = FALSE;
    if (!ok || !MoveFileExA(temp, APPINIT_BACKUP_FILE, MOVEFILE_WRITE_THROUGH)) {
        DeleteFileA(temp);
        return -1;
    }
    g_backup = *snapshot;
    g_has_backup = TRUE;
    return 0;
}

int hwid_backup_appinit(void)
{
    if (g_has_backup) return 0;
    DWORD attrs = GetFileAttributesA(APPINIT_BACKUP_FILE);
    if (attrs != INVALID_FILE_ATTRIBUTES)
        return load_appinit_backup();
    if (GetLastError() != ERROR_FILE_NOT_FOUND) return -1;

    AppInitBackup snapshot;
    if (capture_appinit(&snapshot) != 0) return -1;
    return persist_appinit_backup(&snapshot);
}

int hwid_enable_appinit_injection(const char* dll_path)
{
    if (!dll_path || !*dll_path || GetFileAttributesA(dll_path) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "  [ERROR] DLL path does not exist.\n");
        return -1;
    }
    if (hwid_backup_appinit() != 0) {
        fprintf(stderr, "  [ERROR] Cannot save AppInit originals; refusing changes.\n");
        return -1;
    }
    /* Never replace an unrelated DLL list. */
    if (g_backup.original_dlls[0]) {
        fprintf(stderr, "  [ERROR] Existing AppInit DLLs are configured; refusing to replace them.\n");
        return -1;
    }
    if (apply_value("AppInit_DLLs", REG_SZ, dll_path,
                    (DWORD)strlen(dll_path) + 1, TRUE) != 0)
        return -1;
    DWORD enabled = 1;
    if (apply_value("LoadAppInit_DLLs", REG_DWORD,
                    &enabled, sizeof(enabled), TRUE) != 0) {
        hwid_disable_appinit_injection();
        return -1;
    }
    /* Do not lower Windows' RequireSignedAppInit_DLLs setting. */
    printf("  AppInit configuration updated; Windows signing policy unchanged.\n");
    return 0;
}

int hwid_disable_appinit_injection(void)
{
    if (!g_has_backup && load_appinit_backup() != 0) {
        fprintf(stderr, "  [ERROR] No valid AppInit backup. Refusing to clear system settings.\n");
        return -1;
    }
    int errors = 0;
    errors += apply_value("AppInit_DLLs", REG_SZ, g_backup.original_dlls,
                           (DWORD)strlen(g_backup.original_dlls) + 1,
                           (g_backup.present_flags & HAS_DLLS) != 0) != 0;
    errors += apply_value("LoadAppInit_DLLs", REG_DWORD, &g_backup.original_load,
                           sizeof(DWORD), (g_backup.present_flags & HAS_LOAD) != 0) != 0;
    /* We never modify RequireSignedAppInit_DLLs; leave any third-party setting intact. */
    if (errors) {
        fprintf(stderr, "  [ERROR] AppInit restore incomplete (%d registry errors).\n", errors);
        return -1;
    }
    printf("  Original AppInit settings restored.\n");
    return 0;
}

int hwid_is_appinit_enabled(void)
{
    HKEY key;
    DWORD value = 0, present = 0;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, APPINIT_KEY, 0, KEY_READ, &key) != ERROR_SUCCESS)
        return 0;
    int result = read_dword(key, "LoadAppInit_DLLs", &value, &present);
    RegCloseKey(key);
    return result == 0 && present && value != 0;
}

int hwid_get_appinit_dlls(char* buf, int len)
{
    HKEY key;
    DWORD present = 0;
    if (!buf || len <= 0 ||
        RegOpenKeyExA(HKEY_LOCAL_MACHINE, APPINIT_KEY, 0, KEY_READ, &key) != ERROR_SUCCESS)
        return -1;
    int result = read_string(key, "AppInit_DLLs", buf, (DWORD)len, &present);
    RegCloseKey(key);
    return result;
}
