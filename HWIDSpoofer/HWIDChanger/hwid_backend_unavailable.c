/*
 * Explicitly unavailable backend for unsafe/unfinished interception modes.
 * This module makes unsupported operations fail closed and never loads a
 * kernel driver or changes another process.
 */
#include "hwid_changer.h"
#include <string.h>

int hwid_apply(const HwidConfig *config, RunMode mode) {
    (void)config;
    (void)mode;
    return -1;
}

int hwid_revert(void) {
    /* No native changes were applied by this backend. */
    return 0;
}

int hwid_load_driver(void) {
    return -1;
}

int hwid_unload_driver(void) {
    return -1;
}

int hwid_driver_set_config(const HwidConfig *config) {
    (void)config;
    return -1;
}

int hwid_enable_global_mode(const HwidConfig *config) {
    (void)config;
    return -1;
}

int hwid_disable_global_mode(void) {
    return -1;
}

int hwid_is_global_mode_active(void) {
    return 0;
}

int hwid_get_real_uuid(char *out, int length) {
    if (!out || length < 1) return -1;
    out[0] = '\0';
    return -1;
}
