import requests
import json
import html
import random
import os
import math

# --- CẤU HÌNH ---
TOTAL_AMOUNT = 200      # Tổng số câu hỏi muốn lấy
OUTPUT_DIR = "data"    # Thư mục lưu file
OUTPUT_FILE = "questions.json"

# Danh sách ID các chủ đề bạn muốn lấy (theo OpenTDB)
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
    
    # Tính số câu cần lấy cho mỗi chủ đề (làm tròn lên)
    amount_per_cat = math.ceil(TOTAL_AMOUNT / len(TARGET_CATEGORIES))

    # 1. Lấy dữ liệu từ từng chủ đề
    for cat_id in TARGET_CATEGORIES:
        url = f"https://opentdb.com/api.php?amount={amount_per_cat}&category={cat_id}&type=multiple"
        print(f"Dang tai category ID {cat_id}...", end=" ")
        
        try:
            response = requests.get(url)
            if response.status_code == 200:
                data = response.json()
                if data["response_code"] == 0:
                    raw_results.extend(data["results"])
                    print(f"OK ({len(data['results'])} cau)")
                else:
                    print("Het cau hoi/Loi API")
            else:
                print("Loi mang")
        except Exception as e:
            print(f"Loi: {e}")

    # Cắt bớt nếu lấy dư (do làm tròn lên)
    if len(raw_results) > TOTAL_AMOUNT:
        raw_results = raw_results[:TOTAL_AMOUNT]

    print(f"\nTong thu duoc: {len(raw_results)} cau hoi tho. Dang xu ly va sap xep...")

    # 2. Sắp xếp theo độ khó: Easy -> Medium -> Hard
    # Sử dụng lambda để map từ string sang số (1, 2, 3) để sort
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
            "id": index + 1, # ID tăng dần từ 1 sau khi đã sắp xếp
            "question": question_text,
            "options": options,
            "answer_index": answer_index,
            "difficulty": item["difficulty"], # Giữ lại để debug nếu cần
            "category": item["category"]      # (Tùy chọn) Lưu thêm tên chủ đề
        }
        
        formatted_questions.append(q_obj)

    # 4. Lưu file
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    file_path = os.path.join(OUTPUT_DIR, OUTPUT_FILE)
    
    with open(file_path, "w", encoding="utf-8") as f:
        json.dump(formatted_questions, f, indent=4, ensure_ascii=False)

    print(f"\n✅ HOAN TAT! Da luu file vao: {file_path}")
    print(f"   - So luong: {len(formatted_questions)} cau")
    print(f"   - Format: Easy -> Medium -> Hard")

if __name__ == "__main__":
    fetch_and_process()