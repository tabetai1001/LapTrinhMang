# Makefile - Project Ai La Trieu Phu
# Compatible: Linux, WSL, Windows (MinGW/Git Bash), macOS
# Compiler: GCC

CC = gcc
CFLAGS = -Wall -I./src/common -I./src/server/include 

# --- Tự động phát hiện môi trường ---
ifeq ($(OS),Windows_NT)
	# Windows environment
	LDFLAGS = -lws2_32
	SHELL = cmd.exe
	MKDIR_CMD = if not exist $(BIN_DIR) mkdir $(BIN_DIR)
	CLEAN_CMD = if exist $(BIN_DIR) rmdir /s /q $(BIN_DIR)
	SERVER_EXEC = server.exe
	CLIENT_LIB = client_network.dll
	RUN_CMD = .\\$(BIN_DIR)\\$(SERVER_EXEC)
	FixPath = $(subst /,\,$1)
else
	# Linux/WSL/macOS
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		LDFLAGS = -lpthread
	endif
	ifeq ($(UNAME_S),Darwin)
		LDFLAGS = -lpthread
	endif
	MKDIR_CMD = mkdir -p $(BIN_DIR)
	CLEAN_CMD = rm -rf $(BIN_DIR)
	SERVER_EXEC = server
	CLIENT_LIB = client_network.so
	RUN_CMD = ./$(BIN_DIR)/$(SERVER_EXEC)
	FixPath = $1
endif

# --- Đường dẫn thư mục ---
BIN_DIR = bin
SRC_COMMON = src/common
SRC_SERVER_DIR = src/server
SRC_SERVER_MODULES = src/server/modules
SRC_CLIENT_NATIVE = src/client/native

# --- Danh sách source files ---
SERVER_SOURCES = $(SRC_SERVER_DIR)/main.c \
                 $(SRC_SERVER_MODULES)/server_state.c \
                 $(SRC_SERVER_MODULES)/data_manager.c \
                 $(SRC_SERVER_MODULES)/auth_service.c \
                 $(SRC_SERVER_MODULES)/game_service.c \
                 $(SRC_SERVER_MODULES)/connection_handler.c \
                 $(SRC_COMMON)/cJSON.c

CLIENT_SOURCES = $(SRC_CLIENT_NATIVE)/client_network.c \
                 $(SRC_COMMON)/cJSON.c

# --- TARGETS ---

all: directories server client_lib

# 1. Tạo thư mục bin
directories:
	@$(MKDIR_CMD)

# 2. Build Server
server: $(SERVER_SOURCES)
	@echo [BUILD] Building Server...
	$(CC) $(CFLAGS) -o $(call FixPath,$(BIN_DIR)/$(SERVER_EXEC)) $(SERVER_SOURCES) $(LDFLAGS)
	@echo [SUCCESS] Server built at $(BIN_DIR)/$(SERVER_EXEC)

# 3. Build Client Native Library
client_lib: $(CLIENT_SOURCES)
	@echo [BUILD] Building Client Native Library...
ifeq ($(OS),Windows_NT)
	$(CC) $(CFLAGS) -shared -o $(call FixPath,$(BIN_DIR)/$(CLIENT_LIB)) $(CLIENT_SOURCES) $(LDFLAGS)
else
	$(CC) $(CFLAGS) -shared -fPIC -o $(BIN_DIR)/$(CLIENT_LIB) $(CLIENT_SOURCES) $(LDFLAGS)
endif
	@echo [SUCCESS] Client library built at $(BIN_DIR)/$(CLIENT_LIB)

# 4. Dọn dẹp
clean:
	@$(CLEAN_CMD)
	@echo [CLEAN] Deleted binary files.

# 5. Chạy Server
run: server
	@echo [RUN] Starting Server...
	$(RUN_CMD)

.PHONY: all directories server client_lib clean run