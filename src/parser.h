// src/parser.h
#pragma once

#include <stddef.h>

typedef enum {
    PARSE_OK = 0,
    PARSE_ERR = 1
} parse_status_t;

// Evaluates an expression string.
// On success: returns PARSE_OK, *out_value set.
// On error: returns PARSE_ERR and fills errbuf with a helpful message.
parse_status_t parse_eval(const char *input, double *out_value,
                          char *errbuf, size_t errbuf_sz);
