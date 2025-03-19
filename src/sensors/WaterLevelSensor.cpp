#include "sensors/WaterLevelSensor.h"
#include <Arduino.h>

WaterLevelSensor::WaterLevelSensor(uint8_t pin) : _pin(pin) {
}

WaterLevelSensor::~WaterLevelSensor() {
    // Rien à libérer
}

bool WaterLevelSensor::begin() {
    // Configurer le pin comme entrée
    pinMode(_pin, INPUT);
    _isActive = true;
    Serial.print("Water level sensor initialized on pin ");
    Serial.println(_pin);
    return true;
}

void WaterLevelSensor::update() {
    // Lire la valeur analogique
    _rawValue = analogRead(_pin);
    
    // Convertir en niveau d'eau (en cm)
    // Supposons une relation linéaire entre la lecture ADC et le niveau d'eau
    _waterLevel = (float)_rawValue / MAX_RAW_VALUE * MAX_WATER_LEVEL;
    
    // Mettre à jour la catégorie
    _calculateCategory();
    
    _lastReadTime = millis();
}

const char* WaterLevelSensor::getName() {
    return "WaterLevel";
}

void WaterLevelSensor::getData(float* data, uint8_t& count) {
    // Fournir les données dans un format prêt à être envoyé
    if (count >= 3) {
        data[0] = _waterLevel;    // Niveau d'eau en cm
        data[1] = 0;              // Pas de température pour ce capteur
        data[2] = _category;      // Catégorie d'alerte
        count = 3;
    } else if (count >= 1) {
        data[0] = _waterLevel;
        count = 1;
    }
}

float WaterLevelSensor::getWaterLevel() {
    return _waterLevel;
}

uint8_t WaterLevelSensor::getCategory() {
    return _category;
}

void WaterLevelSensor::_calculateCategory() {
    if (_waterLevel >= WATER_CRITICAL_THRESHOLD) {
        _category = 2; // Critique
    } else if (_waterLevel >= WATER_WARNING_THRESHOLD) {
        _category = 1; // Avertissement
    } else {
        _category = 0; // Normal
    }
}