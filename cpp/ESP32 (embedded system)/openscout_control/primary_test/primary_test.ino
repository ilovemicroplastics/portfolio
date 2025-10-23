// Define TX and RX pins for UART
#define TXD2 19  // Transmit pin (connect to RX of receiving ESP32)
#define RXD2 21  // Receive pin (connect to TX of receiving ESP32)

#define LED_PIN 2
#define NMOTORS 4

// Use Serial1 for UART communication
HardwareSerial mySerial(2);

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  mySerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  
  // Seed random number generator
  randomSeed(analogRead(0));
  
  Serial.println("ESP32 UART Transmitter");
}

void loop() {
  // Generate test data for each motor
  for (int motorIndex = 0; motorIndex < NMOTORS; motorIndex++) {
    // Randomly generate motor control parameters
    int dir = random(-1, 2);  // -1, 0, or 1
    int pwmVal = random(0, 256);  // 0-255 PWM value
    
    // Create the motor control string
    String motorData = "Dir:" + String(dir) + 
                       ",PWMVal:" + String(pwmVal) + 
                       ",PWM:" + String(motorIndex) + "\n";

    // Send data via UART
    mySerial.print(motorData);
    
    // Print to local serial for debugging
    Serial.print("test data...\n");
    Serial.print(motorData);
    
    // Blink LED to indicate transmission
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    
    // Wait a bit between motor commands
    delay(500);
  }
  
  // Overall delay between complete motor cycle
  delay(1000);
}