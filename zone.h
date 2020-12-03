#ifndef _Zone_h_

#define _Zone_h_

class Zone
{
private:
    void setupSensor(uint8_t address);
    void setupPin(int pin);

public:
    Zone(uint8_t address, int pin);

    double temp;
    double humidity;
    int fanSpeed;
    bool online = false;
    int pwmPin;

    BME280_I2C sensor;

    void setFanSpeed(int speed);
};

#endif