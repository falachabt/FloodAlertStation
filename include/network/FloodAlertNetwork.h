#ifndef FLOOD_ALERT_NETWORK_H
#define FLOOD_ALERT_NETWORK_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// Maximum number of peers this device can connect to
#define MAX_PEERS 20

// Message types for different communication purposes
enum MessageType {
    DISCOVERY = 1,     // Device announcing itself to the network
    SENSOR_DATA = 2,   // Sensor data from slave to master
    ALERT = 3,         // Alert message from master to slaves
    STATUS_UPDATE = 4, // Regular status update from master to slaves
    PING = 5,          // Keepalive message to verify connection
    COMMAND = 6        // Command from master to specific slave
};

// Structure for messages transmitted over ESP-NOW
typedef struct {
    uint8_t type;             // Message type (see MessageType enum)
    uint8_t sender_id[6];     // MAC address of sender
    uint32_t message_id;      // Sequential message ID
    uint8_t is_master;        // Flag to indicate if sender is master
    float data[5];            // Array for sensor data/parameters
    uint8_t data_count;       // Number of valid data points
    char text[32];            // Text field for messages/alerts
    uint8_t alert_level;      // Alert level (0-3, with 0 being normal)
    uint8_t battery_level;    // Battery level percentage
    bool ready;               // Flag to indicate device is ready
} network_message_t;

// Peer information structure
struct PeerInfo {
    esp_now_peer_info_t peer_info;
    bool is_master;
    bool is_ready;
    uint32_t last_seen;
    uint8_t retry_count;
    bool in_use;              // Flag to indicate if this slot is used
    
    PeerInfo() : is_master(false), is_ready(false), last_seen(0), retry_count(0), in_use(false) {
        memset(&peer_info, 0, sizeof(esp_now_peer_info_t));
    }
};

class FloodAlertNetwork {
public:
    // Function pointer types for callbacks
    typedef void (*MessageCallback)(const network_message_t&, const uint8_t*);
    typedef void (*DeliveryCallback)(bool, const uint8_t*);
    typedef void (*DataReadyCallback)(float*, uint8_t);

    // Constructor
    FloodAlertNetwork();
    
    // Initialization
    bool begin(bool is_master, uint8_t min_peers = 1, uint8_t channel = 1);
    
    // Set the maximum transmit power (optional)
    void setMaxTxPower();
    
    // Set callbacks
    void onMessageReceived(MessageCallback callback);
    void onDeliveryResult(DeliveryCallback callback);
    void onDataReady(DataReadyCallback callback);
    
    // Send messages
    bool sendToMaster(const float* data, uint8_t data_count, const char* text = nullptr);
    bool sendToAllSlaves(const float* data, uint8_t data_count, uint8_t alert_level = 0, const char* text = nullptr);
    bool sendToSlave(const uint8_t* mac_addr, const float* data, uint8_t data_count, const char* text = nullptr);
    
    // Network management
    void broadcastDiscovery();
    bool isPeerMaster(const uint8_t* mac_addr);
    bool isNetworkReady();
    bool isConnectedToMaster();
    uint8_t getPeerCount();
    uint8_t getMinPeers();
    void setMinPeers(uint8_t min_peers);
    
    // Process network tasks (call this in loop())
    void update();
    
    // Debug
    void printNetworkStatus();
    void printPeers();
    
    // Set device name for identification
    void setDeviceName(const char* name);
    
    // Get master MAC address if known
    bool getMasterMac(uint8_t* mac_out);
    
    // Get own MAC address
    void getOwnMac(uint8_t* mac_out);

private:
    bool _initialized;
    bool _is_master;
    uint8_t _channel;
    uint8_t _own_mac[6];
    uint8_t _master_mac[6];
    bool _master_found;
    uint8_t _min_peers;
    uint32_t _message_counter;
    
    // Network status
    PeerInfo _peers[MAX_PEERS];
    uint8_t _peer_count;
    uint32_t _last_discovery;
    uint32_t _last_status_send;
    uint32_t _network_ready_time;
    
    // Device identification
    char _device_name[16];
    
    // Callbacks
    MessageCallback _message_callback;
    DeliveryCallback _delivery_callback;
    DataReadyCallback _data_ready_callback;
    
    // Internal methods
    void _processPeerDiscovery(const network_message_t& msg, const uint8_t* mac_addr);
    bool _addPeer(const uint8_t* mac_addr, bool is_master);
    bool _removePeer(const uint8_t* mac_addr);
    bool _sendMessage(const uint8_t* mac_addr, network_message_t& msg);
    
    // Helper methods
    int _findPeerIndex(const uint8_t* mac_addr);
    int _findFreePeerSlot();
    bool _compareMac(const uint8_t* mac1, const uint8_t* mac2);
    
    // Static callback handlers
    static void _onReceiveHandler(const uint8_t* mac_addr, const uint8_t* data, int data_len);
    static void _onSendHandler(const uint8_t* mac_addr, esp_now_send_status_t status);
    
    // Static instance pointer for callbacks
    static FloodAlertNetwork* _instance;
};

#endif // FLOOD_ALERT_NETWORK_H