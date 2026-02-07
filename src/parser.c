// src/parser.c
#include "parser.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *s;      // full string
    const char *p;      // current pointer
    const char *err_at; // where error happened
    char err_msg[128];
} parser_t;

static void skip_ws(parser_t *ps) {
    while (*ps->p && isspace((unsigned char)*ps->p)) ps->p++;
}

static void set_err(parser_t *ps, const char *msg) {
    if (!ps->err_at) ps->err_at = ps->p;
    snprintf(ps->err_msg, sizeof(ps->err_msg), "%s", msg);
}

static int match(parser_t *ps, char c) {
    skip_ws(ps);
    if (*ps->p == c) {
        ps->p++;
        return 1;
    }
    return 0;
}

static int peek(parser_t *ps) {
    skip_ws(ps);
    return (unsigned char)*ps->p;
}

static int parse_number(parser_t *ps, double *out);

// Grammar (with typical right-assoc exponent):
// expr   -> term (('+'|'-') term)*
// term   -> power (('*'|'/') power)*
// power  -> unary ('^' power)?          // right associative
// unary  -> ('+'|'-') unary | primary
// primary-> NUMBER | '(' expr ')'

static int parse_expr(parser_t *ps, double *out);
static int parse_term(parser_t *ps, double *out);
static int parse_power(parser_t *ps, double *out);
static int parse_unary(parser_t *ps, double *out);
static int parse_primary(parser_t *ps, double *out);

static int parse_expr(parser_t *ps, double *out) {
    double v;
    if (!parse_term(ps, &v)) return 0;

    for (;;) {
        if (match(ps, '+')) {
            double rhs;
            if (!parse_term(ps, &rhs)) return 0;
            v += rhs;
        } else if (match(ps, '-')) {
            double rhs;
            if (!parse_term(ps, &rhs)) return 0;
            v -= rhs;
        } else {
            break;
        }
    }

    *out = v;
    return 1;
}

static int parse_term(parser_t *ps, double *out) {
    double v;
    if (!parse_power(ps, &v)) return 0;

    for (;;) {
        if (match(ps, '*')) {
            double rhs;
            if (!parse_power(ps, &rhs)) return 0;
            v *= rhs;
        } else if (match(ps, '/')) {
            double rhs;
            if (!parse_power(ps, &rhs)) return 0;
            if (rhs == 0.0) {
                set_err(ps, "division by zero");
                return 0;
            }
            v /= rhs;
        } else {
            break;
        }
    }

    *out = v;
    return 1;
}

static int parse_power(parser_t *ps, double *out) {
    double base;
    if (!parse_unary(ps, &base)) return 0;

    if (match(ps, '^')) {
        double exp;
        if (!parse_power(ps, &exp)) return 0; // right assoc
        errno = 0;
        double r = pow(base, exp);
        if (errno != 0 || !isfinite(r)) {
            set_err(ps, "invalid exponentiation");
            return 0;
        }
        *out = r;
        return 1;
    }

    *out = base;
    return 1;
}

static int parse_unary(parser_t *ps, double *out) {
    if (match(ps, '+')) {
        return parse_unary(ps, out);
    }
    if (match(ps, '-')) {
        double v;
        if (!parse_unary(ps, &v)) return 0;
        *out = -v;
        return 1;
    }
    return parse_primary(ps, out);
}

static int parse_primary(parser_t *ps, double *out) {
    if (match(ps, '(')) {
        double v;
        if (!parse_expr(ps, &v)) return 0;
        if (!match(ps, ')')) {
            set_err(ps, "expected ')'");
            return 0;
        }
        *out = v;
        return 1;
    }
    return parse_number(ps, out);
}

static int parse_number(parser_t *ps, double *out) {
    skip_ws(ps);

    // Use strtod for floats/ints; ensure something was consumed.
    char *end = NULL;
    errno = 0;
    double v = strtod(ps->p, &end);

    if (end == ps->p) {
        set_err(ps, "expected number or '('");
        return 0;
    }
    if (errno != 0 || !isfinite(v)) {
        set_err(ps, "invalid number");
        return 0;
    }

    ps->p = end;
    *out = v;
    return 1;
}

parse_status_t parse_eval(const char *input, double *out_value,
                          char *errbuf, size_t errbuf_sz) {
    if (!input || !out_value) return PARSE_ERR;

    parser_t ps;
    memset(&ps, 0, sizeof(ps));
    ps.s = input;
    ps.p = input;

    double v = 0.0;
    if (!parse_expr(&ps, &v)) {
        // Build "parse error near 'X'" style message
        const char *at = ps.err_at ? ps.err_at : ps.p;
        char near[32];
        if (!at || *at == '\0') {
            snprintf(near, sizeof(near), "end");
        } else if (isprint((unsigned char)*at)) {
            snprintf(near, sizeof(near), "%c", *at);
        } else {
            snprintf(near, sizeof(near), "byte 0x%02X", (unsigned char)*at);
        }

        if (errbuf && errbuf_sz > 0) {
            snprintf(errbuf, errbuf_sz, "%s near '%s'", ps.err_msg[0] ? ps.err_msg : "parse error", near);
        }
        return PARSE_ERR;
    }

    skip_ws(&ps);
    if (*ps.p != '\0') {
        if (errbuf && errbuf_sz > 0) {
            snprintf(errbuf, errbuf_sz, "unexpected token near '%c'", isprint((unsigned char)*ps.p) ? *ps.p : '?');
        }
        return PARSE_ERR;
    }

    *out_value = v;
    return PARSE_OK;
}
