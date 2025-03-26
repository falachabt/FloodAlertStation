#ifndef BUZZER_ALERT_INDICATOR_H
#define BUZZER_ALERT_INDICATOR_H

#include <Arduino.h>
#include "Config.h"

/**
 * Buzzer Alert Indicator
 * 
 * This class manages a buzzer to provide audio alerts
 * based on water level data and other alert conditions.
 */
class BuzzerAlertIndicator {
public:
    // Constructor with pin configuration
    BuzzerAlertIndicator(uint8_t buzzerPin);
    
    // Initialize the buzzer
    bool begin();
    
    // Update buzzer status based on alert data
    void update(float waterLevel, uint8_t category);
    
    // Show alert indication with specific pattern based on alert type
    void showAlert(bool isAlert, uint8_t alertType = 0);
    
    // Manually silence the alert (e.g., when button is pressed)
    void silenceAlert();
    
    // Play different sound patterns
    void playAlertTone(uint8_t alertType);
    void playWarningTone();
    void playErrorTone();
    void playSuccessTone();
    
    // Direct control of buzzer
    void setBuzzer(bool state);
    
    // Update method to be called in the main loop
    void tick();

private:
    uint8_t _buzzerPin;
    
    bool _alertState;
    bool _silenced;
    uint8_t _currentAlertType;
    unsigned long _lastWaterLevelUpdate;
    float _lastWaterLevel;
    uint8_t _lastCategory;
    
    // Timing variables for alert patterns
    unsigned long _lastToneToggle;
    bool _toneState;
    
    // Timing parameters for different alert patterns
    unsigned long _getToneInterval(uint8_t alertType);
    
    // Alert sound constants
    static const unsigned long WATER_ALERT_INTERVAL = 1000;    // 1 second on, 1 second off
    static const unsigned long TEMP_ALERT_INTERVAL = 500;      // 0.5 second on, 0.5 second off
    static const unsigned long SYSTEM_ALERT_INTERVAL = 250;    // 0.25 second on, 0.25 second off
    
    // Thresholds
    static const unsigned long ALERT_DELAY_MS = WATER_ALERT_DELAY_MS; // Same as LED delay
};

#endif // BUZZER_ALERT_INDICATOR_H