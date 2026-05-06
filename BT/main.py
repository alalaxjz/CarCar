import time
import sys
import threading
import re
import logging
from hm10_esp32 import HM10ESP32Bridge
from score import ScoreboardServer, ScoreboardFake

# --- 1. 比賽參數設定 ---
SERVER_IP = "http://carcar.ntuee.org/scoreboard" 
TEAM_NAME = "CarCarTeam_08"          
PORT = '/dev/cu.usbserial-10'
EXPECTED_NAME = 'HM10_Blue'

RAW_ACTIONS = "fllbrffrrblrrbfbrrffflbrllbrflrrfbfrrlrlbfbllrrffrfbflrrlfbfrrlrr" 
ACTIONS = list(RAW_ACTIONS) 

# --- 2. 模式切換 ---
USE_FAKE = False    # True: 讀 CSV, False: 連伺服器
MANUAL_MODE = False  # True: 手動輸入 UID, False: 藍牙自動監聽

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    datefmt='%H:%M:%S'
)

def background_listener(bridge, sb, action_list):
    if bridge is None:
        return
        
    logging.info("📡 藍牙實體監聽已啟動...")
    while True:
        try:
            msg = bridge.listen()
            if msg:
                clean_msg = msg.strip().upper()
                
                if re.match(r"^[0-9A-F]+$", clean_msg) and len(clean_msg) <= 8:
                    clean_msg = clean_msg.zfill(8)
                
                print(f"收到訊號: {clean_msg}")
                if clean_msg == "K":
                    bridge.send('ssssss')
                    
                # 2. 處理 UID 讀取
                if re.match(r"^[0-9A-F]{8}$", clean_msg):
                    process_uid(clean_msg, sb)
                    
        except Exception as e:
            logging.error(f"監聽異常: {e}")
        time.sleep(0.01)
        
def process_uid(uid, sb):
    """處理 UID 並顯示結果"""
    logging.info(f"📤 正在送出 UID: {uid} ...")
    try:
        score, time_left = sb.add_UID(uid)
        if score > 0:
            logging.info(f"✅ 成功！獲得 {score} 分，剩餘時間: {time_left}s")
        else:
            logging.info("❌ 無法得分（UID 錯誤或已重複讀取）")
        
        total = sb.get_current_score()
        logging.info(f"🏆 目前總計分：{total}\n")
    except Exception as e:
        logging.error(f"連線處理失敗: {e}")

def main():
    bridge = None
    sb = None

    # ------------------------------------------------------
    # 第一步：連線藍牙 (HM-10 / ESP32)
    # ------------------------------------------------------
    if not MANUAL_MODE:
        logging.info("正在連線藍牙...")
        try:
            bridge = HM10ESP32Bridge(port=PORT) 
        except Exception as e:
            logging.error(f"無法開啟串口 {PORT}: {e}")
            sys.exit(1)
        
        # 名稱校對與連線檢查
        current_name = bridge.get_hm10_name()
        if current_name != EXPECTED_NAME:
            logging.warning(f"名稱不符 (目前: {current_name}, 預期: {EXPECTED_NAME})，嘗試更新...")
            if bridge.set_hm10_name(EXPECTED_NAME):
                bridge.reset()
                time.sleep(2)  # 等待重啟
                bridge = HM10ESP32Bridge(port=PORT)
            else:
                logging.error("❌ 更新名稱失敗，結束程式。")
                sys.exit(1)

        status = bridge.get_status()
        if status != "CONNECTED":
            logging.error(f"⚠️ ESP32 狀態為 {status}。請確認 HM-10 已開啟。")
            sys.exit(0)
            
        logging.info(f"✨ 藍牙連線成功：{EXPECTED_NAME}")
    else:
        logging.info("💡 已開啟手動模式，跳過藍牙連線。")

    # ------------------------------------------------------
    # 第二步：連線計分伺服器 (Server)
    # ------------------------------------------------------
    try:
        if USE_FAKE:
            logging.info("⚠️ 模式：測試模式 (讀取本地 CSV)")
            sb = ScoreboardFake(TEAM_NAME, "data/fakeUID.csv")
        else:
            logging.info(f"🚀 模式：伺服器模式 (連接至: {SERVER_IP})")
            sb = ScoreboardServer(TEAM_NAME, host=SERVER_IP, debug=True)
        logging.info("✅ 伺服器連線成功！")
    except Exception as e:
        logging.critical(f"❌ 計分板連線失敗: {e}")
        return

    # ------------------------------------------------------
    # 第三步：啟動監聽與發送啟動訊號
    # ------------------------------------------------------
    if not MANUAL_MODE:
        # 非手動模式才需要藍牙
        logging.info("正在連線藍牙...")
        # ... (這裡保留原本的藍牙初始化代碼) ...
        try:
            bridge = HM10ESP32Bridge(port=PORT) 
        except Exception as e:
            logging.error(f"無法開啟串口 {PORT}: {e}")
            sys.exit(1)
        
        current_name = bridge.get_hm10_name()
        if current_name != EXPECTED_NAME:
            print(f"Target mismatch. Current: {current_name}, Expected: {EXPECTED_NAME}")
            print(f"Updating target name to {EXPECTED_NAME}...")
            
            if bridge.set_hm10_name(EXPECTED_NAME):
                print("✅ Name updated successfully. Resetting ESP32...")
                bridge.reset()
                # Re-init after reset
                bridge = HM10ESP32Bridge(port=PORT)
            else:
                print("❌ Failed to set name. Exiting.")
                sys.exit(1)

        # 2. Connection Check
        status = bridge.get_status()
        if status != "CONNECTED":
            print(f"⚠️ ESP32 is {status}. Please ensure HM-10 is advertising. Exiting.")
            sys.exit(0)

        print(f"✨ Ready! Connected to {EXPECTED_NAME}")
        threading.Thread(target=background_listener, args=(bridge,sb,ACTIONS), daemon=True).start()
        threading.Thread(target=background_listener, args=(bridge,sb,ACTIONS), daemon=True).start()


    else:
        logging.info("💡 已開啟手動輸入模式，將跳過藍牙連線。")

    # ------------------------------------------------------
    # 第四步：執行主邏輯
    # ------------------------------------------------------
    try:
        if MANUAL_MODE:
            print("\n" + "="*30)
            print("  手動 UID 測試工具 (Server 模式已開啟)")
            print("="*30 + "\n")
            while True:
                user_input = input("請輸入 UID: ").strip().upper()
                if user_input.lower() in ['exit', 'quit']: break
                if re.match(r"^[0-9A-F]{8}$", user_input):
                    process_uid(user_input, sb)
                else:
                    print("格式錯誤！")
        else:
            logging.info("🏎️ 比賽進行中，等待 K 要求或 UID 回傳...")
            while True:
                time.sleep(1)

    except KeyboardInterrupt:
        logging.info("使用者中斷程式。")
    finally:
        logging.info("程式結束。")


if __name__ == "__main__":
    main()

