import serial
import time
import re

class HM10ESP32Bridge:
    def __init__(self, port, rx_timeout=0.1):
        # 增加緩衝區防止數據斷裂
        self.ser = serial.Serial(port=port, baudrate=115200, timeout=rx_timeout)
        # 修正 Regex 以便更靈活地捕捉 bt_com 標籤
        self.log_regex = re.compile(r'bt_com:\s*(.*)')
        # 移除 ANSI 顏色代碼
        self.ansi_regex = re.compile(r'\x1b\[[0-9;]*m')
        self.line_buffer = ""  # 用於儲存尚未讀取完整的行
        time.sleep(1) 

    def _read_bt_com_payloads(self):
        """核心優化：確保讀取完整的 log 行，防止在雜訊下丟失數據"""
        if self.ser.in_waiting == 0:
            return []

        try:
            # 讀取當前所有可用位元組
            raw_chunk = self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
            self.line_buffer += raw_chunk
        except Exception as e:
            print(f"Read Error: {e}")
            return []

        payloads = []
        # 只有當緩衝區有換行時才處理
        if '\n' in self.line_buffer:
            lines = self.line_buffer.split('\n')
            # 最後一個元素如果不含換行，代表是不完整的行，留到下次讀取
            self.line_buffer = lines.pop() 

            for line in lines:
                line = line.strip()
                if not line:
                    continue
                    
                # 先清掉可能干擾 Regex 的 ANSI 顏色碼
                clean_line = self.ansi_regex.sub('', line)
                match = self.log_regex.search(clean_line)
                
                if match:
                    payload = match.group(1).strip()
                    if payload:
                        payloads.append(payload)
        
        return payloads

    def set_hm10_name(self, name, timeout=2.0):
        """設定名稱並驗證回覆"""
        command = f"AT+NAME{name}"
        self.ser.write(command.encode('utf-8'))
        
        start_time = time.time()
        while (time.time() - start_time) < timeout:
            for entry in self._read_bt_com_payloads():
                if f"OK+SET{name}" in entry:
                    return True
            time.sleep(0.01)
        return False

    def get_hm10_name(self, timeout=2.0):
        """查詢目前名稱"""
        self.ser.write(b"AT+NAME?")
        start_time = time.time()
        while (time.time() - start_time) < timeout:
            for entry in self._read_bt_com_payloads():
                if "OK+NAME" in entry:
                    # 修正：確保回傳乾淨的名稱
                    return entry.replace("OK+NAME", "").strip()
            time.sleep(0.01)
        return None

    def get_status(self, timeout=2.0):
        """檢查連線狀態"""
        self.ser.write(b"AT+STATUS?")
        start_time = time.time()
        while (time.time() - start_time) < timeout:
            for entry in self._read_bt_com_payloads():
                if "OK+CONN" in entry: return "CONNECTED"
                if "OK+UNCONN" in entry: return "DISCONNECTED"
            time.sleep(0.01)
        return "TIMEOUT"

    def reset(self):
        """觸發重置"""
        self.ser.write(b"AT+RESET")
        start_time = time.time()
        while (time.time() - start_time) < 10.0:
            for entry in self._read_bt_com_payloads():
                if "OK+RESET" in entry:
                    print("Reset signal received, waiting for reboot...")
                    time.sleep(6) 
                    return True
            time.sleep(0.01)
        return False

    def listen(self):
        """取得從 BLE 傳回的純數據，並以換行符號分隔多個回傳內容"""
        logs = self._read_bt_com_payloads()
        # 過濾掉所有的 AT 指令回覆 OK+，只留下 Arduino 傳過來的字串
        data_parts = [l for l in logs if not l.startswith("OK+")]
        return "\n".join(data_parts) if data_parts else None

    def send(self, text):
        """將指令發送到 ESP32 並轉發至 HM-10"""
        if text:
            # 確保發送的是 utf-8 字節
            self.ser.write(text.encode('utf-8'))
            self.ser.flush() # 確保緩衝區強制送出
