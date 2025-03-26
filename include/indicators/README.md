# Audio Alert System for Flood Alert Station

This document explains the integration of a buzzer-based audio alert system into the Flood Alert Station project.

## Overview

The audio alert system complements the existing LED visual alerts by providing audible notifications when alert conditions are detected. This is particularly useful for situations where visual alerts might be missed.

## Hardware Requirements

- ESP32 development board
- Buzzer module (active or passive)
- Connecting wires

## Wiring

Connect the buzzer to the ESP32 as follows:

- Buzzer positive (+) terminal → GPIO pin defined in `BUZZER_PIN` (default: GPIO 15)
- Buzzer negative (-) terminal → GND

## Features

The audio alert system provides:

1. **Different alert patterns** based on alert type:
   - Water level alerts: Long beeps (1 second on, 1 second off)
   - Temperature alerts: Medium beeps (0.5 second on, 0.5 second off)
   - System alerts: Rapid beeps (0.25 second on, 0.25 second off)

2. **Alert silencing** via serial command:
   - Send "silence" or "s" via the serial monitor to silence the current alert
   - The alert can be reactivated when a new alert condition is detected

3. **Testing features** via serial command:
   - Send "test" or "t" via the serial monitor to test all alert sounds

4. **Startup and status tones**:
   - Success tone: Ascending beeps when the system starts successfully
   - Warning tone: Repeated beeps for warning conditions
   - Error tone: Descending beeps for error conditions

## Implementation Details

The buzzer alert system is implemented in two main components:

1. **BuzzerAlertIndicator Class** (in `indicators/BuzzerAlertIndicator.h/.cpp`):
   - Handles the direct control of the buzzer hardware
   - Implements different sound patterns for various alert types
   - Provides methods for silencing and testing alerts

2. **Integration in FloodAlertSystem** (modified in `FloodAlertSystem.h/.cpp`):
   - Manages the buzzer indicator alongside the LED indicator
   - Processes serial commands for silencing alerts
   - Updates the buzzer based on sensor data

## Configuration

The buzzer pin is defined in `Config.h` as `BUZZER_PIN`. You can change this value to match your hardware setup.

## Usage

In normal operation, the buzzer will automatically sound when alert conditions are detected, following the same alert thresholds used for the LED indicators.

To silence an active alert:
1. Open the serial monitor (115200 baud)
2. Type "silence" or "s" and press Enter

To test all alert sounds:
1. Open the serial monitor (115200 baud)
2. Type "test" or "t" and press Enter

## Future Improvements

Possible future improvements include:
- Physical silence button integration
- Configurable alert tone frequencies and patterns
- Time-based automatic silencing (e.g., silence after a certain period)
- Web interface controls for audio alert settings