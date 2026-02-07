// src/protocol.c
#include "protocol.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

ssize_t read_exact(int fd, void *buf, size_t n) {
    size_t total = 0;
    char *p = (char *)buf;

    while (total < n) {
        ssize_t r = recv(fd, p + total, n - total, 0);
        if (r == 0) return 0; // peer closed
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total += (size_t)r;
    }
    return (ssize_t)total;
}

ssize_t write_exact(int fd, const void *buf, size_t n) {
    size_t total = 0;
    const char *p = (const char *)buf;

    while (total < n) {
        ssize_t w = send(fd, p + total, n - total, 0);
        if (w <= 0) {
            if (w < 0 && errno == EINTR) continue;
            return -1;
        }
        total += (size_t)w;
    }
    return (ssize_t)total;
}

int send_frame(int fd, const void *data, uint32_t len) {
    uint32_t net_len = htonl(len);
    if (write_exact(fd, &net_len, sizeof(net_len)) < 0) return -1;
    if (len > 0 && write_exact(fd, data, len) < 0) return -1;
    return 0;
}

int recv_frame(int fd, char **out_buf, uint32_t *out_len) {
    if (!out_buf || !out_len) return -1;

    uint32_t net_len = 0;
    ssize_t r = read_exact(fd, &net_len, sizeof(net_len));
    if (r <= 0) return -1;

    uint32_t len = ntohl(net_len);
    if (len == 0 || len > PROTO_MAX_FRAME) return -1;

    char *buf = (char *)malloc((size_t)len + 1);
    if (!buf) return -1;

    r = read_exact(fd, buf, len);
    if (r <= 0) {
        free(buf);
        return -1;
    }

    buf[len] = '\0';
    *out_buf = buf;
    *out_len = len;
    return 0;
}
