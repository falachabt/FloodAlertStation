#ifndef DHT11_SENSOR_H
#define DHT11_SENSOR_H

#include "sensors/SensorBase.h"
#include "Config.h"

// Forward declaration pour éviter d'inclure DHT.h dans le header
class DHT;

class DHT11Sensor : public SensorBase {
public:
    DHT11Sensor(uint8_t pin);
    ~DHT11Sensor();
    
    // Implémentation des méthodes de SensorBase
    bool begin() override;
    void update() override;
    const char* getName() override;
    void getData(float* data, uint8_t& count) override;
    
    // Méthodes spécifiques à DHT11
    float getTemperature();
    float getHumidity();
    uint8_t getTemperatureCategory();  // 0=normal, 1=warning, 2=critical
    
private:
    DHT* _dht = nullptr;
    uint8_t _pin;
    float _temperature = 0;
    float _humidity = 0;
    uint8_t _tempCategory = 0;
    
    void _calculateCategory();
};

#endif // DHT11_SENSOR_H