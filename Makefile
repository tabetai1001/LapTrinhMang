# Makefile - Project Ai La Trieu Phu
# Tuong thich: Windows CMD, PowerShell & MinGW64/Git Bash
# Compiler: GCC

CC = gcc
CFLAGS = -Wall -I./src/common -I./src/server/include 
LDFLAGS = -lws2_32

# --- DUONG DAN THU MUC ---
BIN_DIR = bin
SRC_COMMON = src/common
SRC_SERVER_DIR = src/server
SRC_SERVER_MODULES = src/server/modules
SRC_CLIENT_NATIVE = src/client/native

# --- TU DONG PHAT HIEN MOI TRUONG ---
ifeq ($(OS),Windows_NT)
	# Day la Windows. Tiep tuc kiem tra xem co phai MinGW/Git Bash khong
	ifdef MSYSTEM
		# Moi truong MinGW / Git Bash (Co ho tro lenh Linux)
		MKDIR_CMD = mkdir -p $(BIN_DIR)
		CLEAN_CMD = rm -rf $(BIN_DIR)
		RUN_CMD = ./$(BIN_DIR)/server.exe
		FixPath = $1
	else
		# Moi truong Windows CMD / PowerShell thuan tuy
		# Bat buoc make su dung cmd.exe de tranh loi
		SHELL = cmd.exe
		MKDIR_CMD = if not exist $(BIN_DIR) mkdir $(BIN_DIR)
		CLEAN_CMD = if exist $(BIN_DIR) rmdir /s /q $(BIN_DIR)
		RUN_CMD = .\\$(BIN_DIR)\\server.exe
		FixPath = $(subst /,\,$1)
	endif
else
	# Moi truong Linux / macOS
	MKDIR_CMD = mkdir -p $(BIN_DIR)
	CLEAN_CMD = rm -rf $(BIN_DIR)
	RUN_CMD = ./$(BIN_DIR)/server.exe
	FixPath = $1
endif

# --- DANH SACH SOURCE FILES ---
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

all: directories server client_dll

# 1. Tao thu muc bin
directories:
	@$(MKDIR_CMD)

# 2. Build Server
server: $(SERVER_SOURCES)
	@echo [BUILD] Building Server...
	$(CC) $(CFLAGS) -o $(call FixPath,$(BIN_DIR)/server.exe) $(SERVER_SOURCES) $(LDFLAGS)
	@echo [SUCCESS] Server built at $(BIN_DIR)/server.exe

# 3. Build Client DLL
client_dll: $(CLIENT_SOURCES)
	@echo [BUILD] Building Client DLL...
	$(CC) $(CFLAGS) -shared -o $(call FixPath,$(BIN_DIR)/client_network.dll) $(CLIENT_SOURCES) $(LDFLAGS)
	@echo [SUCCESS] Client DLL built at $(BIN_DIR)/client_network.dll

# 4. Don dep
clean:
	@$(CLEAN_CMD)
	@echo [CLEAN] Deleted binary files.

# 5. Chay Server
run: server
	@echo [RUN] Starting Server...
	$(RUN_CMD)