#include <Arduino.h>

// This program runs on the OpenScout robot.


// Motor pin definitions
const int PWM[] =   {18, 19, 21, 22};  // PWM pins
const int INP_1[] = {15, 4, 15, 4};    // Input 1 pins
const int INP_2[] = {16, 17, 16, 17};  // Input 2 pins

void setMotor(int dir, int PWMVal, int PWM, int INP_1, int INP_2) {
  // Motor control logic
  if (dir == 1) {
    digitalWrite(INP_1, LOW);
    digitalWrite(INP_2, HIGH);
  }
  else if (dir == -1) {
    digitalWrite(INP_1, HIGH);
    digitalWrite(INP_2, LOW);
  }
  else {
    digitalWrite(INP_1, LOW);
    digitalWrite(INP_2, LOW);
  }
  
  // Set motor speed
  ledcWrite(PWM, PWMVal);
}

void setup() {
  // Initialize motor pins
  for(int k = 0; k < 4; k++){
    pinMode(PWM[k], OUTPUT);
    pinMode(INP_1[k], OUTPUT);
    pinMode(INP_2[k], OUTPUT);
    
    // Attach PWM channel to pin
    ledcAttach(PWM[k], 5000, 8);  // Pin, frequency, resolution
  }
}

void loop() {
  // Move all motors forward at full speed
  for(int k = 0; k < 4; k++){
    setMotor(1, 255, PWM[k], INP_1[k], INP_2[k]);
  }
  
  // Keep moving for 2 seconds
  delay(2000);
  
  // Stop all motors
  for(int k = 0; k < 4; k++){
    setMotor(0, 0, PWM[k], INP_1[k], INP_2[k]);
  }
  
  delay(1000);

  // backwards
  for(int k = 0; k < 4; k++){
    setMotor(-1, 255, PWM[k], INP_1[k], INP_2[k]);
  }

  delay(2000);

  // stop
  for(int k = 0; k < 4; k++){
    setMotor(0, 0, PWM[k], INP_1[k], INP_2[k]);
  }
  
  // Wait for 1 second before repeating
  delay(1000);
}
