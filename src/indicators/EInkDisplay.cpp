#include "indicators/EInkDisplay.h"
#include "FloodAlertSystem.h"
#include <time.h>

// Constructor
EInkDisplay::EInkDisplay(uint8_t csPin, uint8_t dcPin, uint8_t rstPin, uint8_t busyPin)
    : _csPin(csPin), _dcPin(dcPin), _rstPin(rstPin), _busyPin(busyPin),
      _floodSystem(nullptr),
      _currentScreen(SCREEN_WELCOME),
      _needsRefresh(true),
      _partialRefreshEnabled(false),
      _rotation(2),
      _lastFullRefresh(0),
      _lastClockUpdate(0),
      _lastDataUpdate(0),
      _hour(12), _minute(0), _second(0),
      _lastTimeUpdate(0),
      _isInitialized(false) {
}

// Initialize the display
bool EInkDisplay::begin() {
    Logger::info("Initializing E-Ink display...");
    
    // Initialize SPI
    SPI.begin(/*SCK*/18, /*MISO*/-1, /*MOSI*/23, /*SS*/_csPin);
    
    // Create GxEPD2 instance
    GxEPD2_579_GDEY0579T93 epd_raw(_csPin, _dcPin, _rstPin, _busyPin);
    _display = new GxEPD2_BW<GxEPD2_579_GDEY0579T93, GxEPD2_579_GDEY0579T93::HEIGHT>(epd_raw);
    
    // Initialize the display
    _display->init();
    _width = _display->width();    // 792 for this model
    _height = _display->height();  // 272 for this model
    
    // Set orientation
    _display->setRotation(_rotation);  // Landscape mode
    
    // Show welcome screen
    showWelcomeScreen();
    
    // Initial time
    _updateTime();
    
    _isInitialized = true;
    _lastFullRefresh = millis();
    _lastClockUpdate = millis();
    _lastDataUpdate = millis();
    
    Logger::info("E-Ink display initialized successfully");
    return true;
}

// Update display based on system state
void EInkDisplay::update() {
    if (!_isInitialized || !_floodSystem) return;
    
    unsigned long currentTime = millis();
    
    // Update time
    if (currentTime - _lastTimeUpdate >= 1000) {
        _updateTime();
        _lastTimeUpdate = currentTime;
    }
    
    // Check if we need a display refresh based on timing or system state
    bool needClockUpdate = (currentTime - _lastClockUpdate >= CLOCK_UPDATE_INTERVAL);
    bool needDataUpdate = (currentTime - _lastDataUpdate >= DATA_UPDATE_INTERVAL);
    bool needFullRefresh = (currentTime - _lastFullRefresh >= FULL_REFRESH_INTERVAL);
    
    // Get system status to determine which screen to show
    bool networkReady = _floodSystem->getNetwork().isNetworkReady();
    uint8_t category = 0; // Default to normal
    
    // Determine highest alert category from all sensors
    if (_floodSystem) {
        // Logic to find the highest alert category
        // This would need to access the sensor data from _floodSystem
        // For now, we'll use a placeholder
        // In a real implementation, you would iterate through _floodSystem->_remoteSensors
        // and find the highest category value
        
        // Placeholder - in the real implementation, get this from system
        category = 0; // 0 = normal, 1 = warning, 2 = alert
    }
    
    // Choose screen based on system state
    ScreenType targetScreen = _currentScreen;
    
    if (category >= 2) {
        // Critical alert - show alert screen
        targetScreen = SCREEN_ALERT;
    } 
    else if (category == 1) {
        // Warning - show water level screen
        targetScreen = SCREEN_WATER_LEVEL;
    }
    else if (!networkReady) {
        // Network issues - show network screen
        targetScreen = SCREEN_NETWORK;
    }
    else if (needDataUpdate) {
        // Cycle between screens during regular updates
        // This rotates through main screens every 5 minutes
        switch (_currentScreen) {
            case SCREEN_CLOCK:
                targetScreen = SCREEN_WATER_LEVEL;
                break;
            case SCREEN_WATER_LEVEL:
                targetScreen = SCREEN_SYSTEM_INFO;
                break;
            case SCREEN_SYSTEM_INFO:
                targetScreen = SCREEN_NETWORK;
                break;
            case SCREEN_NETWORK:
            case SCREEN_ALERT:
            default:
                targetScreen = SCREEN_CLOCK;
                break;
        }
    }
    else if (needClockUpdate && _currentScreen == SCREEN_CLOCK) {
        // Just update the clock if we're already showing it
        _needsRefresh = true;
    }
    
    // If screen changed or refresh needed
    if (targetScreen != _currentScreen || _needsRefresh || needFullRefresh) {
        _currentScreen = targetScreen;
        
        // Do a full refresh periodically to prevent ghosting
        if (needFullRefresh) {
            _partialRefreshEnabled = false;
            _lastFullRefresh = currentTime;
        } else {
            _partialRefreshEnabled = true;
        }
        
        // Display the current screen
        refresh();
        
        // Update timing
        if (needClockUpdate) _lastClockUpdate = currentTime;
        if (needDataUpdate) _lastDataUpdate = currentTime;
    }
}

// Set reference to the main system
void EInkDisplay::setFloodAlertSystem(FloodAlertSystem* system) {
    _floodSystem = system;
}

// Change current screen
void EInkDisplay::setScreen(ScreenType screenType) {
    if (_currentScreen != screenType) {
        _currentScreen = screenType;
        _needsRefresh = true;
    }
}

// Force a refresh of the current screen
void EInkDisplay::refresh() {
    if (!_isInitialized) return;
    
    // Determine alert category outside of switch statement
    uint8_t category = 0;
    if (_floodSystem && _currentScreen == SCREEN_ALERT) {
        // In a real implementation, get this from system
        // category = highest alert from system
    }
    
    // Display the appropriate screen
    switch (_currentScreen) {
        case SCREEN_CLOCK:
            showClockScreen();
            break;
        case SCREEN_WATER_LEVEL:
            showWaterLevelScreen();
            break;
        case SCREEN_SYSTEM_INFO:
            showSystemInfoScreen();
            break;
        case SCREEN_ALERT:
            showAlertScreen(category);
            break;
        case SCREEN_NETWORK:
            showNetworkScreen();
            break;
        case SCREEN_WELCOME:
        default:
            showWelcomeScreen();
            break;
    }
    
    _needsRefresh = false;
}

// Different screen display methods
void EInkDisplay::showClockScreen() {
    Logger::debug("Showing clock screen");
    
    _display->setFullWindow();
    
    // Use partial refresh if enabled
    if (_partialRefreshEnabled) {
        // For real partial refresh, you'd need to set a partial window
        // that covers just the clock area
        _display->setPartialWindow(0, 0, _width, _height);
    }
    
    _display->firstPage();
    do {
        _display->fillScreen(GxEPD_WHITE);
        
        // Draw large clock in center
        drawBigClock();
        
        // Draw date below clock
        drawCenteredText(_getDateString().c_str(), _width/2, _height/2 + 50, 2);
        
        // Draw system status footer
        _display->drawLine(0, _height - 40, _width, _height - 40, GxEPD_BLACK);
        drawSystemStatus();
        
    } while (_display->nextPage());
    
    // Hibernate to save power
    _display->hibernate();
}

void EInkDisplay::showWaterLevelScreen() {
    Logger::debug("Showing water level screen");
    
    // Get sensor data - in real implementation, get this from system
    float waterLevel = 0.0f;
    float temperature = 25.0f;
    
    _display->setFullWindow();
    _display->firstPage();
    
    do {
        _display->fillScreen(GxEPD_WHITE);
        
        // Title
        drawCenteredText("NIVEAU D'EAU", _width/2, 30, 3);
        
        // Water level visualization
        drawWaterLevel(waterLevel, 50, 70, _width - 100, 100);
        
        // Temperature display
        drawTemperature(temperature, _width - 150, 70);
        
        // Current time in corner
        _display->setCursor(20, 30);
        _display->setTextSize(1);
        _display->print(_getTimeString());
        
        // Draw system status footer
        _display->drawLine(0, _height - 40, _width, _height - 40, GxEPD_BLACK);
        drawSystemStatus();
        
    } while (_display->nextPage());
    
    _display->hibernate();
}

void EInkDisplay::showSystemInfoScreen() {
    Logger::debug("Showing system info screen");
    
    _display->setFullWindow();
    _display->firstPage();
    
    do {
        _display->fillScreen(GxEPD_WHITE);
        
        // Title
        drawCenteredText("INFORMATIONS SYSTÈME", _width/2, 30, 2);
        
        // System information
        int lineHeight = 40;
        int startY = 80;
        int leftColumnX = 50;
        int rightColumnX = _width/2 + 50;
        
        // Left column - Device info
        _display->setTextSize(1);
        _display->setCursor(leftColumnX, startY);
        _display->print("Nom de l'appareil: ");
        _display->print(DEVICE_NAME);
        
        _display->setCursor(leftColumnX, startY + lineHeight);
        _display->print("Mode: ");
        _display->print(MODE_MASTER ? "MASTER" : "SLAVE");
        
        _display->setCursor(leftColumnX, startY + lineHeight*2);
        _display->print("Adresse MAC: ");
        // Get MAC address from system in real implementation
        _display->print("XX:XX:XX:XX:XX:XX");
        
        _display->setCursor(leftColumnX, startY + lineHeight*3);
        _display->print("Temps de fonctionnement: ");
        // Get uptime from system
        unsigned long uptime = millis() / 1000;
        int hours = uptime / 3600;
        int mins = (uptime % 3600) / 60;
        int secs = uptime % 60;
        _display->printf("%02d:%02d:%02d", hours, mins, secs);
        
        // Right column - Sensor info
        _display->setCursor(rightColumnX, startY);
        _display->print("Capteurs actifs: ");
        // Get from system
        _display->print("0");
        
        _display->setCursor(rightColumnX, startY + lineHeight);
        _display->print("Niveau d'alerte: ");
        // Get from system
        _display->print("Normal");
        
        _display->setCursor(rightColumnX, startY + lineHeight*2);
        _display->print("Batterie: ");
        // Get from system
        int batteryLevel = 100;
        _display->print(batteryLevel);
        _display->print("%");
        drawBatteryLevel(batteryLevel, rightColumnX + 100, startY + lineHeight*2 - 5);
        
        // Current time in corner
        _display->setCursor(20, 30);
        _display->setTextSize(1);
        _display->print(_getTimeString());
        
        // Draw system status footer
        _display->drawLine(0, _height - 40, _width, _height - 40, GxEPD_BLACK);
        drawSystemStatus();
        
    } while (_display->nextPage());
    
    _display->hibernate();
}

void EInkDisplay::showAlertScreen(uint8_t category) {
    Logger::debug("Showing alert screen");
    
    _display->setFullWindow();
    _display->firstPage();
    
    do {
        if (category >= 2) {
            // Critical alert - use inverse display for emphasis
            _display->fillScreen(GxEPD_BLACK);
            _display->setTextColor(GxEPD_WHITE);
        } else {
            _display->fillScreen(GxEPD_WHITE);
            _display->setTextColor(GxEPD_BLACK);
        }
        
        // Title
        drawCenteredText(_getAlertTitle(category), _width/2, _height/2 - 50, 4);
        
        // Alert message
        drawCenteredText(_getAlertMessage(category), _width/2, _height/2 + 20, 2);
        
        // Current time and date
        if (category >= 2) {
            _display->setTextColor(GxEPD_WHITE);
        } else {
            _display->setTextColor(GxEPD_BLACK);
        }
        
        _display->setCursor(20, 30);
        _display->setTextSize(1);
        _display->print(_getTimeString());
        _display->print(" - ");
        _display->print(_getDateString());
        
        // Water level if available
        float waterLevel = 0.0f; // Get from system
        _display->setCursor(_width - 150, 30);
        _display->print("Niveau: ");
        _display->print(waterLevel, 1);
        _display->print(" cm");
        
    } while (_display->nextPage());
    
    _display->hibernate();
}

void EInkDisplay::showNetworkScreen() {
    Logger::debug("Showing network screen");
    
    _display->setFullWindow();
    _display->firstPage();
    
    do {
        _display->fillScreen(GxEPD_WHITE);
        
        // Title
        drawCenteredText("ÉTAT DU RÉSEAU", _width/2, 30, 2);
        
        // Network information
        drawNetworkStatus();
        
        // Current time in corner
        _display->setCursor(20, 30);
        _display->setTextSize(1);
        _display->print(_getTimeString());
        
    } while (_display->nextPage());
    
    _display->hibernate();
}

void EInkDisplay::showWelcomeScreen() {
    Logger::debug("Showing welcome screen");
    
    _display->setFullWindow();
    _display->firstPage();
    
    do {
        _display->fillScreen(GxEPD_WHITE);
        
        // Logo or title
        drawCenteredText("FLOOD ALERT SYSTEM", _width/2, _height/2 - 50, 3);
        
        // Subtitle
        drawCenteredText("Système de Surveillance des Inondations", _width/2, _height/2, 2);
        
        // Version
        drawCenteredText("v1.0", _width/2, _height/2 + 40, 1);
        
        // Status message
        drawCenteredText("Initialisation en cours...", _width/2, _height/2 + 80, 1);
        
    } while (_display->nextPage());
    
    _display->hibernate();
}

// Helper methods for drawing
void EInkDisplay::drawCenteredText(const char* text, int x, int y, uint8_t textSize) {
    _display->setTextSize(textSize);
    int16_t x1, y1;
    uint16_t w, h;
    _display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    _display->setCursor(x - w/2, y);
    _display->print(text);
}

void EInkDisplay::drawTextAt(const char* text, int x, int y, uint8_t textSize) {
    _display->setTextSize(textSize);
    _display->setCursor(x, y);
    _display->print(text);
}

void EInkDisplay::drawBigClock() {
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", _hour, _minute, _second);
    
    // Calculate font size based on display width
    int textSize = 6; // Adjust as needed for your display
    
    _display->setTextSize(textSize);
    
    // Calculate text bounds
    int16_t x1, y1;
    uint16_t w, h;
    _display->getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
    
    // Center the clock
    _display->setCursor(_width/2 - w/2, _height/2);
    _display->print(timeStr);
}

void EInkDisplay::drawWaterLevel(float waterLevel, int x, int y, int width, int height) {
    // Draw tank outline
    _display->drawRect(x, y, width, height, GxEPD_BLACK);
    
    // Calculate fill level (max 100 cm)
    int maxWaterLevel = 100; // cm
    int fillHeight = min(height, (int)(height * waterLevel / maxWaterLevel));
    
    // Draw fill
    _display->fillRect(x, y + height - fillHeight, width, fillHeight, GxEPD_BLACK);
    
    // Draw measurement scale (every 20 cm)
    for (int i = 0; i <= maxWaterLevel; i += 20) {
        int lineY = y + height - (i * height / maxWaterLevel);
        _display->drawLine(x - 5, lineY, x, lineY, GxEPD_BLACK);
        
        // Draw scale labels
        _display->setCursor(x - 30, lineY + 4);
        _display->setTextSize(1);
        _display->print(i);
    }
    
    // Draw current value
    char valueStr[10];
    sprintf(valueStr, "%.1f cm", waterLevel);
    
    _display->setTextSize(2);
    _display->setCursor(x + width/2 - 50, y + height + 30);
    _display->print(valueStr);
}

void EInkDisplay::drawTemperature(float temperature, int x, int y) {
    _display->setTextSize(3);
    _display->setCursor(x, y);
    _display->print(temperature, 1);
    _display->print("°C");
}

void EInkDisplay::drawSystemStatus() {
    int y = _height - 25;
    
    _display->setTextColor(GxEPD_BLACK);
    _display->setTextSize(1);
    
    // Left side - network status
    _display->setCursor(20, y);
    
    if (_floodSystem) {
        bool networkReady = _floodSystem->getNetwork().isNetworkReady();
        uint8_t peerCount = _floodSystem->getNetwork().getPeerCount();
        
        _display->print("Réseau: ");
        _display->print(networkReady ? "CONNECTÉ" : "DÉCONNECTÉ");
        _display->print(" | Pairs: ");
        _display->print(peerCount);
    } else {
        _display->print("Système non disponible");
    }
    
    // Right side - alert status
    _display->setCursor(_width - 200, y);
    
    // In real implementation, get from system
    uint8_t category = 0;
    _display->print("État: ");
    
    switch (category) {
        case 0:
            _display->print("NORMAL");
            break;
        case 1:
            _display->print("VIGILANCE");
            break;
        case 2:
            _display->print("ALERTE");
            break;
        case 3:
            _display->print("DANGER");
            break;
        default:
            _display->print("INCONNU");
    }
}

void EInkDisplay::drawNetworkStatus() {
    if (!_floodSystem) return;
    
    int startY = 80;
    int lineHeight = 30;
    int leftX = 50;
    
    _display->setTextSize(1);
    
    // Title row
    _display->setCursor(leftX, startY);
    _display->print("État du réseau: ");
    
    bool networkReady = _floodSystem->getNetwork().isNetworkReady();
    _display->print(networkReady ? "CONNECTÉ" : "DÉCONNECTÉ");
    
    // Number of peers
    _display->setCursor(leftX, startY + lineHeight);
    _display->print("Nombre de pairs: ");
    _display->print(_floodSystem->getNetwork().getPeerCount());
    
    // Master info if slave
    if (!MODE_MASTER) {
        _display->setCursor(leftX, startY + lineHeight*2);
        _display->print("Connecté au master: ");
        _display->print(_floodSystem->getNetwork().isConnectedToMaster() ? "OUI" : "NON");
    }
    
    // Connected peers
    _display->setCursor(leftX, startY + lineHeight*3);
    _display->print("Liste des pairs connectés:");
    
    // In real implementation, get peer list from system
    // Placeholder logic
    _display->setCursor(leftX + 20, startY + lineHeight*4);
    _display->print("Aucun pair connecté");
    
    // Draw WiFi signal strength icon if applicable
    // Add logic based on your system
}

void EInkDisplay::drawBatteryLevel(int percentage, int x, int y) {
    int width = 30;
    int height = 15;
    
    // Battery outline
    _display->drawRect(x, y, width, height, GxEPD_BLACK);
    _display->drawRect(x + width, y + 3, 3, height - 6, GxEPD_BLACK);
    
    // Battery fill
    int fillWidth = (width - 4) * percentage / 100;
    _display->fillRect(x + 2, y + 2, fillWidth, height - 4, GxEPD_BLACK);
}

void EInkDisplay::drawProgressBar(int x, int y, int width, int height, int percentage) {
    // Draw outline
    _display->drawRect(x, y, width, height, GxEPD_BLACK);
    
    // Draw fill
    int fillWidth = (width - 2) * percentage / 100;
    _display->fillRect(x + 1, y + 1, fillWidth, height - 2, GxEPD_BLACK);
    
    // Draw percentage
    char percentStr[5];
    sprintf(percentStr, "%d%%", percentage);
    
    int16_t x1, y1;
    uint16_t w, h;
    _display->getTextBounds(percentStr, 0, 0, &x1, &y1, &w, &h);
    
    _display->setCursor(x + width/2 - w/2, y + height/2 + h/2);
    _display->print(percentStr);
}

// Handle timed updates
void EInkDisplay::tick() {
    // This can be called from the main loop to handle updates
    update();
}

// Private methods
void EInkDisplay::_updateTime() {
    // In a real implementation, this would get time from RTC or NTP
    // For now, just increment the time manually
    unsigned long currentMillis = millis();
    unsigned long elapsedSeconds = (currentMillis - _lastTimeUpdate) / 1000;
    
    if (elapsedSeconds > 0) {
        _second += elapsedSeconds;
        
        if (_second >= 60) {
            _minute += _second / 60;
            _second %= 60;
            
            if (_minute >= 60) {
                _hour += _minute / 60;
                _minute %= 60;
                
                if (_hour >= 24) {
                    _hour %= 24;
                }
            }
        }
        
        _lastTimeUpdate = currentMillis;
    }
}

String EInkDisplay::_getTimeString() {
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", _hour, _minute, _second);
    return String(timeStr);
}

String EInkDisplay::_getDateString() {
    // In a real implementation, use a RTC or NTP to get date
    // Placeholder implementation
    static const char* DAYS[] = {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"};
    static const char* MONTHS[] = {"Jan", "Fév", "Mar", "Avr", "Mai", "Jun", "Jul", "Aou", "Sep", "Oct", "Nov", "Déc"};
    
    // Placeholder date - in real implementation get from RTC
    int day = 1;      // Day of month
    int month = 0;    // Month (0-11)
    int year = 2025;  // Year
    int weekday = 3;  // Wednesday
    
    char dateStr[20];
    sprintf(dateStr, "%s %d %s %d", DAYS[weekday], day, MONTHS[month], year);
    return String(dateStr);
}

bool EInkDisplay::_shouldUpdateDisplay() {
    // Check system state to determine if display needs update
    // This would integrate with your system's alert levels
    return true;
}

const char* EInkDisplay::_getAlertTitle(uint8_t category) {
    const char* titles[] = {
        "NORMAL",
        "VIGILANCE",
        "ALERTE",
        "DANGER"
    };
    
    if (category < 4) {
        return titles[category];
    }
    
    return "ALERTE INCONNUE";
}

const char* EInkDisplay::_getAlertMessage(uint8_t category) {
    const char* messages[] = {
        "Aucune action requise",
        "Suivez l'évolution du niveau d'eau",
        "Préparez-vous pour une possible évacuation",
        "ÉVACUATION RECOMMANDÉE"
    };
    
    if (category < 4) {
        return messages[category];
    }
    
    return "État inconnu";
}