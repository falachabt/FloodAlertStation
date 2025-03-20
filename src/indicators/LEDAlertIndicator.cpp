#include "indicators/LEDAlertIndicator.h"
#include "Config.h"

LEDAlertIndicator::LEDAlertIndicator(uint8_t redPin, uint8_t yellowPin, uint8_t greenPin)
    : _redPin(redPin), _yellowPin(yellowPin), _greenPin(greenPin),
      _alertState(false), _lastWaterLevelUpdate(0), _lastWaterLevel(0), _lastCategory(0) {
}

bool LEDAlertIndicator::begin() {
    // Initialize pins
    pinMode(_redPin, OUTPUT);
    pinMode(_yellowPin, OUTPUT);
    pinMode(_greenPin, OUTPUT);
    
    // Start with all LEDs off
    allOff();
    
    // Flash all LEDs to indicate startup
    blinkLED(_greenPin, 1);
    blinkLED(_yellowPin, 1);
    blinkLED(_redPin, 1);
    
    Serial.println("LED Alert Indicator initialized");
    return true;
}

void LEDAlertIndicator::update(float waterLevel, uint8_t category) {
    unsigned long currentTime = millis();
    
    // Print debug info to help troubleshoot
    Serial.print("LED Indicator update - Water level: ");
    Serial.print(waterLevel);
    Serial.print("cm, Category: ");
    Serial.print(category);
    Serial.print(", Alert state: ");
    Serial.println(_alertState ? "ON" : "OFF");
    
    // Check if category has changed
    if (category != _lastCategory) {
        Serial.print("Category changed from ");
        Serial.print(_lastCategory);
        Serial.print(" to ");
        Serial.println(category);
        
        // If we're transitioning to a high water level (cat 1 or 2)
        if (category >= 1 && _lastCategory < 1) {
            _lastWaterLevelUpdate = currentTime;
            Serial.println("Starting high water timer");
        }
        // If we're transitioning to normal water level
        else if (category == 0 && _lastCategory >= 1) {
            _lastWaterLevelUpdate = currentTime;
            Serial.println("Starting normal water timer");
        }
        
        _lastCategory = category;
    }
    
    // Check for alert conditions
    bool shouldAlert = _alertState; // Start with current state
    
    // HIGH WATER CONDITION
    if (category >= 1) { // Warning or critical
        // If this is persistent high water
        if (currentTime - _lastWaterLevelUpdate >= ALERT_DELAY_MS) {
            if (!_alertState) {
                Serial.println("ACTIVATING ALERT: High water persistent for > 1 minute");
            }
            shouldAlert = true;
        }
    } 
    // NORMAL WATER CONDITION
    else {
        // If water has been normal for the required time
        if (_alertState && (currentTime - _lastWaterLevelUpdate >= ALERT_DELAY_MS)) {
            Serial.println("CLEARING ALERT: Normal water persistent for > 1 minute");
            shouldAlert = false;
        }
    }
    
    // If alert state should change, update the LEDs
    if (shouldAlert != _alertState) {
        showAlert(shouldAlert);
    }
    
    // Update status LEDs based on current category
    switch (category) {
        case 0: // Normal
            setGreen(true);
            setYellow(false);
            break;
        case 1: // Warning
            setGreen(false);
            setYellow(true);
            break;
        case 2: // Critical
            setGreen(false);
            if (!_alertState) {
                // Blink yellow if we're not yet in full alert
                setYellow(true);
            } else {
                setYellow(false);
            }
            break;
    }
    
    // Store the water level data
    _lastWaterLevel = waterLevel;
}

void LEDAlertIndicator::showAlert(bool isAlert) {
    _alertState = isAlert;
    
    Serial.print("Alert state changed to: ");
    Serial.println(isAlert ? "ACTIVE" : "INACTIVE");
    
    if (isAlert) {
        // Turn on red LED for alert
        setRed(true);
        
        // Blink yellow LED to draw attention
        blinkLED(_yellowPin, 3);
    } else {
        // Turn off red LED when alert is cleared
        setRed(false);
        
        // Flash green to indicate all is well
        blinkLED(_greenPin, 2);
    }
}

void LEDAlertIndicator::blinkLED(uint8_t pin, int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        delay(delayMs);
        digitalWrite(pin, LOW);
        delay(delayMs);
    }
}

void LEDAlertIndicator::setRed(bool state) {
    digitalWrite(_redPin, state ? HIGH : LOW);
    Serial.print("Red LED: ");
    Serial.println(state ? "ON" : "OFF");
}

void LEDAlertIndicator::setYellow(bool state) {
    digitalWrite(_yellowPin, state ? HIGH : LOW);
    Serial.print("Yellow LED: ");
    Serial.println(state ? "ON" : "OFF");
}

void LEDAlertIndicator::setGreen(bool state) {
    digitalWrite(_greenPin, state ? HIGH : LOW);
    Serial.print("Green LED: ");
    Serial.println(state ? "ON" : "OFF");
}

void LEDAlertIndicator::allOff() {
    digitalWrite(_redPin, LOW);
    digitalWrite(_yellowPin, LOW);
    digitalWrite(_greenPin, LOW);
}