#ifndef FLOOD_ALERT_WEB_SERVER_H
#define FLOOD_ALERT_WEB_SERVER_H

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <DNSServer.h>

class FloodAlertWebServer {
public:
    // Constructor with default port
    FloodAlertWebServer(int port = 80) : server(port), dnsServer(NULL), captivePortalEnabled(false) {}
    
    // Destructor
    ~FloodAlertWebServer() {
        if (dnsServer != NULL) {
            delete dnsServer;
        }
    }
    
    // Initialize SPIFFS for serving files
    bool beginSPIFFS() {
        if (!SPIFFS.begin(true)) {
            Serial.println("Failed to mount SPIFFS");
            return false;
        }
        Serial.println("SPIFFS mounted successfully");
        return true;
    }
    
    // Setup Access Point mode
    bool beginAP(const char* ssid, const char* password = NULL, int channel = 1) {
        WiFi.softAP(ssid, password, channel);  // Specify channel here
        apIP = WiFi.softAPIP();
        
        Serial.print("AP started on channel ");
        Serial.print(channel);
        Serial.print(" with IP: ");
        Serial.println(apIP.toString());
        
        return true;
    }
    
    // Setup Station mode (connect to existing WiFi)
    bool beginSTA(const char* ssid, const char* password) {
        WiFi.begin(ssid, password);
        Serial.print("Connecting to WiFi");
        
        // Wait for connection (timeout after 20 seconds)
        int timeout = 20;
        while (WiFi.status() != WL_CONNECTED && timeout > 0) {
            delay(1000);
            Serial.print(".");
            timeout--;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            staIP = WiFi.localIP();
            Serial.print("\nConnected to WiFi. IP: ");
            Serial.println(staIP.toString());
            return true;
        } else {
            Serial.println("\nFailed to connect to WiFi");
            return false;
        }
    }
    
    // Enable both AP and STA modes
    bool beginAPSTA(const char* apSSID, const char* apPassword,
                    const char* staSSID, const char* staPassword) {
        
        // Set WiFi mode for both AP and STA
        WiFi.mode(WIFI_AP_STA);
        
        // Start AP
        bool apResult = beginAP(apSSID, apPassword);
        
        // Connect to existing WiFi
        bool staResult = beginSTA(staSSID, staPassword);
        
        return apResult; // Return AP result as minimum requirement
    }
    
    // Enable captive portal for AP mode
    void enableCaptivePortal() {
        if (dnsServer == NULL) {
            dnsServer = new DNSServer();
            dnsServer->start(53, "*", apIP);
            captivePortalEnabled = true;
            
            // Add handler for captive portal
            server.on("/generate_204", [this](){ redirectToRoot(); });  // Android captive portal
            server.on("/fwlink", [this](){ redirectToRoot(); });  // Microsoft captive portal
            
            Serial.println("Captive portal enabled");
        }
    }
    
    // Start the web server
    void begin() {
        // Add a default handler for root if none was specified
        bool hasRootHandler = false;
        for (const auto& uri : registeredUris) {
            if (uri == "/") {
                hasRootHandler = true;
                break;
            }
        }
        
        if (!hasRootHandler) {
            Serial.println("Adding default root handler");
            
            // Add a default handler
            on("/", HTTP_GET, [this]() {
                server.send(200, "text/html", 
                    "<html><body><h1>Alert Station</h1>"
                    "<p>The web server is running.</p></body></html>");
            });
        }
        
        // Add 404 handler if not already added
        server.onNotFound([this]() {
            if (captivePortalEnabled) {
                redirectToRoot();
            } else {
                server.send(404, "text/plain", "404: Not found");
            }
        });
        
        // Start the server
        server.begin();
        Serial.println("Web server started");
    }
    
    // Process captive portal DNS and web server requests
    void handleClient() {
        if (captivePortalEnabled && dnsServer != NULL) {
            dnsServer->processNextRequest();
        }
        server.handleClient();
    }
    
    // Add a handler for a specific URI and HTTP method
    void on(const String &uri, HTTPMethod method, WebServer::THandlerFunction handler) {
        server.on(uri, method, handler);
        registeredUris.push_back(uri);
    }
    
    // Add a handler for a specific URI with any HTTP method
    void on(const String &uri, WebServer::THandlerFunction handler) {
        server.on(uri, handler);
        registeredUris.push_back(uri);
    }
    
    // Serve a file from SPIFFS
    bool serveFile(const String &path, const String &contentType = "") {
        if (SPIFFS.exists(path)) {
            File file = SPIFFS.open(path, "r");
            if (contentType.length() > 0) {
                server.streamFile(file, contentType);
            } else {
                server.streamFile(file, getContentType(path));
            }
            file.close();
            return true;
        }
        return false;
    }
    
    // Get direct access to the WebServer object
    WebServer& getServer() {
        return server;
    }
    
    // Get IP addresses
    IPAddress getAPIP() { return apIP; }
    IPAddress getSTAIP() { return staIP; }
    
    // Check if connected to WiFi
    bool isConnectedToWiFi() {
        return WiFi.status() == WL_CONNECTED;
    }

private:
    WebServer server;
    DNSServer* dnsServer;
    bool captivePortalEnabled;
    IPAddress apIP;
    IPAddress staIP;
    std::vector<String> registeredUris;  // Keep track of registered URIs
    
    // Redirect to root page (for captive portal)
    void redirectToRoot() {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    }
    
    // Get content type based on file extension
    String getContentType(const String &path) {
        if (path.endsWith(".html")) return "text/html";
        else if (path.endsWith(".css")) return "text/css";
        else if (path.endsWith(".js")) return "application/javascript";
        else if (path.endsWith(".json")) return "application/json";
        else if (path.endsWith(".png")) return "image/png";
        else if (path.endsWith(".jpg")) return "image/jpeg";
        else if (path.endsWith(".gif")) return "image/gif";
        else if (path.endsWith(".ico")) return "image/x-icon";
        else if (path.endsWith(".svg")) return "image/svg+xml";
        return "text/plain";
    }
};

#endif // FLOOD_ALERT_WEB_SERVER_H