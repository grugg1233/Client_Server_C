// src/util.h
#pragma once

#include <stddef.h>

// Format a result line into buf: "OK <value>\n" or "ERR <reason>\n"
void format_ok(char *buf, size_t bufsz, double value);
void format_err(char *buf, size_t bufsz, const char *reason);
