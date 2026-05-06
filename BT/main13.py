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

RAW_ACTIONS = "frfrrblrrbfbrrffflbrllbrflrrfbfrrlrlbfbllrrffrfbflrflrrbllrllfbfrffrf" 
ACTIONS = list(RAW_ACTIONS) 
ACTIONS.pop(0)

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
        
    logging.info("📡 藍牙實體監聽已啟動，等待 Arduino 請求指令 (K)...")
    
    while True:
        try:
            msg = bridge.listen()
            if msg:
                print(f"DEBUG: 收到來自藍牙的原生數據: {repr(msg)}")
                
                # 轉大寫並去除空白
                clean_msg = msg.strip().upper()
                
                # --- 1. 處理 Arduino 要求下一個動作 ('K') ---
                if clean_msg == "K":
                    if len(action_list) > 0:
                        # 取出 list 最前面的字元並移除
                        next_move = action_list.pop(0)
                        logging.info(f"⚡ 收到請求 'K'，發送指令: {next_move} (剩餘 {len(action_list)} 步)")
                        bridge.send(next_move)
                    else:
                        # List 為空，發送 'S' 代表停止
                        logging.info("🏁 路徑已全數傳送完畢，發送停止訊號 'S'")
                        bridge.send("s")

                # --- 2. 處理 UID 讀取 (得分邏輯維持不變) ---
                elif re.match(r"^[0-9A-F]{8}$", clean_msg):
                    process_uid(clean_msg, sb)
                
                # 處理未補滿 8 位的 UID
                elif re.match(r"^[0-9A-F]+$", clean_msg) and len(clean_msg) < 8:
                    process_uid(clean_msg.zfill(8), sb)
                    
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
    # 第一步：先連線藍牙 (ESP32 / HM-10)
    # ------------------------------------------------------
    if not MANUAL_MODE:
        logging.info("正在連線藍牙...")
        try:
            bridge = HM10ESP32Bridge(port=PORT) 
        except Exception as e:
            logging.error(f"無法開啟串口 {PORT}: {e}")
            sys.exit(1)
        
        # 檢查名稱並確認連線狀態 (維持你原本的邏輯)
        current_name = bridge.get_hm10_name()
        if current_name != EXPECTED_NAME:
            logging.info(f"更新目標名稱為 {EXPECTED_NAME}...")
            if bridge.set_hm10_name(EXPECTED_NAME):
                bridge.reset()
                bridge = HM10ESP32Bridge(port=PORT)
            else:
                logging.error("無法設定名稱，退出程式。")
                sys.exit(1)

        status = bridge.get_status()
        if status != "CONNECTED":
            logging.warning(f"⚠️ ESP32 狀態為 {status}。請確認 HM-10 已開啟。")
            sys.exit(0)
        
        logging.info(f"✨ 藍牙已連線至 {EXPECTED_NAME}")
    else:
        logging.info("💡 手動模式：跳過藍牙連線。")

    # ------------------------------------------------------
    # 第二步：初始化計分系統 (伺服器)
    # ------------------------------------------------------
    try:
        if USE_FAKE:
            logging.info("⚠️ 模式：測試模式 (讀取本地 CSV)")
            sb = ScoreboardFake(TEAM_NAME, "data/fakeUID.csv")
        else:
            logging.info(f"🚀 模式：伺服器模式 (連接至: {SERVER_IP})")
            sb = ScoreboardServer(TEAM_NAME, host=SERVER_IP, debug=True)
        logging.info("✅ 計分板伺服器連線成功！")
    except Exception as e:
        logging.critical(f"計分板連線失敗: {e}")
        return

    # ------------------------------------------------------
    # 第三步：發送啟動訊號給 Arduino 並啟動監聽
    # ------------------------------------------------------
    if not MANUAL_MODE and bridge:
        # 這裡發送 Arduino setup() 裡面在等待的啟動碼
        # 假設發送 26 (注意：如果 Arduino 用 Serial.read()，請傳送對應的 byte 或字元)
        START_SIGNAL = chr(26)
        logging.info(f"📢 所有連線就緒，發送啟動訊號給 Arduino...")
        bridge.send(START_SIGNAL) 

        time.sleep(0.5)
        # 啟動藍牙監聽執行緒
        threading.Thread(target=background_listener, args=(bridge, sb, ACTIONS), daemon=True).start()
        # 啟動手動指令輸入執行緒
        #threading.Thread(target=manual_input_thread, args=(bridge,), daemon=True).start()

    # ------------------------------------------------------
    # 第四步：運行主循環
    # ------------------------------------------------------
    try:
        if MANUAL_MODE:
            # (手動輸入 UID 邏輯維持不變...)
            while True:
                user_input = input("請輸入 UID: ").strip().upper()
                if user_input.lower() in ['exit', 'quit']: break
                if re.match(r"^[0-9A-F]{8}$", user_input): process_uid(user_input, sb)
        else:
            logging.info("🏎️ 比賽進行中，等待 K 請求或 UID 回傳...")
            while True:
                time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        logging.info("程式結束。")
        
def manual_input_thread(bridge):
    """在自動模式下也能手動輸入指令"""
    print("\n💡 手動輸入功能已啟動 (自動模式)")
    print("輸入指令字串 (如: SSSSSS)，輸入 'exit' 結束手動輸入\n")
    while True:
        user_input = input("請輸入指令: ").strip()
        if user_input.lower() in ['exit', 'quit']:
            break
        elif re.match(r"^[a-z]+$", user_input):
            bridge.send(user_input)
            logging.info(f"📡 手動送出指令: {user_input}")
        else:
            print("⚠️ 格式錯誤！請輸入合法指令字母。")


if __name__ == "__main__":
    main()

