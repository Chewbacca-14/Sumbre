#include "Sensor.h"
#include <Arduino.h>

Sensor::Sensor() : dht(DHTPIN, DHTTYPE) {}

void Sensor::begin()
{
    dht.begin();
}

void Sensor::read(float &temperature, float &humidity)
{
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h))
    {
        Serial.println("Failed to read from DHT sensor!");
        temperature = -1;
        humidity = -1;
    }
    else
    {
        temperature = t;
        humidity = h;
    }
}
