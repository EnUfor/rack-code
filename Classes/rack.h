#include <cactus_io_BME280_I2C.h>
#include <CircularBuffer.h>
#include "../pins.h"

class Zone
{
private:
    
    void setupSensor(uint8_t address) {
        sensor = BME280_I2C(address);
        sensor.setTempCal(-1);
        if (sensor.begin()) {online = true;}
    }
    
    void setupPin(int pin) {
        pwmPin = pin;
        pinMode(pin, OUTPUT);
    }
    
public:
    Zone();
    BME280_I2C sensor;
    // CircularBuffer<double, 20> *history;    // 3? hours wasted for a single * (which didn't make anything better)
    // I hate CircularBuffer(s) now
    double temp;
    double humidity;
    int fanSpeed;
    bool online = false;
    int pwmPin;

    // Zone(uint8_t address) {
    //     setupSensor(address);
    // }

    Zone(uint8_t address, int pin) {
        setupSensor(address);
        setupPin(pin);
    }

    void setFanSpeed(int speed) {
        fanSpeed = speed;
        speed = map(speed, 0, 100, 0, 1024);
        analogWrite(pwmPin, speed);

        if (speed <= 0) {            // Turn off fans if either speed 0
            digitalWrite(FANPOWER, LOW);
            digitalWrite(LEDPIN, LOW);   // Turn on LED
        } else {                        // Turns fans on elsewise
            digitalWrite(FANPOWER, HIGH);
            digitalWrite(LEDPIN, HIGH);    // Turn off LED
        }
    }
};

Zone::Zone()
{
}

class Rack
{
private:
    /**
     * Read sensor and set respective values
     * Must be passed by reference (3 hrs wasted)
    **/    
    void reader(Zone& zone) {
        if (zone.online) {
            zone.sensor.readSensor();
            zone.temp = zone.sensor.getTemperature_F();
            zone.humidity = zone.sensor.getHumidity();
            // zone.history->push(zone.temp);
        }
    }

    /**
     * Print sensor values to serial
    **/ 
    void printer(String name, Zone zone) {
        Serial.print(name); Serial.print(":\t");
        if (zone.online) {
            Serial.print(zone.temp); Serial.print(" *F\t\t");
            Serial.print(zone.humidity); Serial.println(" %");
        } else
        {
            Serial.println("Not Connected");
        }
    }

    void setup() {
        pinMode(FANPOWER, OUTPUT);
        pinMode(LEDPIN, OUTPUT);
        setFans(100);
    }
public:
    Rack();
    // NodeMCU ESP8266 pinout: SCL = 5 (D1) SDA = 4 (D2)
    Zone inlet = Zone(0x76, INLETFANPIN);
    Zone outlet = Zone(0x77, OUTLETFANPIN);
    CircularBuffer<double, 20> inletHistory;
    CircularBuffer<double, 20> outletHistory;
    bool manualFans = false;

    /**
     * Read sensor values
    **/
    void readSensors() {
        reader(inlet);
        reader(outlet);
        inletHistory.push(inlet.temp);
        outletHistory.push(outlet.temp);
    }

    /**
     * Print sensor values
    **/
    void printSensors() {
        printer("Inlet", inlet);
        printer("Outlet", outlet);
        Serial.println("\n");
    }

    /**
     * Print Buffer Values
    **/
    void printBuff(CircularBuffer<double, 20> & buff) {
        for (int i = buff.size() - 1; i >= 0; i--)
        {
            Serial.print(buff.operator[](i));
            if (i > 0)
            {
                Serial.print(", ");
            }    
        }
        Serial.print("\n");
    }

    void setFans(int speed) {
        // Serial.println((String)"setFans Ran: " + speed);
        inlet.setFanSpeed(speed);
        outlet.setFanSpeed(speed);
    }
};

Rack::Rack()
{
    setup();
}

void thingy() {
    // Rack testing;
    // Zone zont;

    // zont.history[1];

    // CircularBuffer<double, 20> hellos;
    // hellos.operator[](2);

    

}