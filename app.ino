#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "config.h"         // Must be defined after PubSubClient
#include "Classes/ESP0.h"   // Must be defined after config

unsigned long MQTT_sensor_timer;
unsigned long MQTT_reconnect_timer;
unsigned long printDelay;

int MQTT_initial_delay = 10000;
unsigned int MQTT_delay = MQTT_initial_delay;

ESP0 esp0;

void callback(char* topic, byte* payload, unsigned int length) {
    String topicStr = topic;    // Convert char to String
    payload[length] = '\0';     // Null terminate
    int payloadInt = atoi((char*)payload);  // convert payload to int
    
    Serial.println((String)"Topic: " + topicStr);
    Serial.println((String)"Payload: " + payloadInt);

    if (topicStr == SUB_MAN_FAN) {
        rack.manualFans = payloadInt;
        esp0.client.publish(PUB_MAN_FAN_STATE, String(payloadInt).c_str());
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

    esp0.client.setCallback(callback);
    esp0.setup();

    MQTT_sensor_timer = millis();
    MQTT_reconnect_timer = millis();
    printDelay = millis();
}

void loop() {
    // If WiFi is connected, MQTT is not connected, and delay is satisfied: reconnect MQTT
    if (WiFi.status() == WL_CONNECTED && !esp0.client.connected() && (millis() - MQTT_reconnect_timer >= MQTT_delay)) {
        MQTT_reconnect_timer = millis();
        esp0.connect_MQTT();
        if (!esp0.client.connected() && MQTT_delay < 60000)  // IF not successful and delay is less than 60 seconds
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

        esp0.currentTemp = rack.outlet.temp;
        esp0.setTemp = rack.inlet.temp + 5.0;
        if (!rack.manualFans) {
            esp0.pid.Compute();
            rack.setFans((int)esp0.pidSpeed);
        }
        
        Serial.println((String)"pidSpeed: " + esp0.pidSpeed);
        Serial.println((String)"currentTemp: " + esp0.currentTemp);

    }


    if (millis() - MQTT_sensor_timer >= 10000)
    {
        MQTT_sensor_timer = millis();

        esp0.client.publish(PUB_INLET_TEMP, String(rack.inlet.temp).c_str());
        esp0.client.publish(PUB_INLET_HUMID, String(rack.inlet.humidity).c_str());
        esp0.client.publish(PUB_OUTLET_TEMP, String(rack.outlet.temp).c_str());
        esp0.client.publish(PUB_OUTLET_HUMID, String(rack.outlet.humidity).c_str());

        if(!rack.manualFans) {
            esp0.client.publish(PUB_INLET_FAN, String(rack.inlet.fanSpeed).c_str());
            esp0.client.publish(PUB_OUTLET_FAN, String(rack.outlet.fanSpeed).c_str());
        }
    }

    esp0.loop();
}