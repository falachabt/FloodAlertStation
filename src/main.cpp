#include "FloodAlertSystem.h"
#include "Config.h"
// Inclure les capteurs
#include "sensors/DHT11Sensor.h"
#include "sensors/WaterLevelSensor.h"
#include "indicators/LEDAlertIndicator.h"
#include "indicators/BuzzerAlertIndicator.h"
#include "indicators/EInkDisplay.h" // Include E-Ink display
// Inclure le nouvel encodeur rotatif et le système de menu
#include "indicators/RotaryEncoder.h"
#include "Menu.h"
// Inclure le système de logs
#include "utils/Logger.h"
#include <Ticker.h>

Ticker alertTestTicker;
bool testingAlert = false;
int testPhase = 0;

// Créer l'instance du système
FloodAlertSystem floodSystem;

// Créer les instances des indicateurs
LEDAlertIndicator ledIndicator(LED_RED_PIN, LED_YELLOW_PIN, LED_GREEN_PIN);
BuzzerAlertIndicator buzzerIndicator(BUZZER_PIN);
EInkDisplay einkDisplay(5, 17, 16, 4); // Create E-Ink display with default pins
ToggleSwitchIndicator toggleSwitchIndicator(TOGGLE_SWITCH_PIN, ADDITIONAL_LED_PIN);

// Créer l'instance de l'encodeur rotatif et du menu (uniquement pour le master)
RotaryEncoder* encoder = nullptr;
Menu* menu = nullptr;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialiser le système de logs
    Logger::begin(LOG_LEVEL_INFO);
    
    Logger::ui("\n\n=== SYSTÈME D'ALERTE DE CRUE ===");
    Logger::ui("Tapez 'silence' ou 's' pour couper les alertes sonores");
    Logger::ui("Tapez 'test' ou 't' pour tester les indicateurs");
    Logger::ui("Utilisez l'encodeur rotatif pour naviguer dans le menu");
    Logger::ui("Appui long pour activer/désactiver les logs système");
    
    // Initialiser le système dans le mode approprié défini dans Config.h
    bool isMaster = MODE_MASTER;
    
    // Initialiser les indicateurs
    ledIndicator.begin();
    buzzerIndicator.begin();
    einkDisplay.begin(); // Initialize E-Ink display
    toggleSwitchIndicator.begin(); // Initialize toggle switch indicator
    
    // Configurer les indicateurs dans le système
    floodSystem.setLEDIndicator(&ledIndicator);
    floodSystem.setBuzzerIndicator(&buzzerIndicator);
    floodSystem.setEInkDisplay(&einkDisplay); // Set E-Ink display
    floodSystem.setToggleSwitchIndicator(&toggleSwitchIndicator);

    // Initialiser l'encodeur rotatif et le menu si en mode master
    if (isMaster) {
        encoder = new RotaryEncoder(ROTARY_ENCODER_DT_PIN, ROTARY_ENCODER_CLK_PIN, ROTARY_ENCODER_SW_PIN);
        encoder->begin();
        
        // Créer et initialiser le système de menu
        menu = new Menu(encoder);
        menu->setFloodAlertSystem(&floodSystem);
        menu->begin();
        
        Logger::info("Interface de menu initialisée");
    }
    
    // Initialiser le système
    floodSystem.begin(isMaster);
    
    if (isMaster) {
        Logger::info("Système initialisé en mode MASTER");
        
        // En mode MASTER, ajouter le capteur DHT11
        floodSystem.addSensor(new DHT11Sensor(DHT11_PIN));
        Logger::info("Capteur DHT11 ajouté au master");
        
        // Set E-Ink to show system info screen initially
        einkDisplay.setScreen(SCREEN_SYSTEM_INFO);
        einkDisplay.refresh();
        
    } else {
        Logger::info("Système initialisé en mode SLAVE");
        
        // En mode SLAVE, ajouter le capteur de niveau d'eau
        floodSystem.addSensor(new WaterLevelSensor(WATER_LEVEL_SENSOR_PIN));
        Logger::info("Capteur de niveau d'eau ajouté au slave");
        
        // Set E-Ink to show water level screen initially for slave
        einkDisplay.setScreen(SCREEN_WATER_LEVEL);
        einkDisplay.refresh();
    }
    
    // Test initial des indicateurs
    ledIndicator.setGreen(true);
    buzzerIndicator.playSuccessTone();
    
    Logger::ui("Configuration terminée!");
    
    // Switch to clock screen after 10 seconds
    delay(10000);
    einkDisplay.setScreen(SCREEN_CLOCK);
    einkDisplay.refresh();
}

void loop() {
    // Mettre à jour le système (gère tous les composants)
    floodSystem.update();
    
    // Mettre à jour l'encodeur rotatif et le menu si en mode master
    if (MODE_MASTER && encoder != nullptr && menu != nullptr) {
        encoder->update();
        menu->update();
    }
    
    // Petite pause pour économiser de l'énergie
    delay(10);
}