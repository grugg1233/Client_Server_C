# Makefile for Client_Server_C (gcc)
# Layout:
#   src/server.c src/client.c src/parser.c src/protocol.c src/util.c
#   src/parser.h src/protocol.h src/util.h

CC      := gcc
CFLAGS  := -O2 -Wall -Wextra -Wpedantic
CPPFLAGS:= -Isrc
LDFLAGS :=
LDLIBS_SERVER := -pthread -lm
LDLIBS_CLIENT :=

SRC_DIR := src
BUILD_DIR := build

SERVER := server
CLIENT := client

SERVER_SRCS := $(SRC_DIR)/server.c $(SRC_DIR)/parser.c $(SRC_DIR)/protocol.c $(SRC_DIR)/util.c
CLIENT_SRCS := $(SRC_DIR)/client.c $(SRC_DIR)/protocol.c $(SRC_DIR)/util.c

SERVER_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SERVER_SRCS))
CLIENT_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(CLIENT_SRCS))

.PHONY: all clean run-server run-client debug asan tsan

all: $(SERVER) $(CLIENT)

# Link targets
$(SERVER): $(SERVER_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS_SERVER)

$(CLIENT): $(CLIENT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS_CLIENT)

# Compile .c -> .o into build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Convenience run targets (edit host/port as needed)
run-server: $(SERVER)
	./$(SERVER) 127.0.0.1 9000

run-client: $(CLIENT)
	./$(CLIENT) 127.0.0.1 9000

# Debug build (no optimizations, symbols)
debug: CFLAGS := -O0 -g3 -Wall -Wextra -Wpedantic
debug: clean all

# AddressSanitizer build
asan: CFLAGS := -O1 -g3 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer
asan: LDFLAGS := -fsanitize=address,undefined
asan: clean all

# ThreadSanitizer build (useful for pthread issues)
tsan: CFLAGS := -O1 -g3 -Wall -Wextra -Wpedantic -fsanitize=thread -fno-omit-frame-pointer
tsan: LDFLAGS := -fsanitize=thread
tsan: clean all

clean:
	rm -rf $(BUILD_DIR) $(SERVER) $(CLIENT)
