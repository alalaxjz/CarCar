//馬達
int PWMA = 10;
int PWMB = 11;
int AIN2 = 6;
int AIN1 = 7;
int BIN2 = 9;
int BIN1 = 8;
//紅外線 rd1 最右邊
#define rd1 A3
#define rd2 A4
#define rd3 A5
#define rd4 A6
#define rd5 A7
//卡片偵測
#include <SPI.h>
#include <MFRC522.h>
#define RST_PIN 3
#define SS_PIN 2
MFRC522 *mfrc522;
int i=0;
//藍芽溝通
#define CUSTOM_NAME "HM10_Blue" // Max length is 12 characters [1]
bool moduleReady = false;

void setup() {
  // 馬達
  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN2, OUTPUT);


  // 紅外線
  pinMode(rd1, INPUT);
  pinMode(rd2, INPUT);
  pinMode(rd3, INPUT);
  pinMode(rd4, INPUT);
  pinMode(rd5, INPUT);

  // USB Debug 與藍芽
  Serial.begin(9600);    // USB Serial Monitor
  Serial3.begin(9600);   // HM-10 藍芽
  delay(2000);           // 等 HM-10 啟動完成

  // RFID
  SPI.begin();
  mfrc522 = new MFRC522(SS_PIN, RST_PIN);
  mfrc522->PCD_Init();

  Serial.println("Initialization Complete.");

}



void loop() {
  //如果為白色
  while (Serial3.available()) {
    int val = Serial3.read();   // 讀取一個 byte
    Serial3.println(val);       // 印出數值

    if (val == 'f') {
        forward(250);
        delay(100);
    } else if (val == 'b') {
        backward(250);
        delay(100);
    } else if (val == 'x') {    // 注意這裡是字元比較
        analogWrite(PWMA, 0);
        analogWrite(PWMB, 0);
        delay(100);
    }
  }

  //偵測
  // RFID 偵測
  if (mfrc522->PICC_IsNewCardPresent() && mfrc522->PICC_ReadCardSerial()) {
    char buffer[20];
    sprintf(buffer, "%2x%2x%2x%2x",
            mfrc522->uid.uidByte[0],
            mfrc522->uid.uidByte[1],
            mfrc522->uid.uidByte[2],
            mfrc522->uid.uidByte[3]);
    Serial3.print(buffer);

    mfrc522->PICC_HaltA();
    mfrc522->PCD_StopCrypto1();
  }
}

/**
 * Helper to send AT commands (Uppercase, no \r or \n) [6]
 */
void sendATCommand(const char* command) {
  Serial3.print(command);
  waitForResponse("", 1000); 
}

/**
 * Helper to check response for specific substrings
 */
bool waitForResponse(const char* expected, unsigned long timeout) {
  unsigned long start = millis();
  Serial3.setTimeout(timeout);
  String response = Serial3.readString();
  if (response.length() > 0) {
    Serial.print("HM10 Response: ");
    Serial.println(response);
  }
  return (response.indexOf(expected) != -1);
}
void forward(int speed){
  analogWrite(PWMA, speed);
  analogWrite(PWMB, speed);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN2, LOW);
  digitalWrite(BIN1, HIGH);}
void backward(int speed){
  analogWrite(PWMA, speed);
  analogWrite(PWMB, speed);
  digitalWrite(AIN2, LOW);
  digitalWrite(AIN1, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);}
  
