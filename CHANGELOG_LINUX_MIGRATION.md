# CHANGELOG - LINUX/WSL MIGRATION

## Phiên bản 2.0 - Cross-Platform Support (December 27, 2025)

### 🎉 Thay đổi lớn: Chuyển đổi từ Windows sang Linux/WSL

#### Server Side (C)

**1. Socket Programming**
- ❌ Loại bỏ: `winsock2.h`, `windows.h`, `WSAStartup()`, `WSACleanup()`
- ✅ Thêm: `sys/socket.h`, `netinet/in.h`, `arpa/inet.h`, `unistd.h`
- ✅ Thay đổi:
  - `SOCKET` → `int`
  - `INVALID_SOCKET` → `-1`
  - `SOCKET_ERROR` → `-1`
  - `closesocket()` → `close()`
  - `WSAGetLastError()` → `errno` + `perror()`

**2. Multi-threading**
- ❌ Loại bỏ: `CreateThread()`, `DWORD WINAPI`, `LPVOID`, `HANDLE`
- ✅ Thêm: `pthread.h`, `pthread_create()`, `pthread_detach()`
- ✅ Thay đổi:
  - Function signature: `DWORD WINAPI func(LPVOID)` → `void* func(void*)`
  - Return type: `return 0` → `return NULL`
  - Thread ID: `HANDLE` → `pthread_t`

**3. Synchronization**
- ❌ Loại bỏ: `CRITICAL_SECTION`, `InitializeCriticalSection()`, `EnterCriticalSection()`, `LeaveCriticalSection()`
- ✅ Thêm: `pthread_mutex_t`, `pthread_mutex_init()`, `pthread_mutex_lock()`, `pthread_mutex_unlock()`
- ✅ Khởi tạo static: `PTHREAD_MUTEX_INITIALIZER`

**4. Files thay đổi:**
- [x] `src/server/main.c` - Socket initialization & thread creation
- [x] `src/server/include/models.h` - Data types & headers
- [x] `src/server/include/server_state.h` - Mutex declarations
- [x] `src/server/include/connection_handler.h` - Thread function signature
- [x] `src/server/modules/server_state.c` - Mutex initialization
- [x] `src/server/modules/connection_handler.c` - Thread handler & mutexes
- [x] `src/server/modules/game_service.c` - Mutex locks
- [x] `src/server/modules/data_manager.c` - Mutex locks
- [x] `src/server/modules/auth_service.c` - Mutex locks

#### Client Side

**1. Native Library (C)**
- ❌ Loại bỏ: `winsock2.h`, `ws2tcpip.h`, `__declspec(dllexport)`, `WSAStartup()`, `WSACleanup()`
- ✅ Thêm: `sys/socket.h`, `netinet/in.h`, `arpa/inet.h`, `unistd.h`
- ✅ Thay đổi:
  - Export macro: `__declspec(dllexport)` → `__attribute__((visibility("default")))`
  - `SOCKET` → `int`
  - `INVALID_SOCKET` → `-1`
  - `closesocket()` → `close()`
  - Output: `.dll` → `.so`

**2. Python Client**
- ✅ Thêm logic phát hiện hệ điều hành
- ✅ Tự động load `.dll` (Windows) hoặc `.so` (Linux/macOS)
- [x] `src/client/core/network.py` - Cross-platform library loading

#### Build System

**Makefile Updates:**
- ✅ Tự động phát hiện hệ điều hành (`OS`, `uname -s`)
- ✅ Flags riêng cho từng platform:
  - Windows: `-lws2_32`
  - Linux/macOS: `-lpthread`
- ✅ Binary names:
  - Windows: `server.exe`, `client_network.dll`
  - Linux: `server`, `client_network.so`
- ✅ Shared library compilation flags:
  - Windows: `-shared`
  - Linux: `-shared -fPIC`

#### Documentation

**Files mới:**
- ✅ `README_WSL.md` - Hướng dẫn chi tiết cài đặt và chạy trên WSL
- ✅ `check_linux_compatibility.sh` - Script kiểm tra tính tương thích
- ✅ `build.sh` - Script tự động build cho Linux/WSL
- ✅ `CHANGELOG.md` - File này

**Files cập nhật:**
- ✅ `README.md` - Thêm thông tin cross-platform support

### 🔧 Technical Details

#### Thread Management
```c
// Old (Windows)
CreateThread(NULL, 0, handle_client, (LPVOID)new_socket, 0, NULL);

// New (POSIX)
pthread_t thread_id;
pthread_create(&thread_id, NULL, handle_client, (void*)new_socket);
pthread_detach(thread_id);
```

#### Mutex Operations
```c
// Old (Windows)
CRITICAL_SECTION cs;
InitializeCriticalSection(&cs);
EnterCriticalSection(&cs);
LeaveCriticalSection(&cs);

// New (POSIX)
pthread_mutex_t cs = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_init(&cs, NULL);
pthread_mutex_lock(&cs);
pthread_mutex_unlock(&cs);
```

#### Socket Operations
```c
// Old (Windows)
SOCKET sock = socket(...);
if (sock == INVALID_SOCKET) { ... }
closesocket(sock);

// New (POSIX)
int sock = socket(...);
if (sock < 0) { ... }
close(sock);
```

### 🎯 Tested Platforms

- ✅ Ubuntu 20.04/22.04 LTS
- ✅ WSL2 (Ubuntu)
- ✅ Debian 11/12
- 🔄 macOS (should work, not tested)
- ⚠️  Windows native (requires re-enabling Winsock code)

### 🐛 Known Issues

1. **GUI on WSL**: Cần cấu hình X Server (VcXsrv/X410) để hiển thị Tkinter GUI
2. **File permissions**: Có thể cần `chmod +x` cho các file executable
3. **Line endings**: Files từ Windows có thể cần convert với `dos2unix`

### 📊 Migration Statistics

- **Files modified**: 11 C/H files, 1 Python file, 1 Makefile
- **Lines changed**: ~200+ lines
- **Windows API calls removed**: ~30+
- **POSIX API calls added**: ~30+
- **Backward compatibility**: Có thể compile lại cho Windows nếu cần

### 🚀 Next Steps

1. Test trên nhiều distributions khác nhau
2. Tối ưu performance trên Linux
3. Thêm Docker support
4. CI/CD pipeline cho multi-platform builds

### 👥 Contributors

- Migration to Linux/WSL: [Your Name]
- Original Windows version: [Original Author]

---

**Migration completed on**: December 27, 2025
**Status**: ✅ Fully functional on Linux/WSL
