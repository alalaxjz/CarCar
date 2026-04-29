//馬達
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
int i =0;
int I =1;
int star =0;
//char s[]  = "llbrffrrblrrbfbrrffflbrllbrflrrfbfrrlrlbfbllrrffrfbflrrlfbfrrlrr";
//char s[]  = "rfrrblrrbfbrrffflbrllbrflrrfbfrrlrlbfbllrrffrfbflrrlfbfrrlrr";
char s[]  = "flfbfrrlrbllflbfblffrbflfbfrrlrbllflbfblffrbflfbfrrlrbllflbfblffrbflfbfrrlrbllflbfblffrb";

//std::vector<char> v{'l','l','r','b','l','l'};

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
  delay(1000);           // 等 HM-10 啟動完成

  // RFID
  SPI.begin();
  mfrc522 = new MFRC522(SS_PIN, RST_PIN);
  mfrc522->PCD_Init();

  
  // while (Serial3.available()) {
  //   Serial3.read();
  // }
  // while(star==0){
  //   Serial3.print("K");
  //   delay(50);
  //   while(Serial3.available()){
  //     int val = Serial3.read();
  //     Serial3.print("GET");
  //     Serial3.println(val);
  //     if(val==26||val==67||val==96||val==236||val==16||val==248||val==154||val==12){
  //       star = 1;
  //     }
  //   }
  // }
  forward(190);
  delay(350);
  // while(true){
  //   backturn(75);
  //   delay(50);
  // }
}



void loop() {
  if((analogRead(rd1)>50 || analogRead(rd2)>50)&&(analogRead(rd4)>50 || analogRead(rd5)>50)){
    forward(190);
  }else if(analogRead(rd1)>50 || analogRead(rd2)>50){
    rightward(190);
  }else if(analogRead(rd5)>50 || analogRead(rd4)>50){
    leftward(190);
  }else{
    forward(190);
  }
  
  if(analogRead(rd4)>80 && analogRead(rd3)>80 && analogRead(rd2)>80 ){
    if(s[i]=='f'){
	    forward(180);
	    delay(80);
	    leftward(180);
	    delay(60);
	    forward(180);
	    delay(60);
    }else if(s[i]=='r'){
      backward(90);
      delay(20);
	    rightturn(180);
    }else if(s[i]=='l'){
      backward(90);
      delay(20);
	    leftturn(180);
    }else if(s[i]=='b'){
	    backturn(75);
    }
    i++;
    
  }
  if (mfrc522->PICC_IsNewCardPresent() && mfrc522->PICC_ReadCardSerial()) {
    char buffer[20];
    sprintf(buffer, "%02x%02x%02x%02x",
            mfrc522->uid.uidByte[0],
            mfrc522->uid.uidByte[1],
            mfrc522->uid.uidByte[2],
            mfrc522->uid.uidByte[3]);
    Serial3.print(buffer);

    mfrc522->PICC_HaltA();
    mfrc522->PCD_StopCrypto1();
  }
  if(analogRead(rd1)<30 && analogRead(rd2)<30&&analogRead(rd3)<30&&analogRead(rd4)<30 && analogRead(rd5)<30){
    delay(35);
    if(analogRead(rd1)<30 && analogRead(rd2)<30&&analogRead(rd3)<30&&analogRead(rd4)<30 && analogRead(rd5)<30){
      backward(90);
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
      while(analogRead(rd3)<50){}
      delay(20);
      if(I==1){
        while(analogRead(rd3)<50||(analogRead(rd2)>50||analogRead(rd4)>50)){
          rightward(180);
        }
      }else{
        while(analogRead(rd3)<50||(analogRead(rd2)>50||analogRead(rd4)>50)){
          leftward(180);
        }
      }
      delay(30);
      forward(190);
      delay(100);
    }
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
  delay(350);}
void rightturn(int speed){ //A左輪
  analogWrite(PWMA, speed/1.05);
  analogWrite(PWMB, speed/4);
  digitalWrite(BIN2, LOW);
  digitalWrite(BIN1, HIGH);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  while(analogRead(rd1)>100){}
  delay(260);}
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
  
  
