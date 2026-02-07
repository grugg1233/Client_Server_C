# Multi-Threaded Client/Server in C

A lightweight **multi-threaded TCP client/server** written in **C (gcc)**.  
The server accepts multiple client connections concurrently, receives a **byte stream** containing **math expressions**, **top-down parses** (recursive descent) those expressions, evaluates them, and sends the **result back across the socket**.

This project focuses on real systems-programming concerns: raw sockets, explicit message framing, partial reads/writes, concurrency, and deterministic parsing.

---

## Features

- TCP client/server using POSIX sockets
- Multi-threaded server using `pthread`
- Safe byte-stream protocol (length-prefixed messages)
- Top-down (recursive descent) math expression parser
- Request/response model with structured error handling
- Clean separation of networking, protocol, and parsing logic

---

## Project Layout (Recommended)

```
Client_Server_C/
├─ src/
│  ├─ server.c        # Server entry point + threading
│  ├─ client.c        # Client entry point
│  ├─ parser.c        # Recursive descent parser + evaluator
│  ├─ parser.h
│  ├─ protocol.c     # Framing + read/write helpers
│  ├─ protocol.h
│  ├─ util.c          # Shared helpers
│  └─ util.h
├─ Makefile
└─ README.md
```

---

## Requirements

### Operating System
- Linux or macOS  
  (Windows would require Winsock changes)

### Compiler & Tools
- **gcc**
- `make` (recommended)

---

## Libraries / APIs Used

### Networking (POSIX)
- `sys/socket.h`
- `netinet/in.h`
- `arpa/inet.h`
- `unistd.h`

### Concurrency
- `pthread.h`

### Parsing / Utilities
- `stdlib.h`
- `string.h`
- `ctype.h`
- `errno.h`

### Math
- `math.h`  
  (link with `-lm`)

### Optional / Recommended
- `signal.h`
- `stdatomic.h`

---

## Build (gcc)

### Compiler Flags
- `-Wall -Wextra -Wpedantic`
- `-pthread`
- `-lm`

### Example Build Commands

```sh
gcc -O2 -Wall -Wextra -pthread -o server \
    src/server.c src/parser.c src/protocol.c src/util.c -lm

gcc -O2 -Wall -Wextra -o client \
    src/client.c src/protocol.c src/util.c
```

Or simply:

```sh
make
```

---

## Running

### Start the Server
```sh
./server 127.0.0.1 9000
```

### Run the Client
```sh
./client 127.0.0.1 9000
```

### Example Input
```
2+2
(3+4)*5
-10/2
2^8
```

### Example Output
```
OK 4
OK 35
OK -5
OK 256
```

On error:
```
ERR parse error near '*'
```

---

## TCP Protocol (Byte-Stream Safe)

TCP is a **byte stream**, not message-based.  
This project uses **length-prefixed framing** to safely reconstruct messages.

### Message Format

```
[4-byte uint32 length (network byte order)][payload bytes]
```

- Payload is a UTF-8 / ASCII expression string
- Payload is **not null-terminated**

### Response Payloads

- Success: `OK <result>\n`
- Error: `ERR <reason>\n`

---

## Required I/O Helpers

All socket I/O must handle partial reads and writes.

Recommended helpers:
- `read_exact(int fd, void *buf, size_t n)`
- `write_exact(int fd, const void *buf, size_t n)`

---

## Parsing & Evaluation (Top-Down)

The math parser uses **recursive descent**.

### Grammar

```
expr        → term (('+' | '-') term)*
term        → factor (('*' | '/') factor)*
factor      → unary ('^' unary)*
unary       → ('+' | '-') unary | primary
primary     → NUMBER | '(' expr ')'
```

---

## Concurrency Model

### Thread-Per-Connection

- Main thread:
  - `socket()`, `bind()`, `listen()`, `accept()`
- For each client:
  - spawn a `pthread`
  - read → parse → evaluate → respond
- Thread exits on client disconnect

---

## Summary

This project demonstrates:

- Real TCP byte-stream handling
- Explicit protocol framing
- Multi-threaded server design
- Recursive descent parsing
- Deterministic math evaluation
- Clean C systems programming with gcc
