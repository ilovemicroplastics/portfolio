//#include <Arduino.h>
#include <mutex>
#include <BluetoothSerial.h>
#include "esp_timer.h"

#define NMOTORS 4
#define UMAX 255
#define PGAIN 2
#define DGAIN 0.015
#define IGAIN 5
#define VMAX 12

// Updated pin definitions for ESP32
const int ENC_A[] = {25, 26, 18, 5};   // Encoder A pins
const int ENC_B[] = {22, 23, 17, 16};  // Encoder B pins
const int PWM[] = {16, 17, 4, 5};    // PWM pins

const float linearMax = 1.0;
const float angularMax = 1.5;

volatile long pos_i[] = {0,0,0,0};

// Bluetooth Serial Object
BluetoothSerial SerialBT;

// Mutex for synchronization
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

long previous_pos[] = {0,0,0,0};
float velocity[] = {0,0,0,0};
long prevT = 0;
float deltaT = 0;

int N = 1440;
double wheelRadius = 0.056;
double wheelDistance = 0.395;

float throttleValue = 0.0;
float steeringValue = 0.0;
float targetSpeed[] = {0,0,0,0};
float currentSpeed[] = {0,0,0,0};
float previousSpeed[] = {0,0,0,0};

// UART communication
#define TXD2 19
#define RXD2 21
HardwareSerial mySerial(2);  // Use Serial2 on ESP32

class SimplePID{
  private:
    float kp, kd, ki, umax;
    float eprev, eintegral;

  public:
    SimplePID() : kp(2), kd(1), ki(5), umax(15), eprev(0.0), eintegral(0.0){}

    void setParams(float kpIn, float kdIn, float kiIn, float umaxIn){
      kp = kpIn; kd = kdIn; ki = kiIn; umax = umaxIn;
    }

    void evalu(float value, float target, float deltaT, int &pwr, int &dir){
      float e = target - value;
      float dedt = (e-eprev)/(deltaT);
      eintegral = eintegral + (0.5*(eprev+e)*deltaT);
      float u = kp*e + kd*dedt + ki*eintegral;

      dir = 1;
      if(u<0){
        dir = -1;
      }

      pwr = int(255 * (abs(u)/VMAX));
      if(pwr > umax){
        pwr = umax;
      } else if(pwr < 20){
        pwr = 0;
      }

      if(eintegral > 10){
        eintegral = 10;
      } else if(eintegral < -10){
        eintegral = -10;
      }
      
      eprev = e;
    }
};

SimplePID motor[NMOTORS];

void setMotor(int dir, int PWMVal, int PWM){ //, int INP_1, int INP_2
  // Instead of direct motor control, send UART command
  String motorCommand = "Dir:" + String(dir) + 
                        ",PWMVal:" + String(PWMVal) + 
                        ",PWM:" + String(PWM) + "\n";
  
  mySerial.print(motorCommand);
  Serial.print(motorCommand);
}

template <int j>
void readEncoder(){
  portENTER_CRITICAL_ISR(&mux);
  int b = digitalRead(ENC_B[j]);
  if((j == 0) || (j == 2)){
    if(b > 0){
      pos_i[j]++;
    }
    else{
      pos_i[j]--;
    }
  } else if((j == 1) || (j == 3)){
    if(b > 0){
      pos_i[j]--;
    }
    else{
      pos_i[j]++;
    }
  }
  portEXIT_CRITICAL_ISR(&mux);
}

volatile int count = 0;
int count_prev = 0;

// Timer callback function
void onTimer(void *arg) {
    count++;
}

// Timer handle
esp_timer_handle_t timer;

void setup(){
  Serial.begin(115200);
  SerialBT.begin("RobotController"); // Bluetooth device name
  mySerial.begin(115200, SERIAL_8N1, RXD2, TXD2);  // UART setup

  // Timer setup for ESP32
  esp_timer_create_args_t timer_args = {
      .callback = &onTimer,
      .arg = NULL,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "my_timer"
  };

  esp_err_t err = esp_timer_create(&timer_args, &timer);
  if (err != ESP_OK) {
      Serial.println("Timer creation failed!");
  }

  // Start periodic timer (5000 µs = 5ms)
  err = esp_timer_start_periodic(timer, 5000);
  if (err != ESP_OK) {
      Serial.println("Timer start failed!");
  }


  for(int k = 0; k < NMOTORS; k++){
    pinMode(ENC_A[k],INPUT);
    pinMode(ENC_B[k],INPUT);
    motor[k].setParams(PGAIN,DGAIN,IGAIN,UMAX);
  }
 
  attachInterrupt(digitalPinToInterrupt(ENC_A[0]),readEncoder<0>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[1]),readEncoder<1>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[2]),readEncoder<2>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[3]),readEncoder<3>,RISING);
}

void loop(){
  if(count > count_prev){
    long currT = micros();
    float deltaT = ((float)(currT-prevT))/1.0e6;
  
    // Read Bluetooth data
    if(SerialBT.available()){
      String btData = SerialBT.readStringUntil('\n');
      int commaIndex = btData.indexOf(',');
      if(commaIndex != -1){
        throttleValue = btData.substring(0, commaIndex).toFloat();
        steeringValue = btData.substring(commaIndex + 1).toFloat();
      }
    }

    // Deadzone handling
    if((throttleValue >= -0.10) && (throttleValue <= 0.10)){
      throttleValue = 0.0;
    }
    if((steeringValue >= -0.25) && (steeringValue <= 0.25)){
      steeringValue = 0.0;
    }

    for(int i=0; i<4; i++){
      targetSpeed[i] = 0.0;
    }

    //Serial.print(throttleValue);
    //Serial.print('\t');
    //Serial.println(steeringValue);

    if(throttleValue < -1.5){
      throttleValue = 0.0;
    }
    if(steeringValue > 1.5){
      steeringValue = 0.0;
    }

    // Replace ATOMIC_BLOCK with critical section
    portENTER_CRITICAL(&mux);
    for(int k=0; k<NMOTORS; k++){
      float delta_pos = pos_i[k] - previous_pos[k];
      velocity[k] = delta_pos / N / deltaT * 2 * PI;
      previous_pos[k] = pos_i[k];
    }
    portEXIT_CRITICAL(&mux);

    float linearSpeed = linearMax * throttleValue;
    float angularSpeed = angularMax * steeringValue;
    float rightSpeed = ((linearSpeed + (wheelDistance * angularSpeed)) / wheelRadius);
    float leftSpeed = ((linearSpeed - (wheelDistance * angularSpeed)) / wheelRadius);

    targetSpeed[0] = rightSpeed;
    targetSpeed[2] = rightSpeed;
    targetSpeed[1] = leftSpeed;
    targetSpeed[3] = leftSpeed;
    
    for(int k=0; k<NMOTORS; k++){
      currentSpeed[k] = 0.854*currentSpeed[k] + 0.0728*velocity[k] + 0.0728*previousSpeed[k];
      previousSpeed[k] = velocity[k];
    }

    for(int k=0; k<NMOTORS; k++){
      int pwr, dir;
      motor[k].evalu(velocity[k], targetSpeed[k], deltaT, pwr, dir);

      if((targetSpeed[k] < 0.1) && (targetSpeed[k] > -0.1)){
        pwr = 0;
        dir = 0;
      }
      setMotor(dir, pwr, PWM[k]); // , INP_1[k], INP_2[k]
    }

    float rightVirtualWheelSpeed = (velocity[0] + velocity[2]) * 0.5;
    float leftVirtualWheelSpeed = (velocity[1] + velocity[3]) * 0.5;

    prevT = currT;
    count_prev = count;

  }

}
