"""
Script test để kiểm tra server có trả về TẤT CẢ người chơi không
Chạy script này sau khi đã có một số user đăng ký và một số đang offline
"""

import json
from ctypes import *

# Load DLL
try:
    lib = CDLL("./client_network.dll")
    lib.connect_to_server.argtypes = [c_char_p, c_int]
    lib.connect_to_server.restype = c_int
    lib.send_request_and_wait.argtypes = [c_char_p]
    lib.send_request_and_wait.restype = c_char_p
except Exception as e:
    print(f"❌ Lỗi load DLL: {e}")
    exit()

def send_json(data):
    json_str = json.dumps(data)
    res_ptr = lib.send_request_and_wait(json_str.encode('utf-8'))
    try:
        return json.loads(res_ptr.decode('utf-8'))
    except:
        return {}

print("=" * 60)
print("🧪 TEST SCRIPT - Kiểm tra danh sách người chơi")
print("=" * 60)

# Kết nối đến server
print("\n📡 Đang kết nối đến server...")
ip = "127.0.0.1"
port = 5555

if lib.connect_to_server(ip.encode('utf-8'), port):
    print("✅ Kết nối thành công!")
    
    # Test 1: Yêu cầu KHÔNG include offline
    print("\n" + "=" * 60)
    print("TEST 1: GET_LOBBY_LIST (không có include_offline)")
    print("=" * 60)
    res1 = send_json({"type": "GET_LOBBY_LIST"})
    players1 = res1.get("players", [])
    print(f"📊 Số người chơi nhận được: {len(players1)}")
    
    if len(players1) > 0:
        print("\n📋 Danh sách:")
        for i, p in enumerate(players1, 1):
            if isinstance(p, dict):
                print(f"  {i}. {p.get('name', p.get('user', 'unknown'))} - Status: {p.get('status', 'N/A')}")
            else:
                print(f"  {i}. {p} (chỉ có tên, không có status)")
    else:
        print("⚠️ Không có người chơi nào!")
    
    # Test 2: Yêu cầu CÓ include offline
    print("\n" + "=" * 60)
    print("TEST 2: GET_LOBBY_LIST (include_offline=true) ⭐")
    print("=" * 60)
    res2 = send_json({"type": "GET_LOBBY_LIST", "include_offline": True})
    players2 = res2.get("players", [])
    print(f"📊 Số người chơi nhận được: {len(players2)}")
    
    if len(players2) > 0:
        print("\n📋 Danh sách:")
        free_count = 0
        in_game_count = 0
        offline_count = 0
        
        for i, p in enumerate(players2, 1):
            if isinstance(p, dict):
                name = p.get('name', p.get('user', 'unknown'))
                status = p.get('status', 'N/A')
                
                # Đếm theo status
                if status == "FREE":
                    icon = "🟢"
                    free_count += 1
                elif status == "IN_GAME":
                    icon = "🎮"
                    in_game_count += 1
                elif status == "OFFLINE":
                    icon = "⚫"
                    offline_count += 1
                else:
                    icon = "🟡"
                
                print(f"  {icon} {i}. {name} - {status}")
            else:
                print(f"  🔵 {i}. {p} (format cũ, không có status)")
        
        print("\n📊 Thống kê:")
        print(f"  • Tổng: {len(players2)}")
        print(f"  • 🟢 Rảnh: {free_count}")
        print(f"  • 🎮 Đang chơi: {in_game_count}")
        print(f"  • ⚫ Offline: {offline_count}")
        
        # Phân tích kết quả
        print("\n🔍 Phân tích:")
        if offline_count > 0:
            print("  ✅ Server ĐÃ trả về người chơi offline - HOÀN HẢO!")
        else:
            if len(players2) == free_count + in_game_count:
                print("  ⚠️ Server CHƯA trả về người chơi offline!")
                print("  ⚠️ Cần cập nhật server để hỗ trợ 'include_offline' parameter")
                print("  ⚠️ Xem file SERVER_UPDATE_REQUIREMENTS.md để biết cách sửa")
            else:
                print("  ℹ️ Có thể chưa có ai offline (tất cả đang online)")
        
        # So sánh 2 test
        print("\n📊 So sánh TEST 1 vs TEST 2:")
        print(f"  • Test 1 (không include_offline): {len(players1)} người")
        print(f"  • Test 2 (include_offline=true): {len(players2)} người")
        
        if len(players2) > len(players1):
            print(f"  ✅ Test 2 có nhiều hơn {len(players2) - len(players1)} người - Đúng rồi!")
        elif len(players2) == len(players1):
            print("  ⚠️ Cả 2 test trả về số người giống nhau")
            print("  ⚠️ Server có thể CHƯA xử lý parameter 'include_offline'")
        
    else:
        print("⚠️ Không có người chơi nào!")
        print("⚠️ Hãy đăng ký một vài tài khoản trước khi test")
    
    print("\n" + "=" * 60)
    print("🎯 KẾT LUẬN:")
    print("=" * 60)
    
    if len(players2) > 0 and offline_count > 0:
        print("✅ Server HOẠT ĐỘNG TỐT!")
        print("✅ Có thể hiển thị tất cả người chơi kể cả offline")
    elif len(players2) > 0 and offline_count == 0:
        print("⚠️ Server CẦN CẬP NHẬT!")
        print("⚠️ Hiện tại chỉ trả về người chơi ONLINE")
        print("⚠️ Cần xử lý parameter 'include_offline' trong GET_LOBBY_LIST")
        print("\n📖 Hướng dẫn chi tiết:")
        print("   1. Mở file SERVER_UPDATE_REQUIREMENTS.md")
        print("   2. Xem phần 'Hiển thị tất cả người chơi'")
        print("   3. Cập nhật server để trả về cả người offline")
    else:
        print("⚠️ Chưa có dữ liệu đủ để test")
        print("💡 Gợi ý:")
        print("   1. Đăng ký 3-4 tài khoản")
        print("   2. Đăng nhập 1-2 người")
        print("   3. Để 1-2 người offline")
        print("   4. Chạy lại script này")
    
    print("=" * 60)
    
else:
    print("❌ Không thể kết nối đến server!")
    print(f"   IP: {ip}")
    print(f"   Port: {port}")
    print("\n💡 Hãy chắc chắn:")
    print("   1. Server đang chạy")
    print("   2. IP và Port đúng")
    print("   3. File client_network.dll tồn tại")

print("\n✨ Test hoàn tất!")
