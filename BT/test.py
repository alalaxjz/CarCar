import time
import sys
import threading
import re
import logging
from hm10_esp32 import HM10ESP32Bridge
from score import ScoreboardServer, ScoreboardFake

# --- 1. 比賽參數設定 ---
SERVER_IP = "http://140.112.175.18" 
TEAM_NAME = "CarCarTeam_08"          
PORT = '/dev/cu.usbserial-10'  
EXPECTED_NAME = 'HM10_Blue'

# --- 2. 模式切換 ---
USE_FAKE = True  # 測試模式設為 True，比賽設為 False

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    datefmt='%H:%M:%S'
)

def background_listener(bridge, sb):
    """
    監聽執行緒：只有在非測試模式或連接藍牙時才會運作
    """
    if bridge is None:
        return
        
    logging.info("📡 藍牙實體監聽已啟動...")
    while True:
        try:
            msg = bridge.listen()
            if msg:
                uid_candidate = msg.strip().upper()
                if re.match(r"^[0-9A-F]{8}$", uid_candidate):
                    process_uid(uid_candidate, sb)
        except Exception as e:
            logging.error(f"監聽異常: {e}")
        time.sleep(0.01)

def process_uid(uid, sb):
    """
    處理 UID 的核心邏輯 (共用)
    """
    logging.info(f"🔔 讀取到 UID: {uid}")
    score, time_left = sb.add_UID(uid)
    if score > 0:
        logging.info(f"✨ [得分] +{score} 分！總分: {sb.get_current_score()}")
    else:
        logging.info("ℹ️ [提示] 此卡片無效或已重複。")

def main():
    # ------------------------------------------------------
    # 第一步：初始化計分系統 (無論有無藍牙都要初始化)
    # ------------------------------------------------------
    try:
        if USE_FAKE:
            logging.info("⚠️ 模式：測試模式 (無需藍牙，讀取本地 CSV)")
            sb = ScoreboardFake(TEAM_NAME, "data/fakeUID.csv")
        else:
            logging.info(f"🚀 模式：正式比賽 (連接伺服器: {SERVER_IP})")
            sb = ScoreboardServer(TEAM_NAME, host=SERVER_IP)
    except Exception as e:
        logging.critical(f"計分板初始化失敗: {e}")
        return

    # ------------------------------------------------------
    # 第二步：藍牙連線 (僅在 USE_FAKE = False 時執行)
    # ------------------------------------------------------
    bridge = None
    if not USE_FAKE:
        logging.info(f"正在嘗試連線藍牙端口: {PORT}...")
        try:
            bridge = HM10ESP32Bridge(port=PORT)
            # 檢查連線狀態
            if bridge.get_status() != "CONNECTED":
                logging.error("❌ 藍牙未連線！正式模式必須連接車子。")
                sys.exit(1)
            
            # 啟動藍牙監聽執行緒
            listener_thread = threading.Thread(target=background_listener, args=(bridge, sb), daemon=True)
            listener_thread.start()
            logging.info(f"✅ 已成功連線至車子: {EXPECTED_NAME}")
        except Exception as e:
            logging.error(f"藍牙初始化出錯: {e}")
            sys.exit(1)
    else:
        logging.info("跳過藍牙初始化，進入純軟體模擬狀態。")

    # ------------------------------------------------------
    # 第三步：運行邏輯
    # ------------------------------------------------------
    try:
        if USE_FAKE:
            # 模擬自動從 CSV 讀取 UID 並測試
            logging.info("--- 開始模擬讀取 CSV 內容 ---")
            test_uids = list(sb.uid_to_score.keys())
            
            for test_uid in test_uids:
                process_uid(test_uid, sb)
                time.sleep(1) # 每秒模擬讀一張卡
            
            logging.info("--- 模擬測試結束 ---")
            # 測試完後可以保持運行，或直接結束
            #while True: time.sleep(1)
            sys.exit(0)
        else:
            # 正式比賽模式，主執行緒保持守候即可
            while True:
                time.sleep(1)

    except KeyboardInterrupt:
        logging.info("程式由使用者終止")

if __name__ == "__main__":
    main()