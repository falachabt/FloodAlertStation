#include "indicators/BuzzerAlertIndicator.h"
#include "Config.h"
#include "utils/logger.h"

BuzzerAlertIndicator::BuzzerAlertIndicator(uint8_t buzzerPin)
    : _buzzerPin(buzzerPin),
      _alertState(false), 
      _silenced(false),
      _currentAlertType(0),
      _lastWaterLevelUpdate(0), 
      _lastWaterLevel(0), 
      _lastCategory(0),
      _lastToneToggle(0),
      _toneState(false) {
}

bool BuzzerAlertIndicator::begin() {
    // Initialize pin
    pinMode(_buzzerPin, OUTPUT);
    
    // Start with buzzer off
    setBuzzer(false);
    
    // Play a startup sound to indicate the buzzer is working
    playSuccessTone();
    
    Logger::info("Buzzer Alert Indicator initialized");
    return true;
}

void BuzzerAlertIndicator::update(float waterLevel, uint8_t category) {
    unsigned long currentTime = millis();
    
    // Print debug info to help troubleshoot
    // Serial.print("Buzzer Indicator update - Water level: ");
    // Serial.print(waterLevel);
    // Serial.print("cm, Category: ");
    // Serial.print(category);
    // Serial.print(", Alert state: ");
    // Serial.println(_alertState ? "ON" : "OFF");
    
    // Store the current time when we first detect a category change
    if (category != _lastCategory) {
        // Serial.print("Category changed from ");
        // Serial.print(_lastCategory);
        // Serial.print(" to ");
        // Serial.println(category);
        
        _lastWaterLevelUpdate = currentTime;
        
        // If transitioning to an alert state, reset silence status
        if (category >= 1 && _lastCategory < 1) {
            _silenced = false;
            // Serial.println("Starting alert timer - silence reset");
        }
        
        _lastCategory = category;
    }
    
    // Determine if we should be in alert state
    bool shouldAlert = false;
    
    // Immediate alert for critical water levels (category 2)
    if (category == 2) {
        shouldAlert = true;
        // Serial.println("CRITICAL WATER LEVEL - Immediate alert");
    }
    // Delayed alert for warning levels (category 1)
    else if (category == 1) {
        // Check if warning has persisted long enough
        if (currentTime - _lastWaterLevelUpdate >= ALERT_DELAY_MS) {
            shouldAlert = true;
            // Serial.println("WARNING PERSISTED - Activating alert");
        } else {
            // Serial.print("Warning condition: ");
            // Serial.print((currentTime - _lastWaterLevelUpdate) / 1000);
            // Serial.print(" seconds of ");
            // Serial.print(ALERT_DELAY_MS / 1000);
            // Serial.println(" required before alert");
        }
    }
    // Normal water level (category 0)
    else {
        shouldAlert = false;
        
        // If transitioning from alert to normal
        if (_alertState) {
            // Serial.println("WATER LEVEL NORMAL - Clearing alert");
            _silenced = false; // Reset silenced state when alert clears
        }
    }
    
    // Update alert state if it changed
    if (shouldAlert != _alertState) {
        showAlert(shouldAlert, category); // Use category as alert type
    }
    
    // Store the water level data
    _lastWaterLevel = waterLevel;
}

void BuzzerAlertIndicator::showAlert(bool isAlert, uint8_t alertType) {
    _alertState = isAlert;
    _currentAlertType = alertType;
    
    // Serial.print("Buzzer Alert state changed to: ");
    // Serial.println(isAlert ? "ACTIVE" : "INACTIVE");
    
    if (isAlert && !_silenced) {
        // Play initial alert tone
        playAlertTone(alertType);
    } else {
        // Turn off buzzer when alert is cleared
        setBuzzer(false);
    }
}

void BuzzerAlertIndicator::silenceAlert() {
    if (_alertState) {
        _silenced = true;
        setBuzzer(false);
        // Serial.println("Buzzer alert silenced by user");
    }
}

void BuzzerAlertIndicator::playAlertTone(uint8_t alertType) {
    // Use alertType to determine pattern - for now we'll use based on category
    switch (alertType) {
        case 2: // Critical water level
            // Urgent rapid beeping
            tone(_buzzerPin, 2500);
            delay(150);
            noTone(_buzzerPin);
            delay(50);
            tone(_buzzerPin, 2500);
            delay(150);
            noTone(_buzzerPin);
            delay(50);
            tone(_buzzerPin, 2500);
            delay(150);
            noTone(_buzzerPin);
            break;
            
        case 1: // Warning water level
            // Medium-pitched beeping
            tone(_buzzerPin, 2000);
            delay(200);
            noTone(_buzzerPin);
            delay(100);
            tone(_buzzerPin, 2000);
            delay(200);
            noTone(_buzzerPin);
            break;
            
        default: // Other alerts
            // Default alert sound
            tone(_buzzerPin, 1800);
            delay(200);
            noTone(_buzzerPin);
            delay(100);
            tone(_buzzerPin, 1800);
            delay(200);
            noTone(_buzzerPin);
    }
}

void BuzzerAlertIndicator::playWarningTone() {
    tone(_buzzerPin, 1500);
    delay(100);
    noTone(_buzzerPin);
    delay(50);
    tone(_buzzerPin, 1500);
    delay(100);
    noTone(_buzzerPin);
    delay(50);
    tone(_buzzerPin, 1500);
    delay(100);
    noTone(_buzzerPin);
}

void BuzzerAlertIndicator::playErrorTone() {
    tone(_buzzerPin, 400);
    delay(200);
    tone(_buzzerPin, 300);
    delay(200);
    tone(_buzzerPin, 200);
    delay(200);
    noTone(_buzzerPin);
}

void BuzzerAlertIndicator::playSuccessTone() {
    tone(_buzzerPin, 1000);
    delay(100);
    tone(_buzzerPin, 1500);
    delay(100);
    tone(_buzzerPin, 2000);
    delay(100);
    noTone(_buzzerPin);
}

void BuzzerAlertIndicator::setBuzzer(bool state) {
    if (state) {
        tone(_buzzerPin, 2000); // 2000 Hz tone
    } else {
        noTone(_buzzerPin);
    }
    
    _toneState = state;
    // Serial.print("Buzzer: ");
    // Serial.println(state ? "ON" : "OFF");
}

void BuzzerAlertIndicator::tick() {
    // This method should be called in every loop iteration
    // to handle alert pattern timing
    
    if (_alertState && !_silenced) {
        unsigned long currentTime = millis();
        unsigned long interval = _getToneInterval(_currentAlertType);
        
        // Toggle buzzer state based on the interval
        if (currentTime - _lastToneToggle >= interval) {
            _lastToneToggle = currentTime;
            _toneState = !_toneState;
            setBuzzer(_toneState);
        }
    } else {
        // Make sure buzzer is off when no alert is active
        if (_toneState) {
            setBuzzer(false);
        }
    }
}

unsigned long BuzzerAlertIndicator::_getToneInterval(uint8_t alertType) {
    switch (alertType) {
        case 2: return SYSTEM_ALERT_INTERVAL; // Critical - rapid beeping (250ms)
        case 1: return TEMP_ALERT_INTERVAL;   // Warning - medium beeping (500ms)
        default: return WATER_ALERT_INTERVAL; // Normal alert - slow beeping (1000ms)
    }
}