#ifndef WATER_LEVEL_SENSOR_H
#define WATER_LEVEL_SENSOR_H

#include "sensors/SensorBase.h"
#include "Config.h"

class WaterLevelSensor : public SensorBase {
public:
    WaterLevelSensor(uint8_t pin);
    ~WaterLevelSensor();
    
    // Implémentation des méthodes de SensorBase
    bool begin() override;
    void update() override;
    const char* getName() override;
    void getData(float* data, uint8_t& count) override;
    
    // Méthodes spécifiques au capteur de niveau d'eau
    float getWaterLevel();
    uint8_t getCategory();  // 0=normal, 1=warning, 2=critical
    
private:
    uint8_t _pin;
    float _waterLevel = 0;
    uint8_t _category = 0;
    uint16_t _rawValue = 0;
    
    // Paramètres de conversion
    static const int MAX_RAW_VALUE = 4095;  // Valeur max de l'ADC (12 bits pour ESP32)
    static const int MAX_WATER_LEVEL = 100; // Niveau d'eau maximum en cm
    
    void _calculateCategory();
};

#endif // WATER_LEVEL_SENSOR_H