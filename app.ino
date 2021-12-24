#include <GDBStub.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "config.h"         // Must be defined after PubSubClient
#include "esp0.h"   // Must be defined after config

ESP0 esp0;

void setup() {
    gdbstub_init();
    delay(2000);    // delay to ensure serial monitor is connected
    // //Serial.begin(115200);

    esp0.setup();
}

void loop() {
    esp0.loop();
}