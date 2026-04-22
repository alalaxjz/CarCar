import time
import sys
import threading
import re
import logging
from hm10_esp32 import HM10ESP32Bridge
from score import ScoreboardServer, ScoreboardFake

# --- 1. 比賽與連線參數設定 ---
SERVER_IP = "http://carcar.ntuee.org"
SOCKETIO_PATH = "/scoreboard/socket.io" 
TEAM_NAME = "CarCarTeam_08"          
PORT = '/dev/cu.usbserial-10'  
EXPECTED_NAME = 'HM10_Blue'

# --- 2. 模式切換 ---
USE_FAKE = False     # True: 讀本地 CSV, False: 連線伺服器
MANUAL_MODE = False   # True: 手動輸入 UID, False: 藍牙自動監聽

# --- 3. 預先計算好的路徑指令 (在此複製貼上) ---
# 假設：'F'=前進, 'L'=左轉, 'R'=右轉
RAW_ACTIONS = "fllbrffrrblrrbfbrrffflbrllbrflrrfbfrrlrlbfbllrrffrfbflrrlfbfrrlrr" 
ACTIONS = list(RAW_ACTIONS) 

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    datefmt='%H:%M:%S'
)

def background_listener(bridge, sb, action_list):
    """
    背景監聽執行緒：處理 Arduino 的 ask 請求與 UID 讀取
    """
    if bridge is None:
        return
        
    logging.info("📡 藍牙監聽與指令派發系統已啟動...")
    while True:
        try:
            msg = bridge.listen()
            if not msg:
                time.sleep(0.01)
                continue
            
            clean_msg = msg.strip()

            # --- 處理 Arduino 請求指令 ---
            if clean_msg == "ask":
                if action_list:
                    next_move = action_list.pop(0)
                    bridge.send(next_move)
                    logging.info(f"🤖 Arduino 請求 -> 送出指令: {next_move} (剩餘 {len(action_list)} 步)")
                else:
                    bridge.send('S')
                    logging.warning("⚠️ 指令清單已空，送出 'S' 停止車子")

            # --- 處理 UID 讀取與加分 ---
            elif re.match(r"^[0-9A-F]{8}$", clean_msg.upper()):
                uid = clean_msg.upper()
                logging.info(f"🔔 讀取到 UID: {uid}，正在上傳伺服器...")
                score, time_left = sb.add_UID(uid)
                logging.info(f"✨ [得分] +{score} 分！目前總分: {sb.get_current_score()} | 剩餘時間: {time_left}s")

        except Exception as e:
            logging.error(f"監聽異常: {e}")
        time.sleep(0.01)

def main():
    # --- 第一步：初始化計分系統 ---
    try:
        if USE_FAKE:
            logging.info("⚠️ 模式：測試模式 (讀取本地 CSV)")
            sb = ScoreboardFake(TEAM_NAME, "data/fakeUID.csv")
        else:
            logging.info(f"🚀 模式：伺服器模式 (連接至: {SERVER_IP})")
            # 這裡確保 score.py 內的 socketio_path 有被正確設定
            sb = ScoreboardServer(TEAM_NAME, host=SERVER_IP) 
    except Exception as e:
        logging.critical(f"計分板初始化失敗: {e}")
        return

    # --- 第二步：初始化藍牙與啟動執行緒 ---
    bridge = None
    if not MANUAL_MODE:
        try:
            bridge = HM10ESP32Bridge(port=PORT)
            # 啟動背景監聽，並傳入 ACTIONS 清單
            t = threading.Thread(
                target=background_listener, 
                args=(bridge, sb, ACTIONS), 
                daemon=True
            )
            t.start()
            logging.info(f"✅ 藍牙已連線至 {PORT}，準備接收 ask 指令")
        except Exception as e:
            logging.error(f"藍牙連線失敗: {e}")
            if not USE_FAKE: sys.exit(1)
    else:
        logging.info("💡 手動模式：請在終端機輸入 UID 進行測試")

    # --- 第三步：主運行邏輯 ---
    try:
        if MANUAL_MODE:
            while True:
                user_input = input("請輸入 UID (或輸入 exit 結束): ").strip().upper()
                if user_input in ['EXIT', 'QUIT']: break
                if re.match(r"^[0-9A-F]{8}$", user_input):
                    score, time_left = sb.add_UID(user_input)
                    print(f"得分: {score}, 總分: {sb.get_current_score()}, 剩餘時間: {time_left}")
        else:
            # 正式/自動模式：主執行緒保持守候
            while True:
                time.sleep(1)
    except KeyboardInterrupt:
        logging.info("程式由使用者手動終止")

if __name__ == "__main__":
    main()