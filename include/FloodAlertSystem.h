#ifndef FLOOD_ALERT_SYSTEM_H
#define FLOOD_ALERT_SYSTEM_H

#include "Config.h"
#include "network/FloodAlertNetwork.h"
#include "FloodAlertWebServer.h"
#include "sensors/SensorBase.h"
#include "indicators/LEDAlertIndicator.h"
#include "indicators/BuzzerAlertIndicator.h"
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
    
    // Silencer l'alerte sonore
    void silenceAudioAlert();
    
    // Traitement des données
    void processReceivedMessage(const network_message_t& msg, const uint8_t* mac);
    void processReceivedData(float* data, uint8_t count);
    
    // Accès aux composants principaux
    FloodAlertNetwork& getNetwork() { return _network; }
    FloodAlertWebServer& getWebServer() { return _webServer; }
    
private:
    bool _isMaster;
    bool _isRunning = false;
    FloodAlertNetwork _network;
    FloodAlertWebServer _webServer;
    std::vector<SensorBase*> _sensors;
    LEDAlertIndicator* _ledIndicator = nullptr;
    BuzzerAlertIndicator* _buzzerIndicator = nullptr;
    unsigned long _lastStatusUpdate = 0;

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
};

#endif // FLOOD_ALERT_SYSTEM_H