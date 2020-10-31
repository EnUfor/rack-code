#include <cactus_io_BME280_I2C.h>
#include <CircularBuffer.h>

class Zone
{
private:
    
    void setup() {
        sensor.setTempCal(-1);
        if (sensor.begin()) {online = true;}
    }
    
public:
    Zone();
    BME280_I2C sensor;
    // CircularBuffer<double, 20> *history;    // 3? hours wasted for a single * (which didn't make anything better)
    // I hate CircularBuffer(s) now
    double temp;
    double humidity;
    int fanSpeed = 1024;
    bool online = false;

    Zone(uint8_t address) {
        sensor = BME280_I2C(address);
        setup();
    }

    // void setFanSpeed() {
    // }
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
public:
    Rack();
    // NodeMCU ESP8266 pinout: SCL = 5 (D1) SDA = 4 (D2)
    Zone inlet = Zone(0x76);
    Zone outlet = Zone(0x77);
    CircularBuffer<double, 20> inletHistory;
    CircularBuffer<double, 20> outletHistory;

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

    void printBuff() {
        for (int i = inletHistory.size() - 1; i >= 0; i--)
        {
            Serial.print(inletHistory.operator[](i));
            Serial.print(", ");
        }
        Serial.println("\n");
    }
};

Rack::Rack()
{
}

void thingy() {
    Rack testing;
    Zone zont;

    // zont.history[1];

    // CircularBuffer<double, 20> *distory;

    // distory->size()
}