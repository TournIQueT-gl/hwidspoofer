#include "../hwid_readback.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void set_u32(unsigned char *p, uint32_t n) {
    p[0] = (unsigned char)n;
    p[1] = (unsigned char)(n >> 8);
    p[2] = (unsigned char)(n >> 16);
    p[3] = (unsigned char)(n >> 24);
}
static size_t record(unsigned char *p, int type, int idx,
                     const char *strings, size_t count) {
    size_t size = 8 + count;
    memset(p, 0, size);
    p[0] = (unsigned char)type;
    p[1] = 8;
    p[7] = (unsigned char)idx;
    memcpy(p + 8, strings, count);
    return size;
}
int main(void) {
    unsigned char disk[96] = {0}, table[256] = {0};
    HwidReadback info;
    char serial[32];
    size_t off = 8;
    set_u32(disk + 4, sizeof(disk));
    set_u32(disk + 24, 36);
    memcpy(disk + 36, "VIRTUAL-SERIAL\0", 15);
    assert(hwid_read_storage_serial(disk, sizeof(disk), serial, sizeof(serial)) == 0);
    assert(strcmp(serial, "VIRTUAL-SERIAL") == 0);
    set_u32(disk + 24, 0);
    assert(hwid_read_storage_serial(disk, sizeof(disk), serial, sizeof(serial)) == 1);
    set_u32(disk + 24, 101);
    assert(hwid_read_storage_serial(disk, sizeof(disk), serial, sizeof(serial)) == -1);
    set_u32(disk + 24, 95);
    disk[95] = 'X';
    assert(hwid_read_storage_serial(disk, sizeof(disk), serial, sizeof(serial)) == -1);
    set_u32(disk + 24, 36);
    assert(hwid_read_storage_serial(disk, 20, serial, sizeof(serial)) == -1);
    {
        const char system[] = "SYS-123\0\0";
        const char board[] = "BOARD-456\0\0";
        const char end[] = "\0\0";
        off += record(table + off, 1, 1, system, sizeof(system) - 1);
        off += record(table + off, 2, 1, board, sizeof(board) - 1);
        off += record(table + off, 127, 0, end, sizeof(end) - 1);
    }
    set_u32(table + 4, (uint32_t)(off - 8));
    assert(hwid_read_smbios(table, off, &info) == 0);
    assert(strcmp(info.system_serial, "SYS-123") == 0);
    assert(strcmp(info.board_serial, "BOARD-456") == 0);
    assert(hwid_read_smbios(table, 7, &info) == -1);
    assert(hwid_read_smbios(table, off - 1, &info) == -1);
    table[9] = 255;
    assert(hwid_read_smbios(table, off, &info) == -1);
    puts("PASS: SMBIOS and storage readback parser tests");
    return 0;
}
