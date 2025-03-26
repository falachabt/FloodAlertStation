#include "sensors/DHT11Sensor.h"
#include <DHT.h>
#include "utils/Logger.h"

DHT11Sensor::DHT11Sensor(uint8_t pin) : _pin(pin) {
    _dht = new DHT(pin, DHT11);
}

DHT11Sensor::~DHT11Sensor() {
    if (_dht) {
        delete _dht;
    }
}

bool DHT11Sensor::begin() {
    _dht->begin();
    _isActive = true;
    Logger::infoF("Capteur DHT11 initialisé sur le pin %d", _pin);
    return true;
}

void DHT11Sensor::update() {
    _temperature = _dht->readTemperature();
    _humidity = _dht->readHumidity();
    
    // Vérifier si la lecture a échoué
    if (isnan(_temperature) || isnan(_humidity)) {
        Logger::error("Échec de lecture du capteur DHT!");
        return;
    }
    
    _lastReadTime = millis();
    _calculateCategory();

    Logger::debugF("DHT11: Température=%.1f°C, Humidité=%.1f%%", _temperature, _humidity);
}

const char* DHT11Sensor::getName() {
    return "DHT11";
}

void DHT11Sensor::getData(float* data, uint8_t& count) {
    // Fournir les données dans un format prêt à être envoyé
    if (count >= 3) {
        data[0] = 0;              // Pas de niveau d'eau pour ce capteur
        data[1] = _temperature;   // Température en °C
        data[2] = _tempCategory;  // Catégorie d'alerte
        count = 3;
    } else if (count >= 1) {
        data[0] = _temperature;
        count = 1;
    }
}

float DHT11Sensor::getTemperature() {
    return _temperature;
}

float DHT11Sensor::getHumidity() {
    return _humidity;
}

uint8_t DHT11Sensor::getTemperatureCategory() {
    return _tempCategory;
}

void DHT11Sensor::_calculateCategory() {
    if (_temperature >= TEMP_WARNING_THRESHOLD) {
        _tempCategory = 1; // Warning
    } else {
        _tempCategory = 0; // Normal
    }
}