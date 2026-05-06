#include <ArxContainer.h>
#include <SPI.h>
#include <MFRC522.h>

// =============================
// 1. 硬體接腳定義
// =============================
// 馬達驅動 (TB6612FNG)
const int PWMA = 10;
const int PWMB = 11;
const int AIN2 = 6;
const int AIN1 = 7;
const int BIN2 = 9;
const int BIN1 = 8;

// 紅外線傳感器 (由右至左: rd1 ~ rd5)
#define rd1 A3
#define rd2 A4
#define rd3 A5
#define rd4 A6
#define rd5 A7

// RFID (RC522)
#define RST_PIN 3
#define SS_PIN 2
MFRC522 mfrc522(SS_PIN, RST_PIN);

// =============================
// 2. 全域變數與通訊狀態
// =============================
char nextCommand = ' ';    // 存放預取的下一個節點指令
bool isMoving = false;      // 車子是否已開始移動
int I = 1;     // 1: 右, 2: 左 (全白後退後轉向用)

// =============================
// 3. 基本移動函式
// =============================
void forward(int speed) {
  analogWrite(PWMA, speed / 1.05);
  analogWrite(PWMB, speed);
  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
}

void backward(int speed) {
  analogWrite(PWMA, speed / 1.05);
  analogWrite(PWMB, speed);
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);  digitalWrite(BIN2, HIGH);
}

void leftward(int speed) {
  analogWrite(PWMA, speed / 1.57);
  analogWrite(PWMB, speed);
  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
}

void rightward(int speed) {
  analogWrite(PWMA, speed / 1.05);
  analogWrite(PWMB, speed / 1.5);
  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
}

void stop(int ms) {
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW);
  if (ms > 0) delay(ms);
}

// =============================
// 4. 轉向與特殊動作
// =============================
void leftturn(int speed) {
  analogWrite(PWMA, speed / 4.20);
  analogWrite(PWMB, speed);
  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  while (analogRead(rd5) > 100); 
  delay(370);
}

void rightturn(int speed) {
  analogWrite(PWMA, speed);
  analogWrite(PWMB, speed / 4);
  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  while (analogRead(rd1) > 100);
  delay(300);
}

void backturn(int speed) {
  analogWrite(PWMA, speed / 1.05);
  analogWrite(PWMB, speed);
  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);  digitalWrite(BIN2, HIGH); // 兩輪反向
  while (analogRead(rd1) > 100);
  delay(50);
  while (analogRead(rd1) < 100);
}

// =============================
// 5. 藍牙通訊：索取指令 (K)
// =============================
void requestNextCommand() {
  Serial3.print("K"); // 向 ESP32 發送請求
  unsigned long startTime = millis();
  char received = ' ';
  
  // 等待回傳，超時設為 500ms
  while (received == ' ' && millis() - startTime < 500) {
    if (Serial3.available()) {
      received = (char)Serial3.read();
    }
  }
  
  if (received != ' ') {
    nextCommand = received;
    Serial.print("Next Cmd: "); Serial.println(nextCommand);
  }
}

// =============================
// 6. 初始化
// =============================
void setup() {
  pinMode(PWMA, OUTPUT); pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(PWMB, OUTPUT); pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(rd1, INPUT); pinMode(rd2, INPUT); pinMode(rd3, INPUT); pinMode(rd4, INPUT); pinMode(rd5, INPUT);

  Serial.begin(9600);   // Debug
  Serial3.begin(9600);  // Bluetooth (HM-10)
  
  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println("Waiting for Start Signal...");
  

  while(Serial3.available()) { Serial3.read(); }
  // 等待 ESP32 傳來啟動碼
  bool ready = false;
  while (!ready) {
    if (Serial3.available()) {
      int val = Serial3.read();
      Serial3.println("here");
      Seriak3.println(char(val));
      // 匹配你的啟動碼列表
      if (val==26||val==67||val==96||val==236||val==16||val==248||val==154) {
        ready = true;
      }
    }
  }

  // --- 關鍵：出發前先預取第一步指令 ---
  requestNextCommand();
  
  // 初始出發動作
  forward(190);
  delay(350);
  isMoving = true;
}

// =============================
// 7. 主迴圈
// =============================
void loop() {
  // --- A. 紅外線循跡邏輯 ---
  int s1 = analogRead(rd1), s2 = analogRead(rd2), s3 = analogRead(rd3), s4 = analogRead(rd4), s5 = analogRead(rd5);

  if ((s1 > 50 || s2 > 50) && (s4 > 50 || s5 > 50)) {
    forward(190);
  } else if (s1 > 50 || s2 > 50) {
    rightward(190);
  } else if (s5 > 50 || s4 > 50) {
    leftward(190);
  } else {
    forward(190);
  }

  // --- B. 節點判斷 (遇到橫線) ---
  if (s4 > 80 && s3 > 80 && s2 > 80) {
    char currentToExecute = nextCommand; // 執行先前預取的指令

    if (currentToExecute == 'f') {
      forward(180); delay(120);
      leftward(180); delay(40);
      forward(180); delay(100);
    } else if (currentToExecute == 'r') {
      backward(90); delay(50);
      rightturn(180);
    } else if (currentToExecute == 'l') {
      backward(90); delay(50);
      leftturn(180);
    } else if (currentToExecute == 'b') {
      backturn(70);
    } else if (currentToExecute == 's') {
      while (true) stop(0);
    }

    // --- 關鍵：執行完後立即預取「下一次節點」的指令 ---
    requestNextCommand();
  }

  // --- C. RFID 讀取邏輯 ---
  // 根據你的規則：在 b 指令前後加強讀取
  if (nextCommand == 'b') { // 這裡可根據策略調整讀卡時機
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      char uidBuf[20];
      sprintf(uidBuf, "%02X%02X%02X%02X", 
              mfrc522.uid.uidByte[0], mfrc522.uid.uidByte[1], 
              mfrc522.uid.uidByte[2], mfrc522.uid.uidByte[3]);
      Serial3.print(uidBuf); // 回傳 UID 給 ESP32
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
    }
  }

  // --- D. 死路與全白偵測修正 ---
  if(analogRead(rd1)<30 && analogRead(rd2)<30 && analogRead(rd3)<30 && analogRead(rd4)<30 && analogRead(rd5)<30){
    delay(50); // 防抖動，確認是真的離開了線
    if(analogRead(rd1)<30 && analogRead(rd2)<30 && analogRead(rd3)<30 && analogRead(rd4)<30 && analogRead(rd5)<30){
      
      backward(90); // 發現死路，先往後退
      
      // 進入一個死迴圈，直到重新找到線為止
      while(true){
        if(analogRead(rd1)>50){
          I=1; // 標記右邊有線
          break;
        }
        if(analogRead(rd5)>50){
          I=2; // 標記左邊有線
          break;
        }
      }
      
      // 等待直到中間的感應器 (rd3) 重新壓到線
      while(analogRead(rd3)<50){} 
      delay(150);
      
      // 根據剛才偵測到的方向，把車身修正回線中央
      if(I==1){
        // 持續右轉修正
        while(analogRead(rd3)<50 || (analogRead(rd2)>50 || analogRead(rd4)>50)){
          rightward(180);
        }
      } else {
        // 持續左轉修正
        while(analogRead(rd3)<50 || (analogRead(rd2)>50 || analogRead(rd4)>50)){
          leftward(180);
        }
      }
      delay(100);
    }
  }
}