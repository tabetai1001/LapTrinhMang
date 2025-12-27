#!/bin/bash
# Script kiểm tra tính tương thích Linux/WSL của dự án

echo "====================================="
echo "  KIỂM TRA DỰ ÁN AI LÀ TRIỆU PHÚ"
echo "====================================="
echo ""

# Kiểm tra các dependency Windows còn sót lại
echo "🔍 Kiểm tra các dependency Windows còn sót lại..."
echo ""

windows_found=0

# Kiểm tra trong các file C/H
echo "Kiểm tra file .c và .h..."
if grep -rn "winsock\|WSADATA\|SOCKET\|CreateThread\|CRITICAL_SECTION\|windows\.h" src/ --include="*.c" --include="*.h" 2>/dev/null | grep -v "README\|\.md"; then
    echo "❌ Tìm thấy Windows-specific code!"
    windows_found=1
else
    echo "✅ Không tìm thấy Windows-specific code trong .c/.h"
fi

echo ""

# Kiểm tra pthread
echo "🔍 Kiểm tra pthread implementation..."
if grep -rn "pthread_mutex_lock\|pthread_create" src/ --include="*.c" --include="*.h" 2>/dev/null | wc -l | grep -q "[1-9]"; then
    echo "✅ Đã sử dụng pthread"
else
    echo "❌ Không tìm thấy pthread implementation"
    windows_found=1
fi

echo ""

# Kiểm tra POSIX sockets
echo "🔍 Kiểm tra POSIX sockets..."
if grep -rn "sys/socket.h\|netinet/in.h\|arpa/inet.h" src/ --include="*.c" --include="*.h" 2>/dev/null | wc -l | grep -q "[1-9]"; then
    echo "✅ Đã sử dụng POSIX sockets"
else
    echo "❌ Không tìm thấy POSIX socket headers"
    windows_found=1
fi

echo ""

# Kiểm tra Makefile
echo "🔍 Kiểm tra Makefile..."
if grep -q "pthread\|\.so" Makefile; then
    echo "✅ Makefile đã hỗ trợ Linux"
else
    echo "❌ Makefile chưa hỗ trợ đầy đủ Linux"
    windows_found=1
fi

echo ""

# Kiểm tra client network
echo "🔍 Kiểm tra client network code..."
if grep -q "client_network.so" src/client/core/network.py; then
    echo "✅ Client đã hỗ trợ .so library"
else
    echo "❌ Client chưa hỗ trợ .so library"
    windows_found=1
fi

echo ""
echo "====================================="
if [ $windows_found -eq 0 ]; then
    echo "✅ KẾT QUẢ: Dự án đã sẵn sàng cho Linux/WSL!"
    echo "====================================="
    echo ""
    echo "📌 Để build và chạy:"
    echo "   make clean"
    echo "   make all"
    echo "   make run"
    echo ""
    exit 0
else
    echo "⚠️  KẾT QUẢ: Vẫn còn một số vấn đề"
    echo "====================================="
    echo ""
    echo "Vui lòng kiểm tra lại các file báo lỗi ở trên"
    echo ""
    exit 1
fi
