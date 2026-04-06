int PWMA = 10;
int PWMB = 11;
int AIN2 = 6;
int AIN1 = 7;
int BIN2 = 9;
int BIN1 = 8;
//紅外線 rd1 最右邊
#define R3 A3
#define R2 A4
#define M A5
#define L2 A6
#define L3 A7

int r3; int r2; int m; int l2; int l3;

//PID參數 調整順序 P->D->I
float Kp = 0.8;
float Ki = 0;
float Kd = 0.2;

int lastError = 0;
float integral = 0;
const int TARGET_SPEED = 160;

void setup() {
  
  // 馬達
  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  // 紅外線
  pinMode(R3, INPUT);
  pinMode(R2, INPUT);
  pinMode(M, INPUT);
  pinMode(L2, INPUT);
  pinMode(L3, INPUT);

  //開始
 
  MotorWrite(160, 160);
}

void loop() {

  l3 = analogRead(L3);
  l2 = analogRead(L2);
  m = analogRead(M);
  r2 = analogRead(R2);
  r3 = analogRead(R3);

  float error = calculateError();

  float P = error;
  integral += error;
  float D = error - lastError;

  float turn = Kp * P + Ki * integral + Kd * D;
  lastError = error;

  int speedA = TARGET_SPEED - turn; //left
  int speedB = TARGET_SPEED + turn;

  speedA = constrain(speedA, 0, 255);
  speedB = constrain(speedB, 0, 255);

  if(r2>100 && m>100 && l2>100) {
    MotorWrite(0, 0);
    delay(5000);
  }

  MotorWrite(speedA, speedB);
}

float calculateError() {
  

  int weight = (-2 * l3) + (-1 * l2) + 0 * m + 1 * r2 + 2 * r3;
  return -weight/10.0;
}

// float constrain(int x, int low, int high) {
//   if(x>high)
//     return high;
//   else if(x<low)
//     return low;
//   else
//     return x;

// }

void MotorWrite(int l, int r) {
  analogWrite(PWMA, l);
  analogWrite(PWMB, r);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN2, LOW);
  digitalWrite(BIN1, HIGH);
}











