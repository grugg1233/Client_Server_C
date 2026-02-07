// src/server.c
// Multi-threaded TCP server: one thread per connection.
// Receives length-prefixed expression strings, parses/evaluates, responds.

#include "protocol.h"
#include "parser.h"
#include "util.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct {
    int fd;
    struct sockaddr_in peer;
} client_ctx_t;

static void *client_thread(void *arg) {
    client_ctx_t *ctx = (client_ctx_t *)arg;
    int fd = ctx->fd;

    char peer_ip[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &ctx->peer.sin_addr, peer_ip, sizeof(peer_ip));
    int peer_port = ntohs(ctx->peer.sin_port);

    fprintf(stderr, "[+] client connected %s:%d\n", peer_ip, peer_port);

    free(ctx);

    for (;;) {
        char *expr = NULL;
        uint32_t expr_len = 0;

        if (recv_frame(fd, &expr, &expr_len) < 0) {
            break; // disconnect / protocol error
        }

        double value = 0.0;
        char err[256] = {0};
        char out[512] = {0};

        if (parse_eval(expr, &value, err, sizeof(err)) == PARSE_OK) {
            format_ok(out, sizeof(out), value);
        } else {
            // Match README-ish style: "ERR <reason>\n"
            format_err(out, sizeof(out), err[0] ? err : "parse error");
        }

        free(expr);

        // send response as framed payload (without relying on null terminator)
        uint32_t out_len = (uint32_t)strlen(out);
        if (send_frame(fd, out, out_len) < 0) {
            break;
        }
    }

    close(fd);
    fprintf(stderr, "[-] client disconnected\n");
    return NULL;
}

static int make_listen_socket(const char *ip, uint16_t port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return -1;

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        close(s);
        errno = EINVAL;
        return -1;
    }

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(s);
        return -1;
    }

    if (listen(s, 128) < 0) {
        close(s);
        return -1;
    }

    return s;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <bind_ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *bind_ip = argv[1];
    int port_i = atoi(argv[2]);
    if (port_i <= 0 || port_i > 65535) {
        fprintf(stderr, "Invalid port\n");
        return EXIT_FAILURE;
    }
    uint16_t port = (uint16_t)port_i;

    // Don't die if we write to a closed socket.
    signal(SIGPIPE, SIG_IGN);

    int listen_fd = make_listen_socket(bind_ip, port);
    if (listen_fd < 0) {
        perror("listen socket");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Server listening on %s:%u\n", bind_ip, (unsigned)port);

    for (;;) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);
        int client_fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            break;
        }

        client_ctx_t *ctx = (client_ctx_t *)malloc(sizeof(*ctx));
        if (!ctx) {
            close(client_fd);
            continue;
        }
        ctx->fd = client_fd;
        ctx->peer = peer;

        pthread_t tid;
        int rc = pthread_create(&tid, NULL, client_thread, ctx);
        if (rc != 0) {
            // pthread_create returns error number (not via errno)
            fprintf(stderr, "pthread_create failed: %s\n", strerror(rc));
            close(client_fd);
            free(ctx);
            continue;
        }

        pthread_detach(tid);
    }

    close(listen_fd);
    return EXIT_SUCCESS;
}
