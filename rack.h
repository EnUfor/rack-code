#include <cactus_io_BME280_I2C.h>
#include <CircularBuffer.h>
#include "pins.h"
#include "zone.h"

#ifndef _Rack_h_
#define _Rack_h_

class Rack
{
private:
    void setup();
    void reader(Zone &zone);
    void printer(String name, Zone zone);

public:
    Rack();

    // NodeMCU ESP8266 pinout: SCL = 5 (D1) SDA = 4 (D2)
    Zone inlet = {0x76, INLETFANPIN};
    Zone outlet = {0x77, OUTLETFANPIN};

    CircularBuffer<double, 20> inletHistory;
    CircularBuffer<double, 20> outletHistory;

    bool manualFans = false;

    void readSensors();
    void printSensors();
    void printBuff(CircularBuffer<double, 20> &buff);
    void setFans(int speed);
};

#endif