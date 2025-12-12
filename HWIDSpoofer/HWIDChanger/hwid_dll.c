/*
 * HWID Changer DLL - Process Injection Module
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "iphlpapi.lib")

#define MAX_SERIAL_LEN  64
#define MAX_UUID_LEN    48
#define FLAG_SPOOF_DISK     0x0001
#define FLAG_SPOOF_NETWORK  0x0010
#define FLAG_SPOOF_SYSTEM   0x0008
#define FLAG_ALL            0xFFFF
#define IOCTL_STORAGE_QUERY_PROPERTY 0x2D1400

typedef struct {
    DWORD flags;
    char disk_serial[MAX_SERIAL_LEN];
    DWORD volume_serial;
    char machine_guid[MAX_UUID_LEN];
    BYTE mac_address[6];
    BYTE mac_address2[6];
} DllConfig;

static DllConfig g_cfg = {0};
static CRITICAL_SECTION g_cs;

typedef struct { void* target; void* detour; BYTE orig[16]; int on; } Hook;
static Hook hooks[6];

static int hook_on(Hook* h) {
    DWORD old;
    if (!VirtualProtect(h->target, 16, PAGE_EXECUTE_READWRITE, &old)) return -1;
    memcpy(h->orig, h->target, 16);
    BYTE* p = (BYTE*)h->target;
#ifdef _WIN64
    p[0] = 0x48; p[1] = 0xB8; *(UINT64*)(p+2) = (UINT64)h->detour; p[10] = 0xFF; p[11] = 0xE0;
#else
    p[0] = 0xE9; *(INT32*)(p+1) = (INT32)((BYTE*)h->detour - p - 5);
#endif
    VirtualProtect(h->target, 16, old, &old);
    FlushInstructionCache(GetCurrentProcess(), h->target, 16);
    h->on = 1; return 0;
}

static void hook_off(Hook* h) {
    if (!h->on) return;
    DWORD old;
    VirtualProtect(h->target, 16, PAGE_EXECUTE_READWRITE, &old);
    memcpy(h->target, h->orig, 16);
    VirtualProtect(h->target, 16, old, &old);
    FlushInstructionCache(GetCurrentProcess(), h->target, 16);
    h->on = 0;
}

typedef BOOL (WINAPI *pGetVolInfo)(LPCWSTR, LPWSTR, DWORD, LPDWORD, LPDWORD, LPDWORD, LPWSTR, DWORD);
static BOOL WINAPI HookGetVolInfo(LPCWSTR r, LPWSTR n, DWORD ns, LPDWORD s, LPDWORD ml, LPDWORD fl, LPWSTR fs, DWORD fss) {
    EnterCriticalSection(&g_cs); hook_off(&hooks[0]);
    BOOL ret = ((pGetVolInfo)hooks[0].target)(r,n,ns,s,ml,fl,fs,fss);
    hook_on(&hooks[0]); LeaveCriticalSection(&g_cs);
    if (ret && s && (g_cfg.flags & FLAG_SPOOF_DISK)) *s = g_cfg.volume_serial;
    return ret;
}

typedef BOOL (WINAPI *pDevIoCtrl)(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
static BOOL WINAPI HookDevIoCtrl(HANDLE h, DWORD c, LPVOID in, DWORD is, LPVOID out, DWORD os, LPDWORD ret, LPOVERLAPPED o) {
    EnterCriticalSection(&g_cs); hook_off(&hooks[1]);
    BOOL r = ((pDevIoCtrl)hooks[1].target)(h,c,in,is,out,os,ret,o);
    hook_on(&hooks[1]); LeaveCriticalSection(&g_cs);
    if (r && c == IOCTL_STORAGE_QUERY_PROPERTY && out && os >= 32 && (g_cfg.flags & FLAG_SPOOF_DISK)) {
        DWORD off = *(DWORD*)((BYTE*)out + 24);
        if (off && off < os) strncpy((char*)out + off, g_cfg.disk_serial, 32);
    }
    return r;
}

typedef DWORD (WINAPI *pGetAdapters)(PIP_ADAPTER_INFO, PULONG);
static DWORD WINAPI HookGetAdapters(PIP_ADAPTER_INFO ai, PULONG sz) {
    EnterCriticalSection(&g_cs); hook_off(&hooks[2]);
    DWORD r = ((pGetAdapters)hooks[2].target)(ai, sz);
    hook_on(&hooks[2]); LeaveCriticalSection(&g_cs);
    if (r == ERROR_SUCCESS && ai && (g_cfg.flags & FLAG_SPOOF_NETWORK)) {
        int i = 0; for (PIP_ADAPTER_INFO c = ai; c; c = c->Next, i++)
            if (c->AddressLength == 6) memcpy(c->Address, i ? g_cfg.mac_address2 : g_cfg.mac_address, 6);
    }
    return r;
}

typedef LSTATUS (WINAPI *pRegQuery)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
static LSTATUS WINAPI HookRegQuery(HKEY hk, LPCWSTR nm, LPDWORD rs, LPDWORD tp, LPBYTE dt, LPDWORD cb) {
    EnterCriticalSection(&g_cs); hook_off(&hooks[3]);
    LSTATUS r = ((pRegQuery)hooks[3].target)(hk,nm,rs,tp,dt,cb);
    hook_on(&hooks[3]); LeaveCriticalSection(&g_cs);
    if (r == ERROR_SUCCESS && nm && dt && cb && (g_cfg.flags & FLAG_SPOOF_SYSTEM)) {
        if (wcscmp(nm, L"MachineGuid") == 0 && g_cfg.machine_guid[0]) {
            wchar_t w[64]; MultiByteToWideChar(CP_UTF8, 0, g_cfg.machine_guid, -1, w, 64);
            size_t len = (wcslen(w) + 1) * 2;
            if (*cb >= len) { wcscpy((wchar_t*)dt, w); *cb = (DWORD)len; }
        }
    }
    return r;
}

static void load_config(void) {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* p = strrchr(path, '\\'); if (p) strcpy(p + 1, "hwid.cfg"); else strcpy(path, "hwid.cfg");
    FILE* f = fopen(path, "r");
    g_cfg.flags = FLAG_ALL;
    strcpy(g_cfg.disk_serial, "SPOOFED12345");
    g_cfg.volume_serial = 0xDEADBEEF;
    g_cfg.mac_address[0]=0x00;g_cfg.mac_address[1]=0x11;g_cfg.mac_address[2]=0x22;
    g_cfg.mac_address[3]=0x33;g_cfg.mac_address[4]=0x44;g_cfg.mac_address[5]=0x55;
    memcpy(g_cfg.mac_address2, g_cfg.mac_address, 6);
    strcpy(g_cfg.machine_guid, "12345678-1234-1234-1234-123456789ABC");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        char key[64], val[192];
        if (sscanf(line, "%63[^=]=%191[^\n]", key, val) == 2) {
            char* v = val; while (*v == ' ' || *v == '"') v++;
            char* e = v + strlen(v) - 1; while (e > v && (*e == '"' || *e == ' ' || *e == '\r')) *e-- = 0;
            if (strcmp(key, "flags") == 0) g_cfg.flags = (DWORD)strtoul(v, NULL, 0);
            else if (strcmp(key, "disk_serial") == 0) strncpy(g_cfg.disk_serial, v, MAX_SERIAL_LEN - 1);
            else if (strcmp(key, "volume_serial") == 0) g_cfg.volume_serial = (DWORD)strtoul(v, NULL, 0);
            else if (strcmp(key, "machine_guid") == 0) strncpy(g_cfg.machine_guid, v, MAX_UUID_LEN - 1);
            else if (strcmp(key, "mac_address") == 0) {
                unsigned int m[6];
                if (sscanf(v, "%02X:%02X:%02X:%02X:%02X:%02X", &m[0],&m[1],&m[2],&m[3],&m[4],&m[5]) == 6)
                    for (int i = 0; i < 6; i++) g_cfg.mac_address[i] = (BYTE)m[i];
            }
        }
    }
    fclose(f);
}

static void install_hooks(void) {
    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    HMODULE ip = LoadLibraryW(L"iphlpapi.dll");
    HMODULE adv = GetModuleHandleW(L"advapi32.dll");
    hooks[0].target = GetProcAddress(k32, "GetVolumeInformationW");
    hooks[0].detour = HookGetVolInfo;
    hooks[1].target = GetProcAddress(k32, "DeviceIoControl");
    hooks[1].detour = HookDevIoCtrl;
    if (ip) { hooks[2].target = GetProcAddress(ip, "GetAdaptersInfo"); hooks[2].detour = HookGetAdapters; }
    if (adv) { hooks[3].target = GetProcAddress(adv, "RegQueryValueExW"); hooks[3].detour = HookRegQuery; }
    for (int i = 0; i < 4; i++) if (hooks[i].target) hook_on(&hooks[i]);
}

__declspec(dllexport) void HWID_SetConfig(const DllConfig* cfg) { if (cfg) memcpy(&g_cfg, cfg, sizeof(DllConfig)); }

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) { DisableThreadLibraryCalls(hModule); InitializeCriticalSection(&g_cs); load_config(); install_hooks(); }
    else if (reason == DLL_PROCESS_DETACH) { for (int i = 0; i < 4; i++) hook_off(&hooks[i]); DeleteCriticalSection(&g_cs); }
    return TRUE;
}

