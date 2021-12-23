#include "esp0.h"
#include "rack.h"

Rack rack;
// Constructors
ESP0::ESP0() {}

//Private

/*
* Connects ESP8266 to Wi-Fi
*/
void ESP0::connect_wifi()
{
    WiFi.begin(SSID, WIFI_PASSWORD);
    Serial.println((String) "Connecting to SSID: " + SSID);

    int i = 0;
    double continueDelay = millis();
    while (WiFi.status() != WL_CONNECTED && ((millis() - continueDelay) < 8000))
    {
        delay(1000);
        Serial.print((String)++i + "...");
    }
    Serial.println('\n');

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println((String) "Connected to SSID: " + SSID);
        // Serial.println((String)"IP Address: " + (String)WiFi.localIP());
        Serial.print("IP Address: ");
        Serial.print(WiFi.localIP());
    }
    else
    {
        Serial.println((String) "Failed to connect to WiFi with state: " + WiFi.status());
    }
    Serial.println('\n');
}

void ESP0::callback(char *topic, uint8_t *payload, unsigned int length)
{
    String topicStr = topic;                // Convert char to String
    payload[length] = '\0';                 // Null terminate
    int payloadInt = atoi((char *)payload); // convert payload to int

    Serial.println((String) "Topic: " + topicStr);
    Serial.println((String) "Payload: " + payloadInt);

    if (topicStr == SUB_MAN_FAN)
    {
        rack.manualFans = payloadInt;
        MQTTClient.publish(PUB_MAN_FAN_STATE, String(payloadInt).c_str());

        // Set fans to what was last recieved from MQTT
        rack.inlet.setFanSpeed(subInletFan);
        rack.outlet.setFanSpeed(subOutletFan);
    }
    else if (topicStr == SUB_INLET_FAN)
    {
        subInletFan = payloadInt;
        if (rack.manualFans)
        {
            rack.inlet.setFanSpeed(payloadInt);
        }
    }
    else if (topicStr == SUB_OUTLET_FAN)
    {
        subOutletFan = payloadInt;
        if (rack.manualFans)
        {
            rack.outlet.setFanSpeed(payloadInt);
        }
    }
}

//Public

/*
* Setup Routine (not run on class declaration)
*/
void ESP0::setup()
{
    pid.SetOutputLimits(1, 100);
    pid.SetMode(AUTOMATIC);

    clientId = "ESP-";
    clientId += system_get_chip_id();
    Serial.println((String) "clientId: " + clientId);

    connect_wifi();

    // No Clue: https://hobbytronics.com.pk/arduino-custom-library-and-pubsubclient-call-back/
    MQTTClient.setCallback([this](char *topic, byte *payload, unsigned int length)
                           { this->callback(topic, payload, length); });

    // Initialize MQTT client
    MQTTClient.setServer(MQTT_SERVER, MQTT_PORT);
    // callback set before setup is run

    connect_MQTT();

    MQTTClient.subscribe(SUB_MAN_FAN); // Sub first to get state
    MQTTClient.subscribe(SUB_INLET_FAN);
    MQTTClient.subscribe(SUB_OUTLET_FAN);

    MQTT_reconnect_timer = millis();
    MQTT_publish_timer = millis();
    PID_timer = millis();
}

/*
* Loop Routine
*/
void ESP0::loop()
{
    // If WiFi is connected, MQTT is not connected, and delay is satisfied: reconnect MQTT
    if (WiFi.status() == WL_CONNECTED && !MQTTClient.connected() && (millis() - MQTT_reconnect_timer >= MQTT_delay))
    {
        MQTT_reconnect_timer = millis();
        connect_MQTT();
        if (!MQTTClient.connected() && MQTT_delay < 60000) // IF not successful and delay is less than 60 seconds
        {
            MQTT_delay += 10000; // Increase delay to avoid unecessary reconnect tries
        }
        else
        {
            MQTT_delay = MQTT_initial_delay; // Reset delay
        }
    }

    if (millis() - MQTT_publish_timer >= 10000)
    {
        MQTT_publish_timer = millis();

        MQTTClient.publish(PUB_INLET_TEMP, String(rack.inlet.temp).c_str());
        MQTTClient.publish(PUB_INLET_HUMID, String(rack.inlet.humidity).c_str());
        MQTTClient.publish(PUB_INLET_FAN, String(rack.inlet.fanSpeed).c_str());

        MQTTClient.publish(PUB_OUTLET_TEMP, String(rack.outlet.temp).c_str());
        MQTTClient.publish(PUB_OUTLET_HUMID, String(rack.outlet.humidity).c_str());
        MQTTClient.publish(PUB_OUTLET_FAN, String(rack.outlet.fanSpeed).c_str());
    }

    if (millis() - PID_timer >= 500)
    {
        PID_timer = millis();

        rack.readSensors();
        // rack.printSensors();

        if (!rack.manualFans)
        {
            currentTemp = rack.outlet.temp;
            setTemp = rack.inlet.temp + 5.0;
            pid.Compute();
            rack.setFans((int)pidSpeed);

            Serial.print((String) "pidSpeed: " + pidSpeed + "\t");
            Serial.print((String) "currentTemp: " + currentTemp + "\t");
            Serial.print((String) "setTemp: " + setTemp + "\n");
        }
    }

    handleSerial();
    MQTTClient.loop();
}

/*
* Connects to MQTT broker
*/
void ESP0::connect_MQTT()
{
    Serial.println((String) "Connecting to MQTT: " + MQTT_SERVER);
    if (MQTTClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD))
    {
        Serial.println("Connected to MQTT");
    }
    else
    {
        Serial.println((String) "Failed to connect to MQTT with state: " + MQTTClient.state());
    }
    Serial.println('\n');
}

/*
* Handles input from serial
*/
void ESP0::handleSerial()
{
    while (Serial.available())
    {
        char inputBuffer[6];
        Serial.readBytesUntil('\n', inputBuffer, 5);
        int inputFanSpeed = atoi(inputBuffer);

        rack.setFans(inputFanSpeed);
        rack.manualFans = true;                     // Prevent PID calcs, still publish speeds
        MQTTClient.publish(PUB_MAN_FAN_STATE, "0"); // Let HA know it's no longer in control

        Serial.println((String) "Current PWM = " + inputFanSpeed);
    }
    Serial.flush();
}