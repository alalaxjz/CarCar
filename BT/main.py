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
                # 1. 處理 Arduino 請求
                if clean_msg == "ASK":
                    if action_list:
                        next_move = action_list.pop(0)
                        bridge.send(next_move)
                        logging.info(f"🤖 Arduino 請求 -> 送出指令: {next_move} (剩餘 {len(action_list)} 步)")
                    # else:
                    #     bridge.send('S')
                    #     logging.warning("⚠️ 指令清單已空，送出 'S' 停止車子")
                    
                # 2. 處理 UID 讀取
                elif re.match(r"^[0-9A-F]{8}$", clean_msg):
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
    # ------------------------------------------------------
    # 第一步：初始化計分系統
    # ------------------------------------------------------
    try:
        if USE_FAKE:
            logging.info("⚠️ 模式：測試模式 (讀取本地 CSV)")
            sb = ScoreboardFake(TEAM_NAME, "data/fakeUID.csv")
        else:
            logging.info(f"🚀 模式：伺服器模式 (連接至: {SERVER_IP})")
            sb = ScoreboardServer(TEAM_NAME, host=SERVER_IP, debug=True)
    except Exception as e:
        logging.critical(f"計分板連線失敗: {e}")
        return

    # ------------------------------------------------------
    # 第二步：根據模式決定是否啟動藍牙
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

    else:
        logging.info("💡 已開啟手動輸入模式，將跳過藍牙連線。")

    # ------------------------------------------------------
    # 第三步：運行邏輯
    # ------------------------------------------------------
    try:
        if MANUAL_MODE:
            print("\n" + "="*30)
            print("  手動 UID 測試工具已啟動")
            print("  請輸入 8 位 UID (如: 10BA617E)")
            print("  輸入 'exit' 結束程式")
            print("="*30 + "\n")
            
            while True:
                user_input = input("請輸入 UID: ").strip().upper()
                
                if user_input.lower() in ['exit', 'quit']:
                    break
                
                if re.match(r"^[0-9A-F]{8}$", user_input):
                    process_uid(user_input, sb)
                else:
                    print("⚠️ 格式錯誤！請輸入 8 位 16 進位字元。")
        else:
            # 正式比賽或自動監聽模式
            while True:
                time.sleep(1)

    except KeyboardInterrupt:
        pass
    finally:
        logging.info("程式結束。")

if __name__ == "__main__":
    main()
if __name__ == "__main__":
    main()
