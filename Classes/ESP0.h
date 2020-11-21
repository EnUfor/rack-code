#include <PID_v1.h>
#include "rack.h"

Rack rack;

class ESP0
{
private:

    double Kp = 9.0;
    double Ki = 2.0;
    double Kd = 0.5;

    WiFiClient wificlient;

    String clientId;

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

public:
    ESP0() {};

    double setTemp, currentTemp, pidSpeed;
    PID pid = PID(&currentTemp, &pidSpeed, &setTemp, Kp, Ki, Kd, REVERSE);    // Reverse since we're cooling

    PubSubClient client = PubSubClient(wificlient);

    int subInletFan;
    int subOutletFan;

    // Setup routine (not run on class declaration)
    void setup() {
        pid.SetOutputLimits(1, 100);
        pid.SetMode(AUTOMATIC);

        clientId = "ESP-";
        clientId += system_get_chip_id();
        Serial.println((String)"clientId: " + clientId);

        connect_wifi();

        // Initialize MQTT client
        client.setServer(MQTT_SERVER, MQTT_PORT);
        // callback set before setup is run

        connect_MQTT();

        client.subscribe(SUB_MAN_FAN);  // Sub first to get state
        client.subscribe(SUB_INLET_FAN);
        client.subscribe(SUB_OUTLET_FAN);
    }

    void loop() {
        handleSerial();
        client.loop();
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

    void handleSerial() {
    while (Serial.available()) {
        char inputBuffer[6];
        Serial.readBytesUntil('\n', inputBuffer, 5);

        int inputFanSpeed = atoi(inputBuffer);
        rack.setFans(inputFanSpeed);
        rack.manualFans = false;
        client.publish(PUB_MAN_FAN_STATE, "0");       // Let HA know it's no longer in control
        Serial.print((String)"Current PWM = " + inputFanSpeed);
    }
    Serial.flush();
    }
};