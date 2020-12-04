#ifndef _Rack2_h_

#define _Rack2_h_

class Rack2
{
private:
    void setup();
    void reader(Zone& zone);
    void printer(String name, Zone zone);

public:
    Rack2();

    // NodeMCU ESP8266 pinout: SCL = 5 (D1) SDA = 4 (D2)
    Zone inlet = {0x76, INLETFANPIN};
    Zone outlet = {0x77, OUTLETFANPIN};

    CircularBuffer<double, 20> inletHistory;
    CircularBuffer<double, 20> outletHistory;

    bool manualFans = false;

    void readSensors();
    void printSensors();
    void printBuff(CircularBuffer<double, 20> & buff);
    void setFans(int speed);
};

#endif