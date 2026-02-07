// src/protocol.h
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#define PROTO_MAX_FRAME (1024u * 1024u)  // 1 MiB sanity limit

ssize_t read_exact(int fd, void *buf, size_t n);
ssize_t write_exact(int fd, const void *buf, size_t n);

int send_frame(int fd, const void *data, uint32_t len);
int recv_frame(int fd, char **out_buf, uint32_t *out_len);
