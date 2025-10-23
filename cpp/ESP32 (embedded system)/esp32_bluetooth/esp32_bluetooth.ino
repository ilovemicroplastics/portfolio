#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_BT");  // Set Bluetooth device name
  
  Serial.println("Bluetooth device is ready to pair");
}

void loop() {
  if (SerialBT.available()) {
    char incomingChar = SerialBT.read();
    Serial.print(incomingChar); // Print the data received from Bluetooth
  }
  
  if (Serial.available()) {
    char outgoingChar = Serial.read();
    SerialBT.write(outgoingChar);  // Send data to the phone via Bluetooth
  }
}

// connect to esp32 via "Serial Bluetooth Terminal" app.