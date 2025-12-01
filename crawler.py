import requests
import json
import html
import random
import os
import math
import time

# --- CẤU HÌNH ---
TOTAL_AMOUNT = 2000    # Tổng số câu hỏi muốn lấy
OUTPUT_DIR = "data"    # Thư mục lưu file
OUTPUT_FILE = "questions.json"
BATCH_SIZE = 50        # Số câu lấy mỗi lần gọi API (Max của OpenTDB là 50)

# Danh sách ID các chủ đề (Science, Geography, History, Animals, Politics...)
TARGET_CATEGORIES = [
    17, # Science & Nature
    18, # Science: Computers
    19, # Science: Mathematics
    22, # Geography
    23, # History
    24, # Politics
    27  # Animals
]

# Định nghĩa thứ tự ưu tiên độ khó để sắp xếp
DIFFICULTY_ORDER = {
    "easy": 1,
    "medium": 2,
    "hard": 3
}

def fetch_and_process():
    print(f"--- BAT DAU CRAWL {TOTAL_AMOUNT} CAU HOI ---")
    
    raw_results = []
    
    # Tính số câu cần lấy cho mỗi chủ đề (chia đều)
    amount_per_cat = math.ceil(TOTAL_AMOUNT / len(TARGET_CATEGORIES))
    print(f"Muc tieu: ~{amount_per_cat} cau hoi / moi chu de")

    # 1. Vòng lặp qua từng chủ đề
    for cat_id in TARGET_CATEGORIES:
        print(f"\n>> Dang xu ly Category ID {cat_id}...")
        
        collected_for_cat = 0
        empty_response_count = 0
        
        # Vòng lặp để lấy đủ số lượng cho chủ đề này (Phân trang)
        while collected_for_cat < amount_per_cat:
            # Tính số lượng cần lấy trong lần này (tối đa 50)
            remaining = amount_per_cat - collected_for_cat
            current_batch_size = min(BATCH_SIZE, remaining)
            
            url = f"https://opentdb.com/api.php?amount={current_batch_size}&category={cat_id}&type=multiple"
            
            try:
                response = requests.get(url)
                
                # Xử lý giới hạn tốc độ (Rate Limit)
                if response.status_code == 429:
                    print("   ! Qua nhanh (Rate Limit). Dang cho 5 giay...")
                    time.sleep(5)
                    continue # Thử lại
                
                if response.status_code != 200:
                    print(f"   ! Loi HTTP {response.status_code}. Bo qua lan nay.")
                    break

                data = response.json()
                response_code = data["response_code"]

                # Code 0: Thành công
                if response_code == 0:
                    batch_results = data["results"]
                    if len(batch_results) > 0:
                        raw_results.extend(batch_results)
                        collected_for_cat += len(batch_results)
                        print(f"   + Da lay {len(batch_results)} cau (Tong chu de nay: {collected_for_cat}/{amount_per_cat})")
                    else:
                        print("   ! API tra ve rong.")
                        break # Dừng chủ đề này
                
                # Code 1: No Results (Hết câu hỏi trong DB của họ cho chủ đề này)
                elif response_code == 1:
                    print(f"   ! Da het cau hoi cho chu de {cat_id}. Chuyen chu de khac.")
                    break 
                
                # Các lỗi khác
                else:
                    print(f"   ! Loi API Code: {response_code}. Thu lai...")
                    time.sleep(1)

            except Exception as e:
                print(f"   ! Loi Exception: {e}")
                break
            
            # Nghỉ 2 giây giữa các lần gọi để tránh bị ban IP
            time.sleep(2)

    # Cắt bớt nếu lấy dư
    if len(raw_results) > TOTAL_AMOUNT:
        raw_results = raw_results[:TOTAL_AMOUNT]

    print(f"\n=== TONG KET: Thu duoc {len(raw_results)} cau hoi tho. Dang xu ly... ===")

    # 2. Sắp xếp theo độ khó: Easy -> Medium -> Hard
    raw_results.sort(key=lambda x: DIFFICULTY_ORDER.get(x["difficulty"], 4))

    # 3. Format lại dữ liệu chuẩn JSON dự án
    formatted_questions = []
    
    for index, item in enumerate(raw_results):
        # Giải mã HTML entities
        question_text = html.unescape(item["question"])
        correct_answer = html.unescape(item["correct_answer"])
        incorrect_answers = [html.unescape(ans) for ans in item["incorrect_answers"]]
        
        # Trộn đáp án
        options = incorrect_answers + [correct_answer]
        random.shuffle(options)
        
        # Tìm vị trí đáp án đúng
        answer_index = options.index(correct_answer)
        
        q_obj = {
            "id": index + 1, # ID tăng dần từ 1
            "question": question_text,
            "options": options,
            "answer_index": answer_index,
            "difficulty": item["difficulty"],
            "category": item["category"]
        }
        
        formatted_questions.append(q_obj)

    # 4. Lưu file
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    file_path = os.path.join(OUTPUT_DIR, OUTPUT_FILE)
    
    with open(file_path, "w", encoding="utf-8") as f:
        json.dump(formatted_questions, f, indent=4, ensure_ascii=False)

    print(f"✅ HOAN TAT! Da luu file vao: {file_path}")
    print(f"   - So luong: {len(formatted_questions)} cau")
    print(f"   - Format: Easy -> Medium -> Hard")

if __name__ == "__main__":
    fetch_and_process()