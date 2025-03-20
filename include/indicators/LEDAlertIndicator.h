#ifndef LED_ALERT_INDICATOR_H
#define LED_ALERT_INDICATOR_H

#include <Arduino.h>
#include "Config.h"

/**
 * LED Alert Indicator
 * 
 * This class manages a set of 3 LEDs (red, yellow, green) to provide visual alerts
 * based on water level data from sensors.
 */
class LEDAlertIndicator {
public:
    // Constructor with pin configuration
    LEDAlertIndicator(uint8_t redPin, uint8_t yellowPin, uint8_t greenPin);
    
    // Initialize the LEDs
    bool begin();
    
    // Update LED status based on water level data
    void update(float waterLevel, uint8_t category);
    
    // Show alert indication
    void showAlert(bool isAlert);
    
    // Blink a specific LED (useful for notifications)
    void blinkLED(uint8_t pin, int times, int delayMs = 200);
    
    // Direct control of LEDs
    void setRed(bool state);
    void setYellow(bool state);
    void setGreen(bool state);
    
    // Turn off all LEDs
    void allOff();

private:
    uint8_t _redPin;
    uint8_t _yellowPin;
    uint8_t _greenPin;
    
    bool _alertState;
    unsigned long _lastWaterLevelUpdate;
    float _lastWaterLevel;
    uint8_t _lastCategory;
    
    // Thresholds (can be set from Config.h)
    static const unsigned long ALERT_DELAY_MS = 5000; // 1 minute threshold
};

#endif // LED_ALERT_INDICATOR_H