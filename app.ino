#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <PID_v1.h>
#include "config.h"     // Turns out you need to use quotes for relative imports (2 hrs wasted)
// #include "fanCurve.h"   // Also turns out the includes' first character has to be lower case (10 min wasted)
// #include "Classes/rack.h"


#include "Classes/ESP0.h"
// class ESP0
// {
// private:

//     double Kp = 9.0;
//     double Ki = 2.0;
//     double Kd = 0.5;

//     WiFiClient wificlient;

//     String clientId;

//     // Connects ESP8266 to Wi-Fi
//     void connect_wifi() {
//         WiFi.begin(SSID, WIFI_PASSWORD);
//         Serial.println((String)"Connecting to SSID: " + SSID);

//         int i = 0;
//         double continueDelay = millis();
//         while (WiFi.status() != WL_CONNECTED && ((millis() - continueDelay) < 8000)) {
//             delay(1000);
//             Serial.print((String)++i + "...");
//         }
//         Serial.println('\n');

//         if (WiFi.status() == WL_CONNECTED) {
//             Serial.println((String)"Connected to SSID: " + SSID);
//             // Serial.println((String)"IP Address: " + (String)WiFi.localIP());
//             Serial.print("IP Address: "); Serial.print(WiFi.localIP());
//         } else {
//             Serial.println((String)"Failed to connect to WiFi with state: " + WiFi.status());
//         }
//         Serial.println('\n');
//     }

// public:
//     ESP0() {};

//     double setTemp, currentTemp, pidSpeed;
//     PID pid = PID(&currentTemp, &pidSpeed, &setTemp, Kp, Ki, Kd, REVERSE);    // Reverse since we're cooling

//     PubSubClient client = PubSubClient(wificlient);

//     // Setup routine (not run on class declaration)
//     void setup() {
//         pid.SetOutputLimits(0, 100);
//         pid.SetMode(AUTOMATIC);

//         clientId = "ESP-";
//         clientId += system_get_chip_id();
//         Serial.println((String)"clientId: " + clientId);

//         connect_wifi();

//         // Initialize MQTT client
//         client.setServer(MQTT_SERVER, MQTT_PORT);
//         // callback set before setup is run

//         connect_MQTT();

//         client.subscribe(SUB_MAN_FAN);  // Sub first to get state
//         client.subscribe(SUB_INLET_FAN);
//         client.subscribe(SUB_OUTLET_FAN);
//     }

//     void loop() {
//         //Loop here
//         handleSerial();
//         client.loop();
//     }

//     // Connects to MQTT broker
//     void connect_MQTT() {
//         Serial.println((String)"Connecting to MQTT: " + MQTT_SERVER);
//         if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
//             Serial.println("Connected to MQTT");
//         } else {
//             Serial.println((String)"Failed to connect to MQTT with state: " + client.state());
//         }
//         Serial.println('\n');
//     }

//     void handleSerial() {
//     while (Serial.available()) {
//         char inputBuffer[6];
//         Serial.readBytesUntil('\n', inputBuffer, 5);

//         int inputFanSpeed = atoi(inputBuffer);
//         rack.setFans(inputFanSpeed);
//         rack.manualFans = false;
//         client.publish(PUB_MAN_FAN_STATE, "0");       // Let HA know it's no longer in control
//         Serial.print((String)"Current PWM = " + inputFanSpeed);
//     }
//     Serial.flush();
// }
// };

unsigned long MQTT_sensor_timer;
unsigned long MQTT_reconnect_timer;
unsigned long printDelay;

int MQTT_initial_delay = 10000;
unsigned int MQTT_delay = MQTT_initial_delay;

ESP0 espoooo;

void callback(char* topic, byte* payload, unsigned int length) {
    String topicStr = topic;    // Convert char to String
    payload[length] = '\0';     // Null terminate
    int payloadInt = atoi((char*)payload);  // convert payload to int
    
    Serial.println((String)"Topic: " + topicStr);
    Serial.println((String)"Payload: " + payloadInt);

    if (topicStr == SUB_MAN_FAN) {
        rack.manualFans = payloadInt;
        espoooo.client.publish(PUB_MAN_FAN_STATE, String(payloadInt).c_str());
    } else if (topicStr == SUB_INLET_FAN) {
        if (rack.manualFans)
        {
            rack.inlet.setFanSpeed(payloadInt);
        }    
    } else if (topicStr == SUB_OUTLET_FAN) {
        if (rack.manualFans)
        {
            rack.outlet.setFanSpeed(payloadInt);
        }
    }
}

void setup() {
    delay(2000);    // delay to ensure serial monitor is connected
    Serial.begin(115200);

    espoooo.client.setCallback(callback);
    espoooo.setup();

    MQTT_sensor_timer = millis();
    MQTT_reconnect_timer = millis();
    printDelay = millis();
}

void loop() {
    // If WiFi is connected, MQTT is not connected, and delay is satisfied: reconnect MQTT
    if (WiFi.status() == WL_CONNECTED && !espoooo.client.connected() && (millis() - MQTT_reconnect_timer >= MQTT_delay)) {
        MQTT_reconnect_timer = millis();
        espoooo.connect_MQTT();
        if (!espoooo.client.connected() && MQTT_delay < 60000)  // IF not successful and delay is less than 60 seconds
        {
            MQTT_delay += 10000;    // Increase delay to avoid unecessary reconnect tries
        } else
        {
            MQTT_delay = MQTT_initial_delay;     // Reset delay
        }
    }

    if (millis() - printDelay >= 500) {
        printDelay = millis();

        rack.readSensors();
        // rack.printSensors();

        espoooo.currentTemp = rack.outlet.temp;
        espoooo.setTemp = rack.inlet.temp + 5.0;
        if (!rack.manualFans) {
            espoooo.pid.Compute();
            rack.setFans((int)espoooo.pidSpeed);
        }
        
        Serial.println((String)"pidSpeed: " + espoooo.pidSpeed);
        Serial.println((String)"currentTemp: " + espoooo.currentTemp);

    }


    if (millis() - MQTT_sensor_timer >= 10000)
    {
        MQTT_sensor_timer = millis();

        espoooo.client.publish(PUB_INLET_TEMP, String(rack.inlet.temp).c_str());
        espoooo.client.publish(PUB_INLET_HUMID, String(rack.inlet.humidity).c_str());
        espoooo.client.publish(PUB_OUTLET_TEMP, String(rack.outlet.temp).c_str());
        espoooo.client.publish(PUB_OUTLET_HUMID, String(rack.outlet.humidity).c_str());

        if(!rack.manualFans) {
            espoooo.client.publish(PUB_INLET_FAN, String(rack.inlet.fanSpeed).c_str());
            espoooo.client.publish(PUB_OUTLET_FAN, String(rack.outlet.fanSpeed).c_str());
        }
    }

    espoooo.loop();
}