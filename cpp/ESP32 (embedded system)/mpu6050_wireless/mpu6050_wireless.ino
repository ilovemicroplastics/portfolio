#include <WiFi.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

const char* ssid = "switch";
const char* password = "password";
IPAddress serverIP(192, 168, 0, 6);  // Enter computer ip address
const int serverPort = 5050;

// use ncat to receive messages
// (in cmd): ncat -lvp 5050

Adafruit_MPU6050 mpu;
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
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip!");
    while (1) {
      delay(1000); // Keep ESP32 in a loop if MPU6050 is not found
    }
  }

  Serial.println("");
  delay(100);
}

void loop() {
  if (!client.connected()) {
    Serial.println("Disconnected. Reconnecting...");
    connectToServer();
  } else {
    sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);


  /* Print out the values */
  client.print("Acceleration X: ");
  client.print(a.acceleration.x);
  client.print(", Y: ");
  client.print(a.acceleration.y);
  client.print(", Z: ");
  client.print(a.acceleration.z);
  client.println(" m/s^2");

  client.print("Rotation X: ");
  client.print(g.gyro.x);
  client.print(", Y: ");
  client.print(g.gyro.y);
  client.print(", Z: ");
  client.print(g.gyro.z);
  client.println(" rad/s");

  client.print("Temperature: ");
  client.print(temp.temperature);
  client.println(" degC");

  client.println("");
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