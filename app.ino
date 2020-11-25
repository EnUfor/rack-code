#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "config.h"         // Must be defined after PubSubClient
#include "Classes/ESP0.h"   // Must be defined after config

unsigned long printDelay;

ESP0 esp0;

void callback(char* topic, uint8_t* payload, unsigned int length) {
    String topicStr = topic;    // Convert char to String
    payload[length] = '\0';     // Null terminate
    int payloadInt = atoi((char*)payload);  // convert payload to int
    
    Serial.println((String)"Topic: " + topicStr);
    Serial.println((String)"Payload: " + payloadInt);

    if (topicStr == SUB_MAN_FAN) {
        rack.manualFans = payloadInt;
        esp0.client.publish(PUB_MAN_FAN_STATE, String(payloadInt).c_str());
        
        // Set fans to what was last recieved from MQTT
        rack.inlet.setFanSpeed(esp0.subInletFan);
        rack.outlet.setFanSpeed(esp0.subOutletFan);
    } else if (topicStr == SUB_INLET_FAN) {
        esp0.subInletFan = payloadInt;
        if (rack.manualFans)
        {
            rack.inlet.setFanSpeed(payloadInt);
        }    
    } else if (topicStr == SUB_OUTLET_FAN) {
        esp0.subOutletFan = payloadInt;
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

    printDelay = millis();
}

void loop() {
    if (millis() - printDelay >= 500) {
        printDelay = millis();

        rack.readSensors();
        // rack.printSensors();
        
        if (!rack.manualFans) {
            esp0.currentTemp = rack.outlet.temp;
            esp0.setTemp = rack.inlet.temp + 5.0;
            esp0.pid.Compute();
            rack.setFans((int)esp0.pidSpeed);
        }
        
        Serial.println((String)"pidSpeed: " + esp0.pidSpeed);
        Serial.println((String)"currentTemp: " + esp0.currentTemp);

    }

    esp0.loop();
}