#ifndef SENSOR_BASE_H
#define SENSOR_BASE_H

#include <Arduino.h>

class SensorBase {
public:
    virtual ~SensorBase() {}
    
    // Méthodes communes à tous les capteurs
    virtual bool begin() = 0;  // Initialisation du capteur
    virtual void update() = 0; // Mise à jour des valeurs
    virtual const char* getName() = 0; // Nom du capteur
    
    // Pour préparer les données à envoyer via le réseau
    virtual void getData(float* data, uint8_t& count) = 0;
    
    // État du capteur
    bool isActive() { return _isActive; }
    unsigned long getLastReadTime() { return _lastReadTime; }
    
protected:
    bool _isActive = false;
    unsigned long _lastReadTime = 0;
};

#endif // SENSOR_BASE_H