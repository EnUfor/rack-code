#include <PID_v1.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "config.h"

#ifndef _Esp0_h_
#define _Esp0_h_

class ESP0
{
private:
    double Kp = 9.0;
    double Ki = 2.0;
    double Kd = 0.5;

    WiFiClient wificlient;

    String clientId;

    unsigned long MQTT_reconnect_timer;
    int MQTT_initial_delay = 10000;
    int MQTT_delay = MQTT_initial_delay;

    unsigned long MQTT_publish_timer;

    unsigned long PID_timer;

    void connect_wifi();

    void callback(char *topic, uint8_t *payload, unsigned int length);

public:
    ESP0();

    double setTemp, currentTemp, pidSpeed;
    PID pid = {&currentTemp, &pidSpeed, &setTemp, Kp, Ki, Kd, REVERSE}; // Reverse since we're cooling

    PubSubClient MQTTClient = {wificlient};

    int subInletFan;
    int subOutletFan;

    void setup();
    void loop();
    void connect_MQTT();
    void handleSerial();
};

#endif