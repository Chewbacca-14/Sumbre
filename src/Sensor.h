#ifndef SENSOR_H
#define SENSOR_H

#include <DHT.h>

// Pin and sensor type
#define DHTPIN D4
#define DHTTYPE DHT11

class Sensor
{
public:
    Sensor();
    void begin();
    void read(float &temperature, float &humidity);

private:
    DHT dht;
};

#endif
