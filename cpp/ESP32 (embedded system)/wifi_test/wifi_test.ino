#include <WiFi.h>

const char* ssid = "";
const char* password = "";
IPAddress serverIP(192, 168, 0, 6);  // Enter computer ip address
const int serverPort = 5050;

// use ncat to receive messages
// (in cmd): ncat -lvp 8080 


WiFiClient client;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());

  connectToServer();
}

void loop() {
  if (!client.connected()) {
    Serial.println("Disconnected. Reconnecting...");
    connectToServer();
  } else {
    client.println("Hello from ESP32!");
    Serial.println("Message sent to server.");
  }
  delay(2000); // Send message every 2 seconds
}

void connectToServer() {
  if (client.connect(serverIP, serverPort)) {
    Serial.println("Connected to PC!");
  } else {
    Serial.println("Connection to server failed!");
  }
}