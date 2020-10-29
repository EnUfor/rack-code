#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <cactus_io_BME280_I2C.h>
#include <CircularBuffer.h>
#include "config.h"     // Turns out you need to use quotes for relative imports (2 hrs wasted)
#include "fanCurve.h"   // Also turns out the includes' first character has to be lower case (10 min wasted)

unsigned long MQTT_sensor_timer;
unsigned long MQTT_reconnect_timer;
int MQTT_initial_delay = 10000;
unsigned int MQTT_delay = MQTT_initial_delay;

String clientId;

const int fanPin1   = 13; // D7
const int fanPin2   = 15; // D8
const int fanPower  = 12; // D6
const int LEDPin    = 16; // D0 (also pin for 2nd onboard LED). GPIO2 (D4) is other LED

int pwmValue = 1024;    // Initial pwm

// Initiate WiFi & MQTT. Change the client name if you have multiple ESPs
WiFiClient wificlient;
PubSubClient client(wificlient);

// CircularBuffer<double, 20> buff_inletTemp;
// CircularBuffer<double, 20> buff_outletTemp;

class Sensor
{
private:
    
    void setup() {
        sensor.setTempCal(-1);
        if (sensor.begin()) {online = true;}
    }
    
public:
    Sensor();
    // ~Sensor();
    BME280_I2C sensor;
    // CircularBuffer<double, 20> history;
    // CircularBuffer<double, 20> hi;
    double temp;
    double humidity;
    int fanSpeed = 1024;
    bool online = false;

    Sensor(uint8_t address) {
        sensor = BME280_I2C(address);
        setup();
    }

    // void setFanSpeed() {

    // }
};

Sensor::Sensor()
{
}

// Sensor::~Sensor()
// {
// }

class Rack
{
private:

    /**
     * Read sensor and set respective values
     * Must be passed by reference (3 hrs wasted)
    **/    
    void reader(Sensor& zone) {
        if (zone.online) {
            zone.sensor.readSensor();
            zone.temp = zone.sensor.getTemperature_F();
            zone.humidity = zone.sensor.getHumidity();
        }
    }

    /**
     * Print sensor values to serial
    **/ 
    void printer(String name, Sensor sensor) {
        Serial.print(name); Serial.print(":\t");
        if (sensor.online) {
            Serial.print(sensor.temp); Serial.print(" *F\t\t");
            Serial.print(sensor.humidity); Serial.println(" %");
        } else
        {
            Serial.println("Not Connected");
        }
    }
public:
    Rack();
    // ~Rack();
    // NodeMCU ESP8266 pinout: SCL = 5 (D1) SDA = 4 (D2)
    Sensor inlet = Sensor(0x76);
    Sensor outlet = Sensor(0x77);
    // CircularBuffer<double, 20> YUWORKHERE;

    /**
     * Read sensor values
    **/
    void readSensors() {
        reader(inlet);
        reader(outlet);
    }

    /**
     * Print sensor values
    **/
    void printSensors() {
        // printer("Inlet", inlet);
        // printer("Outlet", outlet);
        Serial.println("\n");
    }
};

Rack::Rack()
{
}

// Rack::~Rack()
// {
// }

Rack rack;

void setup() {
    delay(2000);    // delay to ensure serial monitor is connected
    Serial.begin(115200);
    clientId = "ESP-";
    clientId += system_get_chip_id();
    Serial.println((String)"clientId: " + clientId);
    rack.inlet.sensor.begin();
    rack.outlet.sensor.begin();

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

    client.subscribe(SUB_INLET_FAN);
    client.subscribe(SUB_OUTLET_FAN);
    client.subscribe(SUB_MAN_FAN);

    // Startup Routine
    // for (pwmValue = 1024; pwmValue >= 0; pwmValue -= 5) {
    //   setFanSpeed(fanPin1, pwmValue);       // Set fan speed1
    //   setFanSpeed(fanPin2, pwmValue);       // Set fan speed2
    //   Serial.println(pwmValue);
    //   delay(50);
    // }
    MQTT_sensor_timer = millis();
    MQTT_reconnect_timer = millis();
}

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

    rack.readSensors();
    rack.printSensors();

    if (millis() - MQTT_sensor_timer >= 5000)
    {
        MQTT_sensor_timer = millis();

        client.publish(PUB_INLET_TEMP, String(rack.inlet.temp).c_str());
        client.publish(PUB_INLET_HUMID, String(rack.inlet.humidity).c_str());
        client.publish(PUB_OUTLET_TEMP, String(rack.outlet.temp).c_str());
        client.publish(PUB_OUTLET_HUMID, String(rack.outlet.humidity).c_str());
    }
    

    // Print buff_inletTemp
    // for (int i = 0; i < 19; i++)
    // {
    //     Serial.print(buff_inletTemp[i]);
    //     Serial.print(", ");
    // }
    // Serial.println("");

    // temp();
    handleSerial();
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

/**
 * IS THIS SECTION BELOW NECESSARY IF WE'RE GOING TO UTILIZE PID CONTROL??? PROBABLY NOT
**/

// Adapted from Heater.ino
/**
 * Gets the desired fan speed based on a fan curve
 * @return {double} y - Interpolated fan speed (0% - 100%).
 */ 
float getFanSpeed() {
    float temp = 0;

    // This section would utilize a filter or rolling average to get desired
    // fan speed based on a fan speed v. temperature curve speciifed in fanCurve.h

    // 
    // Since our array contains entities of type 'short' (16 bits or 2 bytes each),
    // sizeof(temptable) represents the size of the entire array (256 bytes).
    // sizeof(temptable[0]) represents the size of a single row of our array (4 bytes),
    // sizeof(temptable[0][0]) represents the size of a single entity in the array (2 bytes),
    //
    
    int numRows = sizeof(fanCurve)/sizeof(fanCurve[0]);
    // int numCols = sizeof(temptable[0])/sizeof(temptable[0][0]);
    // Serial.print("Number of rows = ");Serial.println(numRows);
    // Serial.print("Number of cols = ");Serial.println(numCols);

    return linearInterpolate(temp, fanCurve, numRows);
}

// Stolen from Heater.ino
/**
 * Linear interpolate a value using a table for lookup.
 * @param {double} x - X value to interpolate to.
 * @param {short} table - Table containing lookup values.
 * @param {int} numRows - Number of rows in table.
 */
float linearInterpolate(double x, const short table[][2], int numRows) {
    // numRows must be submitted as an argument since table is a pointer
    // therefore sizeof(table) will always be 2, causing errors
    
    float x0, x1, y0, y1, y;

    // TODO: Add code that if a value doesn't match (1023 or 0), proper flags are raised

    for (int i = 0; i < numRows; i++) { // for each row in table
        if (x > table[i][0] && x < table[i + 1][0]) {
            x0 = table[i][0];       // lower bound x
            x1 = table[i + 1][0];   // upper bound x
            y0 = table[i][1];       // lower bound y
            y1 = table[i + 1][1];   // upper bound y
        }
    }

    // Serial.print("x0: ");Serial.print(x0);
    // Serial.print(" y0: ");Serial.println(y0);
    // Serial.print("x1: ");Serial.print(x1);
    // Serial.print(" y1: ");Serial.println(y1);

    return y0 + ((y1 - y0) / (x1 - x0)) * (x - x0);

}