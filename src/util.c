// src/util.c
#include "util.h"

#include <stdio.h>

void format_ok(char *buf, size_t bufsz, double value) {
    // Use %.15g to avoid goofy trailing zeros while still being precise
    snprintf(buf, bufsz, "OK %.15g\n", value);
}

void format_err(char *buf, size_t bufsz, const char *reason) {
    if (!reason) reason = "error";
    snprintf(buf, bufsz, "ERR %s\n", reason);
}
