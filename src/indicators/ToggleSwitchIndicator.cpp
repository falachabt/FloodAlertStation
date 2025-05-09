// src/indicators/ToggleSwitchIndicator.cpp
#include "indicators/ToggleSwitchIndicator.h"
#include "utils/logger.h"

ToggleSwitchIndicator::ToggleSwitchIndicator(uint8_t switchPin, uint8_t ledPin)
    : _switchPin(switchPin), 
      _ledPin(ledPin),
      _currentSwitchState(false),
      _lastSwitchState(false),
      _ledState(false),
      _toggleOn(false),
      _toggleOff(false),
      _lastDebounceTime(0),
      _debounceDelay(50) {
}

bool ToggleSwitchIndicator::begin() {
    // Initialize pins
    pinMode(_switchPin, INPUT_PULLUP); // Use internal pull-up
    pinMode(_ledPin, OUTPUT);
    
    // Start with LED off
    setLed(false);
    
    // Read initial state (accounting for pull-up)
    _lastSwitchState = digitalRead(_switchPin);
    _currentSwitchState = _lastSwitchState;
    
    Logger::info("Toggle Switch Indicator initialized");
    return true;
}

bool ToggleSwitchIndicator::update() {
    // Reset event flags
    _toggleOn = false;
    _toggleOff = false;
    
    // Read the current switch state
    bool reading = digitalRead(_switchPin);
    
    // Check if switch state changed (debounce)
    if (reading != _lastSwitchState) {
        _lastDebounceTime = millis();
    }
    
    // If sufficient time has passed for debounce
    if ((millis() - _lastDebounceTime) > _debounceDelay) {
        // If the switch state has changed
        if (reading != _currentSwitchState) {
            _currentSwitchState = reading;
            
            // For pull-up resistor, LOW means the switch is closed/ON
            bool switchOn = (_currentSwitchState == LOW);
            
            // Update LED state to match switch state
            setLed(switchOn);
            
            // Set event flags
            if (switchOn) {
                _toggleOn = true;
                Logger::info("Toggle switch turned ON");
            } else {
                _toggleOff = true;
                Logger::info("Toggle switch turned OFF");
            }
            
            return true; // State changed
        }
    }
    
    _lastSwitchState = reading;
    return false; // No state change
}

bool ToggleSwitchIndicator::getSwitchState() const {
    // With pull-up resistor, LOW means the switch is closed/ON
    return (_currentSwitchState == LOW);
}

bool ToggleSwitchIndicator::isToggleOn() const {
    return _toggleOn;
}

bool ToggleSwitchIndicator::isToggleOff() const {
    return _toggleOff;
}

void ToggleSwitchIndicator::setLed(bool state) {
    _ledState = state;
    digitalWrite(_ledPin, state ? HIGH : LOW);
}

void ToggleSwitchIndicator::resetEvents() {
    _toggleOn = false;
    _toggleOff = false;
}