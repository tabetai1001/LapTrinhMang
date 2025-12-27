#!/bin/bash
# Script tự động build và chạy dự án trên WSL/Linux

set -e  # Exit on error

echo "====================================="
echo "  AI LÀ TRIỆU PHÚ - BUILD SCRIPT"
echo "====================================="
echo ""

# Kiểm tra các công cụ cần thiết
echo "🔍 Kiểm tra các công cụ cần thiết..."

check_command() {
    if ! command -v $1 &> /dev/null; then
        echo "❌ $1 chưa được cài đặt!"
        echo "   Cài đặt: sudo apt install $2"
        exit 1
    else
        echo "✅ $1: $(command -v $1)"
    fi
}

check_command "gcc" "build-essential"
check_command "make" "make"
check_command "python3" "python3"

echo ""

# Build project
echo "🔨 Build dự án..."
echo ""

make clean
echo ""

make all
echo ""

# Kiểm tra kết quả build
if [ -f "bin/server" ] && [ -f "bin/client_network.so" ]; then
    echo "✅ Build thành công!"
    echo ""
    echo "Các file đã được tạo:"
    ls -lh bin/
    echo ""
    
    # Kiểm tra dependencies
    echo "🔍 Kiểm tra dependencies của server..."
    ldd bin/server | grep -E "pthread|libc"
    echo ""
    
    echo "🔍 Kiểm tra dependencies của client library..."
    ldd bin/client_network.so | grep -E "pthread|libc"
    echo ""
    
    echo "====================================="
    echo "✅ SẴN SÀNG CHẠY!"
    echo "====================================="
    echo ""
    echo "📌 Để chạy server:"
    echo "   ./bin/server"
    echo "   hoặc: make run"
    echo ""
    echo "📌 Để chạy client:"
    echo "   python3 src/client/main.py"
    echo ""
    echo "📌 Server sẽ lắng nghe trên port: 5555"
    echo ""
else
    echo "❌ Build thất bại!"
    echo "Vui lòng kiểm tra lỗi ở trên"
    exit 1
fi
