#include "hwid_readback.h"
#include <stdint.h>
#include <string.h>

static uint32_t little32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int copy_string(const unsigned char *p, size_t available,
                       char *out, size_t cap) {
    const unsigned char *zero;
    size_t len;
    if (!p || !out || cap == 0) return -1;
    zero = (const unsigned char *)memchr(p, 0, available);
    if (!zero) return -1;
    len = (size_t)(zero - p);
    if (len >= cap) len = cap - 1;
    memcpy(out, p, len);
    out[len] = 0;
    return 0;
}

int hwid_read_storage_serial(const void *response, size_t bytes,
                             char *out, size_t cap) {
    const unsigned char *p = (const unsigned char *)response;
    uint32_t reported_size, offset;
    if (!out || cap == 0) return -1;
    out[0] = 0;
    if (!p || bytes < 36) return -1;
    reported_size = little32(p + 4);
    if (reported_size < 36 || reported_size > bytes) return -1;
    offset = little32(p + 24);
    if (offset == 0) return 1;
    if (offset < 36 || offset >= reported_size) return -1;
    if (copy_string(p + offset, reported_size - offset, out, cap) != 0) return -1;
    return out[0] == 0 ? 1 : 0;
}

static int dmi_string(const unsigned char *strings, const unsigned char *end,
                      unsigned int index, char *out, size_t cap) {
    const unsigned char *cursor = strings;
    unsigned int i;
    if (!out || cap == 0) return -1;
    out[0] = 0;
    if (index == 0) return 1;
    if (index > 127) return -1;
    for (i = 1; i <= index; ++i) {
        const unsigned char *terminator;
        if (cursor >= end || *cursor == 0) return 1;
        terminator = (const unsigned char *)memchr(cursor, 0,
                                                   (size_t)(end - cursor));
        if (!terminator) return -1;
        if (i == index)
            return copy_string(cursor, (size_t)(end - cursor), out, cap);
        cursor = terminator + 1;
    }
    return 1;
}

int hwid_read_smbios(const void *response, size_t bytes, HwidReadback *out) {
    const unsigned char *b = (const unsigned char *)response;
    const unsigned char *cur, *end;
    uint32_t length;
    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    if (!b || bytes < 8) return -1;
    length = little32(b + 4);
    if (length > bytes - 8) return -1;
    cur = b + 8;
    end = cur + length;
    while (cur < end) {
        unsigned int type, format_len;
        const unsigned char *strings, *next = NULL, *p;
        if ((size_t)(end - cur) < 4) return -1;
        type = cur[0];
        format_len = cur[1];
        if (format_len < 4 || format_len > (size_t)(end - cur)) return -1;
        strings = cur + format_len;
        for (p = strings; p + 1 < end; ++p) {
            if (p[0] == 0 && p[1] == 0) {
                next = p + 2;
                break;
            }
        }
        if (!next) return -1;
        if (type == 0 && format_len >= 9) {
            if (dmi_string(strings, next, cur[4],
                    out->bios_vendor, sizeof(out->bios_vendor)) < 0) return -1;
            if (dmi_string(strings, next, cur[5],
                    out->bios_version, sizeof(out->bios_version)) < 0) return -1;
        }
        if (type == 1 && format_len >= 8) {
            if (dmi_string(strings, next, cur[7],
                    out->system_serial, sizeof(out->system_serial)) < 0) return -1;
        }
        if (type == 2 && format_len >= 8) {
            if (dmi_string(strings, next, cur[7],
                    out->board_serial, sizeof(out->board_serial)) < 0) return -1;
        }
        if (type == 127) break;
        cur = next;
    }
    return 0;
}
