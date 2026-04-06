//馬達
#include <ArxContainer.h>

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
//藍芽溝通
#define CUSTOM_NAME "HM10_Blue" // Max length is 12 characters [1]
bool moduleReady = false;

int I =1;
std::vector<char> v{'f','l','f','b','f','r','f','b'};
void setup() {
  // 馬達
  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);

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

  //開始
 
  forward(160);
}



void loop() {
  
  if((analogRead(rd1)>50 || analogRead(rd2)>50)&&(analogRead(rd4)>50 || analogRead(rd5)>50)){
    forward(160);
  }else if(analogRead(rd1)>50 || analogRead(rd2)>50){
    rightward(160);
  }else if(analogRead(rd5)>50 || analogRead(rd4)>50){
    leftward(160);
  }else{
    forward(160);
  }
  if(analogRead(rd4)>100 && analogRead(rd3)>100 && analogRead(rd2)>100){
    delay(30);
    if(v[0]=='f'){
	    forward(160);
	    delay(140);
	    leftward(150);
	    delay(70);
	    forward(160);
	    delay(140);
    }else if(v[0]=='r'){
	    rightturn(160);
    }else if(v[0]=='l'){
	    leftturn(160);
    }else if(v[0]=='b'){
	    backturn(80);
    }
    v.push_back(v[0]);
    v.erase(v.begin());
  }
  if(analogRead(rd1)<30 && analogRead(rd2)<30&&analogRead(rd3)<30&&analogRead(rd4)<30 && analogRead(rd5)<30){
    delay(50);
    if(analogRead(rd1)<30 && analogRead(rd2)<30&&analogRead(rd3)<30&&analogRead(rd4)<30 && analogRead(rd5)<30){
      backward(70);
      while(true){
        if(analogRead(rd1)>50){
          I=1;
          break;
        }
        if(analogRead(rd5)>50){
          I=2;
          break;
        }
      }
      delay(100);
      if(I==1){
        while(analogRead(rd3)<50&&(analogRead(rd2)>50||analogRead(rd4)>50)){
          rightward(160);
        }
      }else{
        while(analogRead(rd3)<50&&(analogRead(rd2)>50||analogRead(rd4)>50)){
          leftward(160);
        }
      }
    }
  }
  while (Serial3.available()) {
    char cmd[3];
    sprintf(cmd, "%2x",
            Serial3.read());
    Serial3.print("收到字元: ");
    Serial3.println(cmd);
    
    // 過濾換行符號，只處理有效的指令字元
    if (cmd == 'f') {
      forward(250);
      delay(100);
    } else if (cmd == 'b') {
      backward(250);
      delay(100);
    } else if (cmd == 'x') {
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
void forward(int speed){ //A左輪
  analogWrite(PWMA, speed/1.05);
  analogWrite(PWMB, speed);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN2, LOW);
  digitalWrite(BIN1, HIGH);}
void backward(int speed){
  analogWrite(PWMA, speed/1.05);
  analogWrite(PWMB, speed);
  digitalWrite(AIN2, LOW);
  digitalWrite(AIN1, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);}
void leftward(int speed){ //A左輪
  analogWrite(PWMA, speed/1.57);
  analogWrite(PWMB, speed);
  digitalWrite(BIN2, LOW);
  digitalWrite(BIN1, HIGH);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);}
void rightward(int speed){ //A左輪
  analogWrite(PWMA, speed/1.05);
  analogWrite(PWMB, speed/1.5);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN2, LOW);
  digitalWrite(BIN1, HIGH);}
void stop(int speed){ //A左輪
  analogWrite(PWMA, speed/1.05);
  analogWrite(PWMB, speed);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN2, LOW);
  digitalWrite(BIN1, LOW);
  delay(600);}
void leftturn(int speed){ //A左輪
  analogWrite(PWMA, speed/4.20);
  analogWrite(PWMB, speed);
  digitalWrite(BIN2, LOW);
  digitalWrite(BIN1, HIGH);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  while(analogRead(rd5)>100){}
  delay(420);}
void rightturn(int speed){ //A左輪
  analogWrite(PWMA, speed/1.05);
  analogWrite(PWMB, speed/4);
  digitalWrite(BIN2, LOW);
  digitalWrite(BIN1, HIGH);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  while(analogRead(rd1)>100){}
  delay(420);}
void backturn(int speed){ //A左輪
  analogWrite(PWMA, speed/1.05);
  analogWrite(PWMB, speed);
  digitalWrite(BIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  while(analogRead(rd1)>100){}
  delay(50);
  while(analogRead(rd1)<100){}}
