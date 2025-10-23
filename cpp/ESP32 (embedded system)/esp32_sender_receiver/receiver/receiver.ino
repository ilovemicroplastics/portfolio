/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp32-uart-communication-serial-arduino/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

// This was also customized by Bartosz Krawczyk

// Define TX and RX pins for UART
#define TXD1 19
#define RXD1 21

// Use Serial1 for UART communication
HardwareSerial mySerial(2);

#define LED_PIN 2

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600, SERIAL_8N1, RXD1, TXD1);  // UART setup
  
  pinMode(LED_PIN, OUTPUT);

  Serial.println("ESP32 UART Receiver");
}

void loop() {
  // Check if data is available to read
  if (mySerial.available()) {
    // Read data and display it
    String message = mySerial.readStringUntil('\n');

    // Convert the received string to an integer
    int receivedNumber = message.toInt();
    
    // Check if the number is even
    if (receivedNumber % 2 == 0) {
      // Blink LED if the number is even
      digitalWrite(LED_PIN, HIGH);  // Turn LED on
      delay(500);                   // Wait for 500 milliseconds
      digitalWrite(LED_PIN, LOW);   // Turn LED off
      delay(500);                   // Wait for 500 milliseconds
    }

    Serial.println("Received: " + message);
  }
}
