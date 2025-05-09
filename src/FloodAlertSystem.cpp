#include "FloodAlertSystem.h"
#include <ArduinoJson.h>

// Initialisation du pointeur statique
FloodAlertSystem *FloodAlertSystem::_instance = nullptr;

FloodAlertSystem::FloodAlertSystem()
{
    _instance = this;
    _ledIndicator = nullptr;
    _buzzerIndicator = nullptr;

    // Initialiser les capteurs distants
    for (int i = 0; i < MAX_SENSORS; i++)
    {
        memset(&_remoteSensors[i], 0, sizeof(SensorData));
        _remoteSensors[i].active = false;
    }
}

FloodAlertSystem::~FloodAlertSystem()
{
    // Libérer les capteurs
    for (auto sensor : _sensors)
    {
        delete sensor;
    }
    _sensors.clear();

    _instance = nullptr;
}

bool FloodAlertSystem::begin(bool isMaster)
{
    _isMaster = isMaster;

    // Initialiser le réseau ESP-NOW
    _network.setDeviceName(_isMaster ? DEVICE_NAME : SLAVE_NAME);
    if (!_network.begin(_isMaster, MIN_PEERS, WIFI_CHANNEL))
    {
        Serial.println("✗ Failed to initialize network!");
        return false;
    }

    // Définir les callbacks
    _network.onMessageReceived(onMessageReceived);
    _network.onDataReady(onDataReady);

    // Initialiser le serveur web si c'est le master
    if (_isMaster)
    {
        if (!_webServer.beginSPIFFS())
        {
            Serial.println("✗ Failed to initialize web server file system!");
            return false;
        }

        // Configurer le WiFi selon le mode
        _webServer.beginAP(AP_SSID, AP_PASSWORD);

        // Activer le portail captif
        _webServer.enableCaptivePortal();

        // Configurer les endpoints du serveur web
        setupWebServer();

        // Démarrer le serveur web
        _webServer.begin();
    }

    // Initialiser tous les capteurs
    for (auto sensor : _sensors)
    {
        if (!sensor->begin())
        {
            Serial.print("Failed to initialize sensor: ");
            Serial.println(sensor->getName());
        }
    }

    _isRunning = true;
    return true;
}

// Configuration du serveur web
void FloodAlertSystem::setupWebServer()
{
    // API endpoint for sensor data
    _webServer.on("/api/sensors", HTTP_GET, [this]()
                  {
        DynamicJsonDocument doc(2048);
        JsonArray sensorArray = doc.createNestedArray("sensors");
        
        for (int i = 0; i < MAX_SENSORS; i++) {
            if (_remoteSensors[i].active) {
                JsonObject sensor = sensorArray.createNestedObject();
                sensor["name"] = _remoteSensors[i].name;
                
                // Format MAC address
                char macStr[18];
                snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                        _remoteSensors[i].mac[0], _remoteSensors[i].mac[1], _remoteSensors[i].mac[2],
                        _remoteSensors[i].mac[3], _remoteSensors[i].mac[4], _remoteSensors[i].mac[5]);
                sensor["mac"] = macStr;
                
                sensor["waterLevel"] = _remoteSensors[i].waterLevel;
                sensor["temperature"] = _remoteSensors[i].temperature;
                sensor["category"] = _remoteSensors[i].category;
                
                // Calculate time since last seen
                unsigned long secsSinceLastSeen = (millis() - _remoteSensors[i].lastSeen) / 1000;
                sensor["lastSeenSeconds"] = secsSinceLastSeen;
                
                // Category text
                switch(_remoteSensors[i].category) {
                    case 0: sensor["status"] = "Normal"; break;
                    case 1: sensor["status"] = "Warning"; break;
                    case 2: sensor["status"] = "Alert"; break;
                    default: sensor["status"] = "Unknown";
                }
            }
        }
        
        // Add network status
        doc["networkReady"] = _network.isNetworkReady();
        doc["connectedPeers"] = _network.getPeerCount();
        doc["timestamp"] = millis() / 1000;
        
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        _webServer.getServer().send(200, "application/json", jsonResponse); });

    // System status API
    _webServer.on("/api/status", HTTP_GET, [this]()
                  {
        DynamicJsonDocument doc(512);
        
        // Device info
        uint8_t mac[6];
        _network.getOwnMac(mac);
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        
        doc["deviceName"] = DEVICE_NAME;
        doc["deviceMac"] = macStr;
        doc["uptime"] = millis() / 1000;
        
        // Network info
        doc["networkReady"] = _network.isNetworkReady();
        doc["connectedPeers"] = _network.getPeerCount();
        doc["minPeers"] = _network.getMinPeers();
        
        // WiFi info
        JsonObject wifiInfo = doc.createNestedObject("wifi");
        wifiInfo["apIP"] = _webServer.getAPIP().toString();
        wifiInfo["apSSID"] = AP_SSID;
        
        wifiInfo["staConnected"] = _webServer.isConnectedToWiFi();
        if (_webServer.isConnectedToWiFi()) {
            wifiInfo["staIP"] = _webServer.getSTAIP().toString();
            wifiInfo["rssi"] = WiFi.RSSI();
        }
        
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        _webServer.getServer().send(200, "application/json", jsonResponse); });

    // Root URL handler
    _webServer.on("/", HTTP_GET, [this]()
                  {
        if (_webServer.serveFile("/static/index.html")) {
            Serial.println("Served dashboard page");
        } else {
            _webServer.getServer().send(200, "text/html", "<h1>Flood Alert System</h1><p>Dashboard not found. Please upload files to SPIFFS.</p>");
        } });

    // Simple static file handler
    _webServer.on("/static/(.*)", HTTP_GET, [this]()
                  {
        String requestPath = _webServer.getServer().uri();
        if (_webServer.serveFile(requestPath)) {
            Serial.println("Served file: " + requestPath);
        } else {
            _webServer.getServer().send(404, "text/plain", "File not found");
        } });

    // API endpoint for silencing audio alerts
    _webServer.on("/api/silence", HTTP_POST, [this]()
                  {
        silenceAudioAlert();
        DynamicJsonDocument doc(128);
        doc["success"] = true;
        doc["message"] = "Audio alert silenced";
        
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        _webServer.getServer().send(200, "application/json", jsonResponse); });
}

// Ajouter un capteur à la liste
void FloodAlertSystem::addSensor(SensorBase *sensor)
{
    _sensors.push_back(sensor);

    // Initialiser le capteur si le système est déjà en cours d'exécution
    if (_isRunning)
    {
        sensor->begin();
    }

    Serial.print("Added sensor: ");
    Serial.println(sensor->getName());
}

void FloodAlertSystem::update()
{
    // Mettre à jour le réseau
    _network.update();

    // Mettre à jour le serveur web si c'est le master
    if (_isMaster)
    {
        _webServer.handleClient();

        

        // toogle switch only for master
        // Handle toggle switch if available
        if (_toggleSwitchIndicator != nullptr)
        {

            if (_toggleSwitchIndicator->update())
            {
                // Toggle state changed
                if (_toggleSwitchIndicator->isToggleOn())
                {
                    // Toggle switched ON - trigger alert sounds
                    if (_buzzerIndicator != nullptr)
                    {
                        _buzzerIndicator->playAlertTone(1);
                    }
                }
                else if (_toggleSwitchIndicator->isToggleOff())
                {
                    // Toggle switched OFF - silence alerts
                    if (_buzzerIndicator != nullptr)
                    {
                        _buzzerIndicator->silenceAlert();
                    }
                }
            }

            // If toggle is ON, periodically play buzzer
            if (_toggleSwitchIndicator->getSwitchState())
            {
                static unsigned long lastBuzzerTime = 0;
                unsigned long currentTime = millis();

                if (currentTime - lastBuzzerTime > 3000)
                { // Every 3 seconds
                    lastBuzzerTime = currentTime;
                    if (_buzzerIndicator != nullptr)
                    {
                        _buzzerIndicator->playAlertTone(1);
                    }
                }
            }
        }
    }

    // Mettre à jour les capteurs locaux
    processLocalSensors();

    // Vérifier les capteurs inactifs
    updateInactiveSensors();

    // Vérifier les commandes série
    processSerialCommands();

    // Mettre à jour le buzzer (pour gérer les motifs d'alerte)
    if (_buzzerIndicator != nullptr)
    {
        _buzzerIndicator->tick();
    }

    updateEInkDisplay();

    // Envoyer les données périodiquement
    unsigned long now = millis();
    if (now - _lastStatusUpdate >= 5000)
    { // Toutes les 5 secondes
        _lastStatusUpdate = now;

        // Envoyer les données des capteurs
        if (!_isMaster)
        {
            // Si esclave, envoyer les données au master
            sendSensorData();
        }

        // Imprimer l'état du réseau
        _network.printNetworkStatus();
    }
}

// Callbacks statiques
void FloodAlertSystem::onMessageReceived(const network_message_t &msg, const uint8_t *mac)
{
    if (_instance)
    {
        _instance->processReceivedMessage(msg, mac);
    }
}

void FloodAlertSystem::onDataReady(float *data, uint8_t count)
{
    if (_instance)
    {
        _instance->processReceivedData(data, count);
    }
}

// Traitement des messages reçus
void FloodAlertSystem::processReceivedMessage(const network_message_t &msg, const uint8_t *mac)
{
    if (msg.type == SENSOR_DATA)
    {
        // Traitement des données du capteur
        handleSensorData(msg.data, msg.data_count, mac, msg.text);
    }
}

// Traitement des données reçues
void FloodAlertSystem::processReceivedData(float *data, uint8_t count)
{
    // Cette méthode peut être vide car nous utilisons déjà processReceivedMessage
}

// Méthode pour traiter les données locales des capteurs
void FloodAlertSystem::processLocalSensors()
{
    // Mettre à jour tous les capteurs
    for (auto sensor : _sensors)
    {
        sensor->update();
    }

    // Si c'est un esclave, pas besoin de faire plus
    if (!_isMaster)
        return;

    // Si c'est un master, traiter les données des capteurs locaux
    // (comme le DHT11) et les ajouter à la liste des capteurs distants
    // pour qu'elles soient affichées sur l'interface web

    for (auto sensor : _sensors)
    {
        if (strcmp(sensor->getName(), "DHT11") == 0)
        {
            // Récupérer les données du DHT11
            float data[5];
            uint8_t count = 5;
            sensor->getData(data, count);

            // Créer une entrée "locale" dans le tableau des capteurs distants
            int idx = -1;
            for (int i = 0; i < MAX_SENSORS; i++)
            {
                if (!_remoteSensors[i].active)
                {
                    idx = i;
                    _remoteSensorCount++;
                    break;
                }
                else if (strcmp(_remoteSensors[i].name, "Local-DHT11") == 0)
                {
                    idx = i;
                    break;
                }
            }

            if (idx >= 0)
            {
                // Obtenir notre propre MAC
                uint8_t macAddr[6];
                _network.getOwnMac(macAddr);

                // Mettre à jour les données
                memcpy(_remoteSensors[idx].mac, macAddr, 6);
                strncpy(_remoteSensors[idx].name, "Local-DHT11", sizeof(_remoteSensors[idx].name) - 1);
                _remoteSensors[idx].temperature = data[1]; // Température
                _remoteSensors[idx].waterLevel = 0;        // Pas de niveau d'eau
                _remoteSensors[idx].category = data[2];    // Catégorie
                _remoteSensors[idx].lastSeen = millis();
                _remoteSensors[idx].active = true;
            }
        }
    }
}

// Vérifier et mettre à jour les capteurs inactifs
void FloodAlertSystem::updateInactiveSensors()
{
    unsigned long now = millis();
    for (int i = 0; i < MAX_SENSORS; i++)
    {
        if (_remoteSensors[i].active && (now - _remoteSensors[i].lastSeen > 30000))
        { // 30 secondes timeout
            Serial.print("Sensor disconnected: ");
            Serial.println(_remoteSensors[i].name);
            _remoteSensors[i].active = false;
            _remoteSensorCount--;
        }
    }
}

// Traiter les commandes reçues par le port série
void FloodAlertSystem::processSerialCommands()
{
    if (Serial.available() > 0)
    {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command.equalsIgnoreCase("silence") || command.equalsIgnoreCase("s"))
        {
            silenceAudioAlert();
            Serial.println("Command received: Silencing audio alert");
        }
        else if (command.equalsIgnoreCase("test") || command.equalsIgnoreCase("t"))
        {
            // Test the alerts
            if (_ledIndicator != nullptr)
            {
                _ledIndicator->blinkLED(LED_RED_PIN, 2);
                _ledIndicator->blinkLED(LED_YELLOW_PIN, 2);
                _ledIndicator->blinkLED(LED_GREEN_PIN, 2);
            }

            if (_buzzerIndicator != nullptr)
            {
                _buzzerIndicator->playWarningTone();
                delay(500);
                _buzzerIndicator->playAlertTone(0);
                delay(500);
                _buzzerIndicator->playSuccessTone();
            }

            Serial.println("Command received: Testing alerts");
        }
    }
}

// Envoyer les données des capteurs (pour les esclaves)
void FloodAlertSystem::sendSensorData()
{
    // Cette méthode n'est utilisée que par les esclaves
    if (_isMaster)
        return;

    // Préparer un tableau pour stocker les données de tous les capteurs
    float data[5]; // Maximum 5 valeurs pour la structure network_message_t
    uint8_t count = 5;

    // Réinitialiser les données
    for (int i = 0; i < 5; i++)
    {
        data[i] = 0;
    }

    // Si on a un capteur de niveau d'eau, obtenir ses données
    for (auto sensor : _sensors)
    {
        if (strcmp(sensor->getName(), "WaterLevel") == 0)
        {
            sensor->getData(data, count);

            // Print sensor data to Serial Monitor
            Serial.println("\n--- SLAVE SENSOR DATA ---");
            Serial.print("Water Level: ");
            Serial.print(data[0]);
            Serial.println(" cm");
            Serial.print("Category: ");
            Serial.println((int)data[2]); // 0=normal, 1=warning, 2=critical
            Serial.println("------------------------\n");
            break; // On ne prend que le premier capteur de niveau d'eau
        }
    }

    // Envoyer les données au master
    if (count > 0)
    {
        char deviceName[16];
        snprintf(deviceName, sizeof(deviceName), "%s", SLAVE_NAME);

        // Envoyer au master
        bool result = _network.sendToMaster(data, count, deviceName);

        if (result)
        {
            Serial.println("Sensor data sent to master successfully");
        }
        else
        {
            Serial.println("Failed to send sensor data to master");
        }
    }
}

// Traiter les données des capteurs reçues du réseau
void FloodAlertSystem::handleSensorData(const float *data, uint8_t count, const uint8_t *mac, const char *sensorName)
{
    // Find an existing slot or create a new one
    int idx = -1;

    // Look for the sensor by its MAC address
    for (int i = 0; i < MAX_SENSORS; i++)
    {
        if (_remoteSensors[i].active && memcmp(_remoteSensors[i].mac, mac, 6) == 0)
        {
            idx = i;
            break;
        }
    }

    // If not found, look for a free slot
    if (idx < 0)
    {
        for (int i = 0; i < MAX_SENSORS; i++)
        {
            if (!_remoteSensors[i].active)
            {
                idx = i;
                _remoteSensorCount++;
                break;
            }
        }
    }

    // If no slot available, quit
    if (idx < 0)
    {
        Serial.println("No free slots for new sensor data");
        return;
    }

    // Initialize or update sensor data
    memcpy(_remoteSensors[idx].mac, mac, 6);
    _remoteSensors[idx].active = true;

    // Update sensor name
    strncpy(_remoteSensors[idx].name, sensorName, sizeof(_remoteSensors[idx].name) - 1);

    // Update values according to expected format
    if (count >= 1)
        _remoteSensors[idx].waterLevel = data[0]; // First field = water level
    if (count >= 2)
        _remoteSensors[idx].temperature = data[1]; // Second field = temperature
    if (count >= 3)
        _remoteSensors[idx].category = (uint8_t)data[2]; // Third field = category

    _remoteSensors[idx].lastSeen = millis();

    // Update indicators based on water level data
    updateIndicators(_remoteSensors[idx].waterLevel, _remoteSensors[idx].category);

    // Debug output
    Serial.print("Received from ");
    Serial.print(_remoteSensors[idx].name);
    Serial.print(": Water=");
    Serial.print(_remoteSensors[idx].waterLevel);
    Serial.print("cm, Temp=");
    Serial.print(_remoteSensors[idx].temperature);
    Serial.print("°C, Category=");
    Serial.println(_remoteSensors[idx].category);

    if (count >= 2)
    {
        Serial.print("Temperature: ");
        Serial.print(_remoteSensors[idx].temperature);
        Serial.println("°C");
    }

    Serial.print("Category: ");
    Serial.print(_remoteSensors[idx].category);
    Serial.print(" (");
    Logger::infoF("nmmmmm  1073496478: %d", _remoteSensors[idx].name);
    switch (_remoteSensors[idx].category)
    {
    case 0:
        Serial.print("Normal");
      
        break;
    case 1:
       
        Serial.print("Warning");
        break;
    case 2:
        Serial.print("Alert");
        break;
    default:
        Serial.print("Unknown");
    }
    Serial.println(")");

    // Print formatted MAC address
    Serial.print("MAC Address: ");
    for (int i = 0; i < 6; i++)
    {
        Serial.print(_remoteSensors[idx].mac[i], HEX);
        if (i < 5)
            Serial.print(":");
    }
    Serial.println();

    Serial.println("-----------------------------------\n");
}

// Update both LED and Buzzer indicators with the same data
void FloodAlertSystem::updateIndicators(float waterLevel, uint8_t category)
{
    Serial.println("\n--- Updating Indicators ---");
    Serial.print("Water Level: ");
    Serial.print(waterLevel);
    Serial.print(" cm, Category: ");
    Serial.println(category);

    // Update LED indicator if available
    if (_ledIndicator != nullptr)
    {
        _ledIndicator->update(waterLevel, category);
    }
    else
    {
        Logger::info("LED Indicator not available");
    }

    // Update Buzzer indicator if available
    if (_buzzerIndicator != nullptr)
    {
        _buzzerIndicator->update(waterLevel, category);
        switch (category)
        {
        case 0:
            _ledIndicator->setRed(false);
            break;
            case 1: 
            _ledIndicator->setRed(false);
            break;
            case 2:
            _ledIndicator->setRed(true);
            break;
        }
    }
}

// Set the LED indicator
void FloodAlertSystem::setLEDIndicator(LEDAlertIndicator *ledIndicator)
{
    _ledIndicator = ledIndicator;
}

// Set the Buzzer indicator
void FloodAlertSystem::setBuzzerIndicator(BuzzerAlertIndicator *buzzerIndicator)
{
    _buzzerIndicator = buzzerIndicator;
}

// Silence the audio alert
void FloodAlertSystem::silenceAudioAlert()
{
    if (_buzzerIndicator != nullptr)
    {
        _buzzerIndicator->silenceAlert();
    }
}

// E-ink display management
// Set the E-Ink display
void FloodAlertSystem::setEInkDisplay(EInkDisplay *einkDisplay)
{
    _einkDisplay = einkDisplay;

    // Set reference to this system for the display
    if (_einkDisplay)
    {
        _einkDisplay->setFloodAlertSystem(this);
        Logger::info("E-Ink display registered with FloodAlertSystem");
    }
}

// New method to handle E-Ink display updates
void FloodAlertSystem::updateEInkDisplay()
{
    if (_einkDisplay != nullptr)
    {
        // Update the display
        _einkDisplay->update();

        // Periodic full refresh to prevent ghosting
        unsigned long now = millis();
        if (now - _lastEInkUpdate >= 3600000)
        { // Every hour
            _lastEInkUpdate = now;
            _einkDisplay->refresh(); // Force a full refresh
        }
    }
}

// Toggle switch management
// Toogle switch indicator set
void FloodAlertSystem::setToggleSwitchIndicator(ToggleSwitchIndicator *toggleSwitchIndicator)
{
    _toggleSwitchIndicator = toggleSwitchIndicator;
}