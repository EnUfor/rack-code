#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <cactus_io_BME280_I2C.h>
#include "config.h" // Turns out you need to use quotes for relative imports (2 hrs wasted)

// Create two BME280 instances
// NodeMCU ESP8266 pinout: SCL = 5 (D1) SDA = 4 (D2)
BME280_I2C bme1(0x76); // I2C using address 0x76
BME280_I2C bme2(0x77); // I2C using address 0x77

String clientId;

const double tempOffsetF = -1.8;
const double tempOffsetC = -1.0;

bool BME1 = 1;
bool BME2 = 1;

const int fanPin1   = 13; // D7
const int fanPin2   = 15; // D8
const int fanPower  = 12; // D6
const int LEDPin    = 16; // D0 (also pin for 2nd onboard LED). GPIO2 (D4) is other LED

int pwmValue = 1024;    // Initial pwm

// Initiate WiFi & MQTT. Change the client name if you have multiple ESPs
WiFiClient wificlient;
PubSubClient client(wificlient);



void setup() {
  // Initiate random seed without using analog input
  randomSeed(micros());
  // Create a random client ID
  clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);

  delay(2000);    // delay to ensure serial monitor is connected

  Serial.begin(115200);
  Serial.println((String)"clientId: " + clientId);
  Serial.println((String)"system chip id" + system_get_chip_id());

  if (!bme1.begin()) {
    Serial.println("Could not find 1st (0x76) BME280 sensor, check wiring!");
    BME1 = 0;
    // while (1);
  }

  if (!bme2.begin()) {
    Serial.println("Could not find 2nd (0x77) BME280 sensor, check wiring!");
    BME2 = 0;
    // while (1);
  }
  // Serial.println((String)"BME1 = " + BME1 + " BME2 = " + BME2);

  pinMode(fanPin1,  OUTPUT);
  pinMode(fanPin2,  OUTPUT);
  pinMode(fanPower, OUTPUT);
  pinMode(LEDPin,   OUTPUT);
  digitalWrite(fanPower, HIGH);
  analogWrite(fanPin1, pwmValue);   // Initial speed
  
  connect_wifi();

  // Initialize MQTT client
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);       // Run callback function for when MQTT message received

  connect_MQTT();

  // client.publish("esp/test", "Hello from ESP8266");
  client.subscribe("rack/inlet/fan_speed");
  client.subscribe("rack/outlet/fan_speed");
  client.subscribe("rack/manual_fan");

  // Startup Routine
  // for (pwmValue = 1024; pwmValue >= 0; pwmValue -= 5) {
  //   setFanSpeed(fanPin1, pwmValue);       // Set fan speed1
  //   setFanSpeed(fanPin2, pwmValue);       // Set fan speed2
  //   Serial.println(pwmValue);
  //   delay(50);
  // }
}
int tracker = 10;
void loop() {
  // If WiFi connected and MQTT not connected, reconnect MQTT (necessary)
  if (WiFi.status() == WL_CONNECTED && !client.connected() && tracker >= 10) {
    connect_MQTT();
    tracker = 0;
  }
  Serial.println((String)"tracker: " + tracker);
  tracker++;
  
  // temp();
  handleSerial();
  String message = String(random(0xfdff), HEX);
  client.publish("esp/test", message.c_str());

  // setFanSpeed(fanPin1, pwmValue);       // Set fan speed1
  // setFanSpeed(fanPin2, pwmValue);       // Set fan speed2
  delay(500);

  client.loop();
}

void handleSerial() {         // Must imput single digit numbers with leading 0
  boolean printValue = false;
  while (Serial.available()) {
    char inputBuffer[6];
    Serial.readBytesUntil('\n', inputBuffer, 5);

    pwmValue = atoi(inputBuffer);
    printValue = true;
  }

  if (printValue) {
    Serial.print("Current PWM = ");
    Serial.println(pwmValue);
    Serial.flush();
  }
}

void setFanSpeed(int fanPin, int fanSpeed) {
  analogWrite(fanPin, fanSpeed);

  //Serial.print("PWM Value Set: ");
  //Serial.println(fanSpeed);

  if (fanSpeed <= 0) {            // Turn off fans if either speed 0 (or less than)
    digitalWrite(fanPower, LOW);
    digitalWrite(LEDPin, LOW);   // Turn on LED
  } else {                        // Turns fans on elsewise
    digitalWrite(fanPower, HIGH);
    digitalWrite(LEDPin, HIGH);    // Turn off LED
  }
}

void temp() {
  // Only read sensor if it was initiated
  if (BME1 == 1) {
    bme1.readSensor();
    Serial.print("BME 1\t");
    Serial.print(bme1.getHumidity()); Serial.print(" %\t\t");
    // Serial.print(bme1.getTemperature_C() - tempOffsetC); Serial.print(" *C\t");
    Serial.print(bme1.getTemperature_F() + tempOffsetF); Serial.print(" *F\t");
    Serial.println();
  } else {
    Serial.print("BME 1\t");
    Serial.println("Not Connected");
  }

  // Only read sensor if it was initiated
  if (BME2 == 1) {
    bme2.readSensor();
    Serial.print("BME 2\t");
    Serial.print(bme2.getHumidity()); Serial.print(" %\t\t");
    // Serial.print(bme2.getTemperature_C() + tempOffsetC); Serial.print(" *C\t");
    Serial.print(bme2.getTemperature_F() + tempOffsetF); Serial.print(" *F\t");
  } else {
    Serial.print("BME 2\t");
    Serial.println("Not Connected");
  }

  Serial.println(); Serial.println();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println();
  Serial.println("-----------------------");
}

// Connects ESP8266 to Wi-Fi
void connect_wifi() {
  WiFi.begin(SSID, WIFI_PASSWORD);
  Serial.println((String)"Connecting to SSID: " + SSID);

  int i = 0;
  double continueDelay = millis();
  while (WiFi.status() != WL_CONNECTED && ((millis() - continueDelay) < 8000)) {
    delay(1000);
    Serial.print((String)++i + "...");
  }
  Serial.println('\n');

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println((String)"Connected to SSID: " + SSID);
    // Serial.println((String)"IP Address: " + (String)WiFi.localIP());
    Serial.print("IP Address: "); Serial.print(WiFi.localIP());
  } else {
    Serial.println((String)"Failed to connect to WiFi with state: " + WiFi.status());
  }
  Serial.println('\n');
}

// Connects to MQTT broker
void connect_MQTT() {
  Serial.println((String)"Connecting to MQTT: " + MQTT_SERVER);
  if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("Connected to MQTT");
  } else {
    Serial.println((String)"Failed to connect to MQTT with state: " + client.state());
  }
  Serial.println('\n');
}
