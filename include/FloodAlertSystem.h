#ifndef FLOOD_ALERT_SYSTEM_H
#define FLOOD_ALERT_SYSTEM_H

#include "Config.h"
#include "network/FloodAlertNetwork.h"
#include "FloodAlertWebServer.h"
#include "sensors/SensorBase.h"
#include "indicators/LEDAlertIndicator.h"
#include "indicators/BuzzerAlertIndicator.h"
#include "indicators/ToggleSwitchIndicator.h" // Include toggle switch header
#include "indicators/EInkDisplay.h" // Add E-Ink display header
#include <vector>

// Structure pour stocker les données des capteurs
struct SensorData {
    uint8_t mac[6];    // Adresse MAC
    char name[16];     // Nom du périphérique
    float waterLevel;  // Niveau d'eau actuel
    float temperature; // Température actuelle
    uint8_t category;  // Catégorie d'alerte
    uint32_t lastSeen; // Dernière fois où les données ont été reçues
    bool active;       // Ce capteur est-il actif
};

// Classe principale du système
class FloodAlertSystem {
public:
    FloodAlertSystem();
    ~FloodAlertSystem();
    
    bool begin(bool isMaster);
    void update();
    
    // Gestion des capteurs
    void addSensor(SensorBase* sensor);
    void removeSensor(SensorBase* sensor);
    
    // Gestion des indicateurs
    void setLEDIndicator(LEDAlertIndicator* ledIndicator);
    void setBuzzerIndicator(BuzzerAlertIndicator* buzzerIndicator);
    void setEInkDisplay(EInkDisplay* einkDisplay); // Add E-Ink display setter
    
    // Accès aux indicateurs (ajout pour le système de menu)
    BuzzerAlertIndicator* getBuzzerIndicator() { return _buzzerIndicator; }
    LEDAlertIndicator* getLEDIndicator() { return _ledIndicator; }
    EInkDisplay* getEInkDisplay() { return _einkDisplay; }
    
    // Silencer l'alerte sonore
    void silenceAudioAlert();
    
    // Traitement des données
    void processReceivedMessage(const network_message_t& msg, const uint8_t* mac);
    void processReceivedData(float* data, uint8_t count);
    
    //  Toogle switch management
    void setToggleSwitchIndicator(ToggleSwitchIndicator* toggleSwitchIndicator);
    ToggleSwitchIndicator* getToggleSwitchIndicator() { return _toggleSwitchIndicator; }

    // Accès aux composants principaux
    FloodAlertNetwork& getNetwork() { return _network; }
    FloodAlertWebServer& getWebServer() { return _webServer; }
    
    // Accès aux données des capteurs (pour l'affichage)
    SensorData* getSensorData(int index);
    int getSensorCount();
    float getHighestWaterLevel();
    float getAverageTemperature();
    uint8_t getHighestAlertCategory();
    
private:
    bool _isMaster;
    bool _isRunning = false;
    FloodAlertNetwork _network;
    FloodAlertWebServer _webServer;
    std::vector<SensorBase*> _sensors;
    LEDAlertIndicator* _ledIndicator = nullptr;
    BuzzerAlertIndicator* _buzzerIndicator = nullptr;
    ToggleSwitchIndicator* _toggleSwitchIndicator = nullptr;
    EInkDisplay* _einkDisplay = nullptr; // Add E-Ink display pointer
    unsigned long _lastStatusUpdate = 0;
    unsigned long _lastEInkUpdate = 0; // Track last E-Ink update time

    // Liste des capteurs distants
    SensorData _remoteSensors[MAX_SENSORS];
    uint8_t _remoteSensorCount = 0;

    // Gestion du réseau et des capteurs
    void setupWebServer();
    void processLocalSensors();
    void updateInactiveSensors();
    void sendSensorData();
    void handleSensorData(const float* data, uint8_t count, const uint8_t* mac, const char* sensorName);
    void updateIndicators(float waterLevel, uint8_t category);
    
    // Traitement des commandes série
    void processSerialCommands();
    
    // Instance singleton pour les callbacks
    static FloodAlertSystem* _instance;
    
    // Callbacks statiques pour FloodAlertNetwork
    static void onMessageReceived(const network_message_t& msg, const uint8_t* mac);
    static void onDataReady(float* data, uint8_t count);
    
    // Added: Update E-Ink display with current system state
    void updateEInkDisplay();
};

#endif // FLOOD_ALERT_SYSTEM_H