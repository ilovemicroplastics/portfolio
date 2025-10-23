// Define TX and RX pins for UART (change if needed)
#define TXD2 19
#define RXD2 21
#define NMOTORS 4
const int validPWM[] = {16, 17, 4, 5};        // PWM pins
const int INP_1[] = {12, 13, 14, 15};    // Input 1 pins
const int INP_2[] = {25, 26, 27, 33};    // Input 2 pins
#define LED_PIN 2

// Use Serial1 for UART communication
HardwareSerial mySerial(2);

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
  // Set PWM value (you can modify this if you're using actual PWM pins for speed control)
  analogWrite(PWM, PWMVal);
}

bool isValidPWM(int pin) {
    for (int i = 0; i < sizeof(validPWM) / sizeof(validPWM[0]); i++) {
        if (validPWM[i] == pin) {
            return true;
        }
    }
    return false;
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200, SERIAL_8N1, RXD2, TXD2);  // UART setup
 
  pinMode(LED_PIN, OUTPUT);
  for(int k = 0; k < NMOTORS; k++) {
    pinMode(validPWM[k], OUTPUT);
    pinMode(INP_1[k], OUTPUT);
    pinMode(INP_2[k], OUTPUT);
  }
  Serial.println("ESP32 UART Receiver");
}

void loop() {
  // Check if data is available to read from the UART
  if (mySerial.available() > 0) {
    // Read the incoming data with a timeout
    String receivedData = mySerial.readStringUntil('\n');
    receivedData.trim();  // Remove any leading/trailing whitespace
    Serial.println("Received: " + receivedData);
    // Check if the received data is not empty
    if (receivedData.length() > 0) {
      // Print the received data to the Serial Monitor for debugging
    
   
      // Validate the received data format
      if (receivedData.startsWith("Dir:") && 
          receivedData.indexOf(",PWMVal:") != -1 && 
          receivedData.indexOf(",PWM:") != -1) {
        
        // Parse the received data
        int dirIndex = receivedData.indexOf("Dir:") + 4;
        int pwmValIndex = receivedData.indexOf("PWMVal:") + 7;
        int pwmIndex = receivedData.indexOf("PWM:") + 4;
        
        int dir = receivedData.substring(dirIndex, receivedData.indexOf(",", dirIndex)).toInt();
        int PWMVal = receivedData.substring(pwmValIndex, receivedData.indexOf(",", pwmValIndex)).toInt();
        int PWM = receivedData.substring(pwmIndex).toInt();
   
        // Process the motor control data with additional validation
        if (isValidPWM(PWM)) {
          //setMotor(dir, PWMVal, PWM[PWM], INP_1[PWM], INP_2[PWM]);
              // REMEMBER TO UNCOMMENT THIS FOR THE REAL CODE
        } else {
          Serial.println("Invalid PWM channel");
        }
      } else {
        Serial.println("Invalid data format");
      }
    }
    
    // Small delay to prevent rapid repeated readings
    delay(50);
  }
}