#include <WiFi.h> // Use WiFi.h for ESP32
#include <PubSubClient.h>
#include <ArduinoJson.h>        // For parsing JSON input

// WiFi settings
char ssid[] = "";  // Network SSID
char pass[] = "";  // WPA key

// MQTT Broker settings
const char *mqtt_broker = "broker.emqx.io"; // EMQX broker endpoint
const char *mqtt_topic = "robot/velocity"; // MQTT topic
const char *mqtt_username = "emqx"; // MQTT username for authentication
const char *mqtt_password = "public"; // MQTT password for authentication
const int mqtt_port = 1883; // MQTT port (TCP)

WiFiClient wifiClient;
PubSubClient mqtt_client(wifiClient);

#define NMOTORS 4
#define UMAX 255
#define PGAIN 2
#define DGAIN 0.015
#define IGAIN 5
#define VMAX 12

//                    Declare Pins
//                  {RB, LB, RF, LF}
const int ENC_A[] = {26, 27, 14, 13};  // Encoder A pins     4 pins
const int ENC_B[] = {23, 32, 33, 25};  // Encoder B pins     4 pins
const int PWM[] =   {18, 19, 21, 22};  // PWM pins           4 pins
const int INP_1[] = {15, 4 , 15, 4 };  // Input 1 pins       2 pins 
const int INP_2[] = {16, 17, 16, 17};  // Input 2 pins       2 pins

float throttleValue = 0.0;
float steeringValue = 0.0;
const float linearMax = 1.0;
const float angularMax = 1.5;

volatile long pos_i[] = {0,0,0,0};

void connectToWiFi();
void connectToMQTTBroker();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void processTwistMessage(const char *payload);

// Mutex for synchronization
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

long previous_pos[] = {0,0,0,0};
float velocity[] = {0,0,0,0};
long prevT = 0;
float deltaT = 0;

int N = 1440;
double wheelRadius = 0.056;
double wheelDistance = 0.395;

float targetSpeed[] = {0,0,0,0};
float currentSpeed[] = {0,0,0,0};
float previousSpeed[] = {0,0,0,0};

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
  
  // veloctiy control
  ledcWrite(PWM, PWMVal);
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
  connectToWiFi();
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();

  // Timer setup for ESP32
  esp_timer_create_args_t timer_args = {
      .callback = &onTimer,
      .arg = NULL,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "timer"
  };

  esp_err_t err = esp_timer_create(&timer_args, &timer);
  // Start periodic timer (1000 µs = 1ms)
  err = esp_timer_start_periodic(timer, 1000);

  for(int k = 0; k < NMOTORS; k++){
    pinMode(ENC_A[k],INPUT);
    pinMode(ENC_B[k],INPUT);
    pinMode(PWM[k], OUTPUT);
    pinMode(INP_1[k], OUTPUT);
    pinMode(INP_2[k], OUTPUT);
    motor[k].setParams(PGAIN,DGAIN,IGAIN,UMAX);
  }
  

  for(int k = 0; k < NMOTORS; k++){
    // Replace ledcSetup and ledcAttachPin with ledcAttach
    ledcAttach(PWM[k], 5000, 8);  // Pin, frequency, resolution
  }

  attachInterrupt(digitalPinToInterrupt(ENC_A[0]),readEncoder<0>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[1]),readEncoder<1>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[2]),readEncoder<2>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[3]),readEncoder<3>,RISING);
}

void connectToWiFi() {
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

void connectToMQTTBroker() {
    while (!mqtt_client.connected()) {
        String client_id = "esp32-client-" + String(WiFi.macAddress());

        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            mqtt_client.subscribe(mqtt_topic);

        } else {
            delay(5000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    // Convert payload (byte array) to a String
    String msg;
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }

    // Pass the message as a C-string
    processTwistMessage(msg.c_str());
}

// Process the incoming twist message and parse the linear and angular velocities
void processTwistMessage(const char *payload) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (!error) {
    float linear_x = doc["linear"]["x"];
    float linear_y = doc["linear"]["y"];
    float linear_z = doc["linear"]["z"];
    float angular_x = doc["angular"]["x"];
    float angular_y = doc["angular"]["y"];
    float angular_z = doc["angular"]["z"];

    // For this robot, we care only about the x component of linear and z component of angular velocity
    throttleValue = linear_x;  // Linear velocity (x-axis)
    steeringValue = angular_z; // Angular velocity (z-axis)

  }
}

void loop(){
  if (!mqtt_client.connected()) {
    connectToMQTTBroker();
  }
  mqtt_client.loop();

  if(count > count_prev){
    long currT = micros();
    float deltaT = ((float)(currT-prevT))/1.0e6;

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
      setMotor(dir, pwr, PWM[k], INP_1[k], INP_2[k]);
    }

    float rightVirtualWheelSpeed = (velocity[0] + velocity[2]) * 0.5;
    float leftVirtualWheelSpeed = (velocity[1] + velocity[3]) * 0.5;

    prevT = currT;
    count_prev = count;

  }

}