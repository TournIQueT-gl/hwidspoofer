#include "../hwid_readback.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Deterministic malformed API replies for sanitizer / CI regression tests.
 * This only reads process-owned byte arrays; it does not touch hardware. */
static uint32_t state = 0x8bd4a932u;
static uint32_t rng(void) {
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return state = x;
}
int main(void) {
    unsigned char b[4096];
    HwidReadback r;
    char disk[128];
    unsigned i;
    for (i = 0; i < 200000; i++) {
        size_t size = (size_t)(rng() % sizeof(b));
        size_t j;
        for (j = 0; j < size; j++) b[j] = (unsigned char)rng();
        (void)hwid_read_smbios(b, size, &r);
        (void)hwid_read_storage_serial(b, size, disk, sizeof disk);
        if (size >= 8) {
            uint32_t n = (uint32_t)(rng() % (size - 7));
            b[4] = (unsigned char)n;
            b[5] = (unsigned char)(n >> 8);
            b[6] = (unsigned char)(n >> 16);
            b[7] = (unsigned char)(n >> 24);
            (void)hwid_read_smbios(b, size, &r);
            (void)hwid_read_storage_serial(b, size, disk, sizeof disk);
        }
    }
    puts("PASS: 200000 deterministic randomized parsing trials");
    return 0;
}
