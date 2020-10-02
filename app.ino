#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <cactus_io_BME280_I2C.h>
#include "config.h"

// Create two BME280 instances
// NodeMCU ESP8266 pinout: SCL = 5 (D1) SDA = 4 (D2)
BME280_I2C bme1(0x76); // I2C using address 0x76
BME280_I2C bme2(0x77); // I2C using address 0x77

const double tempOffsetF = -1.8;
const double tempOffsetC = -1.0;

bool BME1 = 1;
bool BME2 = 1;

const int fanPin1   = 13; // D7
const int fanPin2   = 15; // D8
const int fanPower  = 12; // D6
const int LEDPin    = 16; // D0 (also pin for 2nd onboard LED). GPIO2 (D4) is other LED

int pwmValue = 1024;    // Initial pwm

const char* ssid = SSID;                  // WiFi SSID
const char* password = WIFI_PASSWORD;     // WiFi password
const char* mqttServer = MQTT_SERVER;     // MQTT broker
const int mqttPort = MQTT_PORT;           // MQTT broker port    
const char* mqttUser = MQTT_USER;         // MQTT username
const char* mqttPassword = MQTT_PASSWORD; // MQTT password

// Initiate WiFi / MQTT. Change the client name if you have multiple ESPs
WiFiClient wificlient;
PubSubClient client(wificlient);

void setup() {
  delay(5000);
  Serial.begin(115200);

  if (!bme1.begin()) {
    Serial.println("Could not find a 1st (0x76) BME280 sensor, check wiring!");
    BME1 = 0;
    // while (1);
  }

  if (!bme2.begin()) {
    Serial.println("Could not find a 2nd (0x77) BME280 sensor, check wiring!");
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

  // Connect to WiFI
  WiFi.begin(ssid, password);

  Serial.println((String)"Connecting to SSID: " + ssid);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print((String)++i + "...");
  }
  Serial.println('\n');

  Serial.println((String)"Connected to SSID: " + ssid);
  Serial.print("IP Address: "); Serial.print(WiFi.localIP());
  Serial.println('\n');
  
  // Connect to MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);       // Run callback function for when MQTT message received

  // MODIFY to still control fans based on temperatures but attempt connection
  while (!client.connected()) {
    Serial.println((String)"Connecting to MQTT: " + mqttServer);
    if (client.connect("ESP8266Client", mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT");
    } else {
      Serial.println((String)"Failed to connect to MQTT with state: " + client.state());
      delay(2000);
    }
  }

  client.publish("esp/test", "Hello from ESP8266");
  client.subscribe("esp/test");

  // Startup Routine
  for (pwmValue = 1024; pwmValue >= 0; pwmValue -= 5) {
    setFanSpeed(fanPin1, pwmValue);       // Set fan speed1
    setFanSpeed(fanPin2, pwmValue);       // Set fan speed2
    Serial.println(pwmValue);
    delay(50);
  }
}

void loop() {
  temp();
  handleSerial();

  setFanSpeed(fanPin1, pwmValue);       // Set fan speed1
  setFanSpeed(fanPin2, pwmValue);       // Set fan speed2
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
  
  //https://community.home-assistant.io/t/arduino-relay-switches-over-wifi-controlled-via-hass-mqtt-comes-with-ota-support/13439
}
