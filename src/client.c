// src/client.c
// Simple TCP client for sending math expressions and receiving results
// Uses length-prefixed framing over a byte stream

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_EXPR_LEN 4096

/* ---- I/O helpers ---- */

static ssize_t read_exact(int fd, void *buf, size_t n) {
    size_t total = 0;
    char *p = buf;

    while (total < n) {
        ssize_t r = recv(fd, p + total, n - total, 0);
        if (r == 0) return 0;              // peer closed
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total += (size_t)r;
    }
    return (ssize_t)total;
}

static ssize_t write_exact(int fd, const void *buf, size_t n) {
    size_t total = 0;
    const char *p = buf;

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

/* ---- Framing helpers ---- */

static int send_frame(int fd, const char *data, uint32_t len) {
    uint32_t net_len = htonl(len);
    if (write_exact(fd, &net_len, sizeof(net_len)) < 0) return -1;
    if (write_exact(fd, data, len) < 0) return -1;
    return 0;
}

static int recv_frame(int fd, char **out_buf, uint32_t *out_len) {
    uint32_t net_len;
    if (read_exact(fd, &net_len, sizeof(net_len)) <= 0) return -1;

    uint32_t len = ntohl(net_len);
    if (len == 0 || len > 1024 * 1024) return -1; // sanity check

    char *buf = malloc(len + 1);
    if (!buf) return -1;

    if (read_exact(fd, buf, len) <= 0) {
        free(buf);
        return -1;
    }

    buf[len] = '\0';
    *out_buf = buf;
    *out_len = len;
    return 0;
}

/* ---- Main ---- */

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address\n");
        close(sock);
        return EXIT_FAILURE;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return EXIT_FAILURE;
    }

    printf("Connected to %s:%d\n", server_ip, port);
    printf("Enter math expressions (Ctrl+D to quit)\n");

    char line[MAX_EXPR_LEN];

    while (fgets(line, sizeof(line), stdin)) {
        size_t len = strlen(line);

        // trim trailing newline
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }

        if (len == 0) continue;

        if (send_frame(sock, line, (uint32_t)len) < 0) {
            perror("send_frame");
            break;
        }

        char *response = NULL;
        uint32_t resp_len = 0;

        if (recv_frame(sock, &response, &resp_len) < 0) {
            fprintf(stderr, "Server disconnected or protocol error\n");
            break;
        }

        printf("%s", response);
        if (response[resp_len - 1] != '\n')
            printf("\n");

        free(response);
    }

    printf("\nClosing connection\n");
    close(sock);
    return EXIT_SUCCESS;
}
