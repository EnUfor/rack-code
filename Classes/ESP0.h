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

    unsigned long MQTT_reconnect_timer;
    int MQTT_initial_delay = 10000;
    int MQTT_delay = MQTT_initial_delay;

    unsigned long MQTT_publish_timer;

    unsigned long PID_timer;

    /*
     * Connects ESP8266 to Wi-Fi
     */
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
    PID pid = {&currentTemp, &pidSpeed, &setTemp, Kp, Ki, Kd, REVERSE};     // Reverse since we're cooling
    
    PubSubClient client = {wificlient};

    int subInletFan;
    int subOutletFan;

    /*
     * Setup Routine (not run on class declaration)
     */
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

        MQTT_reconnect_timer = millis();
        MQTT_publish_timer = millis();
        PID_timer = millis();
    }
    /*
     * Loop Routine
     */
    void loop() {
        // If WiFi is connected, MQTT is not connected, and delay is satisfied: reconnect MQTT
        if (WiFi.status() == WL_CONNECTED && !client.connected() && (millis() - MQTT_reconnect_timer >= MQTT_delay)) {
            MQTT_reconnect_timer = millis();
            connect_MQTT();
            if (!client.connected() && MQTT_delay < 60000)  // IF not successful and delay is less than 60 seconds
            {
                MQTT_delay += 10000;    // Increase delay to avoid unecessary reconnect tries
            } else
            {
                MQTT_delay = MQTT_initial_delay;     // Reset delay
            }
        }

        if (millis() - MQTT_publish_timer >= 10000)
        {
            MQTT_publish_timer = millis();

            client.publish(PUB_INLET_TEMP, String(rack.inlet.temp).c_str());
            client.publish(PUB_INLET_HUMID, String(rack.inlet.humidity).c_str());
            client.publish(PUB_OUTLET_TEMP, String(rack.outlet.temp).c_str());
            client.publish(PUB_OUTLET_HUMID, String(rack.outlet.humidity).c_str());

            if(!rack.manualFans) {
                client.publish(PUB_INLET_FAN, String(rack.inlet.fanSpeed).c_str());
                client.publish(PUB_OUTLET_FAN, String(rack.outlet.fanSpeed).c_str());
            }
        }

        if (millis() - PID_timer >= 500) {
            PID_timer = millis();

            rack.readSensors();
            // rack.printSensors();
            
            if (!rack.manualFans) {
                currentTemp = rack.outlet.temp;
                setTemp = rack.inlet.temp + 5.0;
                pid.Compute();
                rack.setFans((int)pidSpeed);
            }
            
            Serial.println((String)"pidSpeed: " + pidSpeed);
            Serial.println((String)"currentTemp: " + currentTemp);
        }
        
        handleSerial();
        client.loop();
    }

    /*
     * Connects to MQTT broker
     */
    void connect_MQTT() {
        Serial.println((String)"Connecting to MQTT: " + MQTT_SERVER);
        if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
            Serial.println("Connected to MQTT");
        } else {
            Serial.println((String)"Failed to connect to MQTT with state: " + client.state());
        }
        Serial.println('\n');
    }

    /*
     * Handles input from serial
     */
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