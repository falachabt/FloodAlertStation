#ifndef EINK_DISPLAY_H
#define EINK_DISPLAY_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <SPI.h>
#include "Config.h"
#include "utils/Logger.h"

// Forward declaration
class FloodAlertSystem;

// Define screen types
enum ScreenType {
    SCREEN_CLOCK = 0,
    SCREEN_WATER_LEVEL = 1,
    SCREEN_SYSTEM_INFO = 2,
    SCREEN_ALERT = 3,
    SCREEN_NETWORK = 4,
    SCREEN_WELCOME = 5
};

/**
 * E-Ink Display Indicator
 * 
 * This class manages an E-Ink display to provide visual information
 * about system state, water levels, alerts, and a clock when idle.
 */
class EInkDisplay {
public:
    // Constructor with pin configuration (default pins used if not specified)
    EInkDisplay(uint8_t csPin = 5, uint8_t dcPin = 17, uint8_t rstPin = 16, uint8_t busyPin = 4);
    
    // Initialize the display
    bool begin();
    
    // Update display based on system state
    void update();
    
    // Set reference to the main system
    void setFloodAlertSystem(FloodAlertSystem* system);
    
    // Change current screen
    void setScreen(ScreenType screenType);
    
    // Force a refresh of the current screen
    void refresh();
    
    // Different screen display methods
    void showClockScreen();
    void showWaterLevelScreen();
    void showSystemInfoScreen();
    void showAlertScreen(uint8_t category);
    void showNetworkScreen();
    void showWelcomeScreen();
    
    // Helper methods for drawing
    void drawCenteredText(const char* text, int x, int y, uint8_t textSize = 1);
    void drawTextAt(const char* text, int x, int y, uint8_t textSize = 1);
    void drawBigClock();
    void drawWaterLevel(float waterLevel, int x, int y, int width, int height);
    void drawTemperature(float temperature, int x, int y);
    void drawSystemStatus();
    void drawNetworkStatus();
    void drawBatteryLevel(int percentage, int x, int y);
    void drawProgressBar(int x, int y, int width, int height, int percentage);
    
    // Handle timed updates
    void tick();

private:
    // Display instance
    GxEPD2_BW<GxEPD2_579_GDEY0579T93, GxEPD2_579_GDEY0579T93::HEIGHT>* _display;
    
    // Pin configuration
    uint8_t _csPin;
    uint8_t _dcPin;
    uint8_t _rstPin;
    uint8_t _busyPin;
    
    // Reference to the main system
    FloodAlertSystem* _floodSystem;
    
    // Current screen and state
    ScreenType _currentScreen;
    bool _needsRefresh;
    bool _partialRefreshEnabled;
    
    // Screen rotation/orientation 
    uint8_t _rotation;
    
    // Update timing
    unsigned long _lastFullRefresh;
    unsigned long _lastClockUpdate;
    unsigned long _lastDataUpdate;
    static const unsigned long FULL_REFRESH_INTERVAL = 3600000;  // 1 hour full refresh to prevent ghosting
    static const unsigned long CLOCK_UPDATE_INTERVAL = 60000;    // 1 minute for clock updates
    static const unsigned long DATA_UPDATE_INTERVAL = 300000;    // 5 minutes for sensor data updates
    
    // Time tracking
    int _hour;
    int _minute;
    int _second;
    unsigned long _lastTimeUpdate;
    
    // Screen dimensions
    int _width;
    int _height;
    
    // Screen state
    bool _isInitialized;
    
    // Get current time
    void _updateTime();
    
    // Get formatted time string
    String _getTimeString();
    
    // Get formatted date string
    String _getDateString();
    
    // Check if display needs update based on system state
    bool _shouldUpdateDisplay();
    
    // Get alert title based on category
    const char* _getAlertTitle(uint8_t category);
    
    // Get alert message based on category
    const char* _getAlertMessage(uint8_t category);
};

#endif // EINK_DISPLAY_H