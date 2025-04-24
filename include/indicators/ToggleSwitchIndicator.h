// include/indicators/ToggleSwitchIndicator.h
#ifndef TOGGLE_SWITCH_INDICATOR_H
#define TOGGLE_SWITCH_INDICATOR_H

#include <Arduino.h>
#include "Config.h"

/**
 * Toggle Switch Indicator
 * 
 * This class manages a toggle switch and associated LED to provide
 * manual control of alert states.
 */
class ToggleSwitchIndicator {
public:
    // Constructor with pin configuration
    ToggleSwitchIndicator(uint8_t switchPin, uint8_t ledPin);
    
    // Initialize the switch and LED
    bool begin();
    
    // Update switch status (to be called in main loop)
    bool update();
    
    // Get the current state of the toggle switch
    bool getSwitchState() const;
    
    // Get the toggle event (true when changed from OFF to ON)
    bool isToggleOn() const;
    
    // Get the untoggle event (true when changed from ON to OFF)
    bool isToggleOff() const;
    
    // Set the LED directly
    void setLed(bool state);
    
    // Reset the state change flags
    void resetEvents();

private:
    uint8_t _switchPin;
    uint8_t _ledPin;
    
    bool _currentSwitchState;
    bool _lastSwitchState;
    bool _ledState;
    
    bool _toggleOn;   // Flag for ON event
    bool _toggleOff;  // Flag for OFF event
    
    unsigned long _lastDebounceTime;
    unsigned long _debounceDelay;
};

#endif // TOGGLE_SWITCH_INDICATOR_H