/*
    AlertStation.ino

    Main sketch for the Flood Alert System's Alert Station.
    Combines the FloodAlertNetwork and FloodAlertWebServer modules.

    This sketch:
    1. Initializes ESP-NOW communication with sensors via FloodAlertNetwork
    2. Sets up a web server via FloodAlertWebServer
    3. Updates and maintains both systems
    4. Handles the integration between them
*/

#include "FloodAlertNetwork.h"
#include "FloodAlertWebServer.h"
#include <ArduinoJson.h>
#include <vector>

// ===== CONFIGURATION =====

// Network Configuration
#define DEVICE_NAME "AlertStation" // Name for device identification
#define MIN_PEERS 1                // Minimum sensors needed
#define WIFI_CHANNEL 1             // WiFi channel for ESP-NOW

// WiFi Configuration
#define AP_SSID "FloodAlertSystem"  // Access Point SSID
#define AP_PASSWORD "floodalert123" // Access Point Password (min 8 chars)

// Uncomment and set these to connect to an existing WiFi network
// #define WIFI_SSID "YourWiFiNetwork"    // Your WiFi network SSID
// #define WIFI_PASSWORD "YourWiFiPass"   // Your WiFi network password

// ===== GLOBAL VARIABLES =====

FloodAlertNetwork network;          // ESP-NOW Network
FloodAlertWebServer webServer;      // Web Server
unsigned long lastStatusUpdate = 0; // Last time status was printed

// Sensor data storage for web interface
#define MAX_SENSORS 10

struct SensorData
{
  uint8_t mac[6];    // MAC address
  char name[16];     // Device name
  float waterLevel;  // Current water level
  float temperature; // Current temperature
  uint8_t category;  // Water category level
  uint32_t lastSeen; // Last time data was received
  bool active;       // Is this sensor active
};

SensorData sensors[MAX_SENSORS];
uint8_t sensorCount = 0;
uint8_t ownMac[6];

// ===== FUNCTION PROTOTYPES =====
void setupWebServer();
void handleSensorData(const float *data, uint8_t count, const uint8_t *mac, const char *sensorName);
void updateInactiveSensors();
int findSensorIndex(const uint8_t *mac);
void printNetworkStatus();

// ===== ESP-NOW CALLBACKS =====

// Process received data from sensors
void onDataReady(float *data, uint8_t count)
{
  // The data is processed along with metadata in onMessageReceived
}

// Handle received messages including metadata
void onMessageReceived(const network_message_t &msg, const uint8_t *mac)
{
  if (msg.type == SENSOR_DATA)
  {
    handleSensorData(msg.data, msg.data_count, mac, msg.text);
  }
}

// ===== SETUP =====

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000); // Give time for Serial to connect

  Serial.println("\n\n=== FLOOD ALERT SYSTEM - ALERT STATION ===");

  // Initialize the ESP-NOW network
  network.setDeviceName(DEVICE_NAME);

  if (network.begin(true, MIN_PEERS, WIFI_CHANNEL))
  {
    Serial.println("✓ Network initialized successfully as MASTER");

    // Register callbacks
    network.onMessageReceived(onMessageReceived);
    network.onDataReady(onDataReady);
  }
  else
  {
    Serial.println("✗ Failed to initialize network!");
  }

  // Initialize the web server
  if (webServer.beginSPIFFS())
  {
    // Set up web server (AP mode only or AP+STA mode)
    #ifdef WIFI_SSID
        webServer.beginAPSTA(AP_SSID, AP_PASSWORD, WIFI_SSID, WIFI_PASSWORD);
    #else
        webServer.beginAP(AP_SSID, AP_PASSWORD);
    #endif

    // Enable captive portal to redirect to our web interface
    webServer.enableCaptivePortal();

    // Set up web server endpoints
    setupWebServer();

    // Start web server
    webServer.begin();
  }
  else
  {
    Serial.println("✗ Failed to initialize web server file system!");
  }

  // Initialize sensor data array
  for (int i = 0; i < MAX_SENSORS; i++)
  {
    memset(&sensors[i], 0, sizeof(SensorData));
    sensors[i].active = false;
  }

  Serial.println("Setup complete!");
}

// ===== LOOP =====

void loop()
{
  // Process network events
  network.update();

  // Process web server requests
  webServer.handleClient();

  // Check for inactive sensors
  updateInactiveSensors();

  // Print status periodically
  unsigned long now = millis();
  if (now - lastStatusUpdate >= 5000)
  { // Every 5 seconds
    lastStatusUpdate = now;
    printNetworkStatus();
    network.getOwnMac(ownMac);
    Serial.print("Own MAC: ");
    for (int i = 0; i < 6; i++) {
        Serial.print(ownMac[i], HEX);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
  }
}

// ===== WEB SERVER SETUP =====

void setupWebServer()
{

  // API endpoint for sensor data
  webServer.on("/api/sensors", HTTP_GET, []()
               {
        DynamicJsonDocument doc(2048);
        JsonArray sensorArray = doc.createNestedArray("sensors");
        
        for (int i = 0; i < MAX_SENSORS; i++) {
            if (sensors[i].active) {
                JsonObject sensor = sensorArray.createNestedObject();
                sensor["name"] = sensors[i].name;
                
                // Format MAC address
                char macStr[18];
                snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                        sensors[i].mac[0], sensors[i].mac[1], sensors[i].mac[2],
                        sensors[i].mac[3], sensors[i].mac[4], sensors[i].mac[5]);
                sensor["mac"] = macStr;
                
                sensor["waterLevel"] = sensors[i].waterLevel;
                sensor["temperature"] = sensors[i].temperature;
                sensor["category"] = sensors[i].category;
                
                // Calculate time since last seen
                unsigned long secsSinceLastSeen = (millis() - sensors[i].lastSeen) / 1000;
                sensor["lastSeenSeconds"] = secsSinceLastSeen;
                
                // Category text
                switch(sensors[i].category) {
                    case 0: sensor["status"] = "Normal"; break;
                    case 1: sensor["status"] = "Warning"; break;
                    case 2: sensor["status"] = "Alert"; break;
                    default: sensor["status"] = "Unknown";
                }
            }
        }
        
        // Add network status
        doc["networkReady"] = network.isNetworkReady();
        doc["connectedPeers"] = network.getPeerCount();
        doc["timestamp"] = millis() / 1000;
        
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        webServer.getServer().send(200, "application/json", jsonResponse); });

  // System status API
  webServer.on("/api/status", HTTP_GET, []()
               {
        DynamicJsonDocument doc(512);
        
        // Device info
        uint8_t mac[6];
        network.getOwnMac(mac);
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        
        doc["deviceName"] = DEVICE_NAME;
        doc["deviceMac"] = macStr;
        doc["uptime"] = millis() / 1000;
        
        // Network info
        doc["networkReady"] = network.isNetworkReady();
        doc["connectedPeers"] = network.getPeerCount();
        doc["minPeers"] = network.getMinPeers();
        
        // WiFi info
        JsonObject wifiInfo = doc.createNestedObject("wifi");
        wifiInfo["apIP"] = webServer.getAPIP().toString();
        wifiInfo["apSSID"] = AP_SSID;
        
        wifiInfo["staConnected"] = webServer.isConnectedToWiFi();
        if (webServer.isConnectedToWiFi()) {
            wifiInfo["staIP"] = webServer.getSTAIP().toString();
#ifdef WIFI_SSID
            wifiInfo["staSSID"] = WIFI_SSID;
#endif
            wifiInfo["rssi"] = WiFi.RSSI();
        }
        
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        webServer.getServer().send(200, "application/json", jsonResponse); });

  // Static files
  // Static files handler - this will catch any request for static files
  webServer.on("/static", HTTP_GET, []()
               {
    // Get the full URI (e.g., /static/css/style.css)
    String uri = webServer.getServer().uri();
    
    // Try to serve the exact file path from SPIFFS
    if (webServer.serveFile(uri)) {
        Serial.println("Served file: " + uri);
        return;
    }
    
    // If the file wasn't found at that exact path, try prepending a slash
    // Some browsers might request /static/style.css but the actual path is /static/style.css
    if (uri.indexOf("/static/") == 0) {
        // Extract the filename without /static prefix
        String filename = uri.substring(7); // Remove "/static"
        if (webServer.serveFile(filename)) {
            Serial.println("Served file with modified path: " + filename);
            return;
        }
    }
    
    // File not found
    Serial.println("File not found: " + uri);
    webServer.getServer().send(404, "text/plain", "File not found"); });
}

// ===== HELPER FUNCTIONS =====

// Process sensor data and update records
void handleSensorData(const float *data, uint8_t count, const uint8_t *mac, const char *sensorName)
{
  int idx = findSensorIndex(mac);

  // If not found, create new entry
  if (idx < 0)
  {
    for (int i = 0; i < MAX_SENSORS; i++)
    {
      if (!sensors[i].active)
      {
        idx = i;
        sensorCount++;
        break;
      }
    }

    // If no free slots, return
    if (idx < 0)
    {
      Serial.println("No free slots for new sensor data");
      return;
    }

    // Initialize new sensor
    memcpy(sensors[idx].mac, mac, 6);
    sensors[idx].active = true;

    Serial.print("New sensor connected: ");
    Serial.println(sensorName);
  }

  // Update sensor data
  strncpy(sensors[idx].name, sensorName, sizeof(sensors[idx].name) - 1);
  if (count >= 1)
    sensors[idx].waterLevel = data[0];
  if (count >= 2)
    sensors[idx].temperature = data[1];
  if (count >= 3)
    sensors[idx].category = (uint8_t)data[2];
  sensors[idx].lastSeen = millis();

  // Print data for debugging
  Serial.print("Received from ");
  Serial.print(sensors[idx].name);
  Serial.print(": Water=");
  Serial.print(sensors[idx].waterLevel);
  Serial.print("cm, Temp=");
  Serial.print(sensors[idx].temperature);
  Serial.print("°C, Category=");
  Serial.println(sensors[idx].category);
}

// Find a sensor by MAC address
int findSensorIndex(const uint8_t *mac)
{
  for (int i = 0; i < MAX_SENSORS; i++)
  {
    if (sensors[i].active && memcmp(sensors[i].mac, mac, 6) == 0)
    {
      return i;
    }
  }
  return -1;
}

// Check for and update inactive sensors
void updateInactiveSensors()
{
  unsigned long now = millis();
  for (int i = 0; i < MAX_SENSORS; i++)
  {
    if (sensors[i].active && (now - sensors[i].lastSeen > 30000))
    { // 30 seconds timeout
      Serial.print("Sensor disconnected: ");
      Serial.println(sensors[i].name);
      sensors[i].active = false;
      sensorCount--;
    }
  }
}

// Print network status to serial
void printNetworkStatus()
{
  Serial.println("\n--- NETWORK STATUS ---");
  Serial.print("Connected sensors: ");
  Serial.print(network.getPeerCount());
  Serial.print("/");
  Serial.println(MIN_PEERS);

  Serial.print("Network ready: ");
  Serial.println(network.isNetworkReady() ? "YES" : "NO");

  Serial.print("Active sensors in memory: ");
  Serial.println(sensorCount);

  // Print WiFi info
  Serial.print("AP IP: ");
  Serial.println(webServer.getAPIP().toString());

  if (webServer.isConnectedToWiFi())
  {
    Serial.print("WiFi IP: ");
    Serial.println(webServer.getSTAIP().toString());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }
  else
  {
    Serial.println("Not connected to WiFi network");
  }

  Serial.println("----------------------");
}