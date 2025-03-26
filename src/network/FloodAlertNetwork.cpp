#include "network/FloodAlertNetwork.h"
#include "utils/logger.h"

// Initialize static instance pointer
FloodAlertNetwork* FloodAlertNetwork::_instance = nullptr;

// Constructor implementation
FloodAlertNetwork::FloodAlertNetwork() 
    : _initialized(false), 
      _is_master(false), 
      _channel(1), 
      _master_found(false), 
      _min_peers(1), 
      _message_counter(0), 
      _peer_count(0),
      _last_discovery(0),
      _last_status_send(0),
      _network_ready_time(0) {
    
    memset(_own_mac, 0, 6);
    memset(_master_mac, 0, 6);
    memset(_device_name, 0, 16);
    strcpy(_device_name, "FloodDevice");
    
    for (int i = 0; i < MAX_PEERS; i++) {
        _peers[i] = PeerInfo();
    }
    
    // Set the singleton instance
    _instance = this;
}

// Set maximum transmit power
void FloodAlertNetwork::setMaxTxPower() {
    // Set ESP32 WiFi to max power
    // esp_wifi_set_max_tx_power(84); // 20.5dBm
}

// Initialize the ESP-NOW network
bool FloodAlertNetwork::begin(bool is_master, uint8_t min_peers, uint8_t channel) {
    _is_master = is_master;
    _min_peers = min_peers;
    _channel = channel;
    
    // Initialize WiFi in Station mode
    WiFi.mode(WIFI_STA);
    
    // Note: For this version of ESP32 Arduino core, we can't directly set the channel
    // The channel will be set when connecting to an AP or when scanning
    // For ESP-NOW, we'll rely on the channel set during the peer registration
    // Instead of trying to change it here
    
    // Get own MAC address
    WiFi.macAddress(_own_mac);
    
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Logger::error("Error initializing ESP-NOW");
        return false;
    }
    
    // Register callbacks - UPDATED for older ESP32 core
    esp_now_register_recv_cb(_onReceiveHandler);
    esp_now_register_send_cb(_onSendHandler);
    
    _initialized = true;
    
    // Serial.println("FloodAlertNetwork initialized");
    // Serial.print("Device role: ");
    // Serial.println(_is_master ? "MASTER" : "SLAVE");
    // Serial.print("MAC Address: ");
    // for (int i = 0; i < 6; i++) {
    //     Serial.print(_own_mac[i], HEX);
    //     if (i < 5) Serial.print(":");
    // }
    // Serial.println();
    
    // If slave, start discovery to find master
    if (!_is_master) {
        broadcastDiscovery();
    }
    
    return true;
}

// Set callback for message reception
void FloodAlertNetwork::onMessageReceived(MessageCallback callback) {
    _message_callback = callback;
}

// Set callback for delivery results
void FloodAlertNetwork::onDeliveryResult(DeliveryCallback callback) {
    _delivery_callback = callback;
}

// Set callback for when sensor data is ready to be processed
void FloodAlertNetwork::onDataReady(DataReadyCallback callback) {
    _data_ready_callback = callback;
}

// Send sensor data to the master device (for slave devices)
bool FloodAlertNetwork::sendToMaster(const float* data, uint8_t data_count, const char* text) {
    if (!_master_found) {
        // Serial.println("Master not found. Cannot send data.");
        return false;
    }
    
    network_message_t msg;
    memset(&msg, 0, sizeof(network_message_t));
    
    msg.type = SENSOR_DATA;
    memcpy(msg.sender_id, _own_mac, 6);
    msg.message_id = _message_counter++;
    msg.is_master = _is_master;
    msg.ready = true;
    msg.data_count = min(data_count, (uint8_t)5);  // Maximum 5 data points
    msg.battery_level = 100;  // Placeholder, implement actual battery level reading
    
    memcpy(msg.data, data, msg.data_count * sizeof(float));
    
    if (text) {
        strncpy(msg.text, text, sizeof(msg.text) - 1);
    }
    
    return _sendMessage(_master_mac, msg);
}

// Send alert or status update to all slave devices (for master device)
bool FloodAlertNetwork::sendToAllSlaves(const float* data, uint8_t data_count, uint8_t alert_level, const char* text) {
    if (!_is_master) {
        Serial.println("Only master can send to all slaves.");
        return false;
    }
    
    bool all_success = true;
    network_message_t msg;
    memset(&msg, 0, sizeof(network_message_t));
    
    msg.type = alert_level > 0 ? ALERT : STATUS_UPDATE;
    memcpy(msg.sender_id, _own_mac, 6);
    msg.message_id = _message_counter++;
    msg.is_master = true;
    msg.ready = true;
    msg.data_count = min(data_count, (uint8_t)5);
    msg.alert_level = alert_level;
    msg.battery_level = 100;  // Placeholder
    
    memcpy(msg.data, data, msg.data_count * sizeof(float));
    
    if (text) {
        strncpy(msg.text, text, sizeof(msg.text) - 1);
    }
    
    // Send to all peers except master
    for (int i = 0; i < MAX_PEERS; i++) {
        if (_peers[i].in_use && !_peers[i].is_master) {
            bool result = _sendMessage(_peers[i].peer_info.peer_addr, msg);
            all_success = all_success && result;
        }
    }
    
    return all_success;
}

// Send a message to a specific slave
bool FloodAlertNetwork::sendToSlave(const uint8_t* mac_addr, const float* data, uint8_t data_count, const char* text) {
    if (!_is_master) {
        // Serial.println("Only master can send to specific slaves.");
        return false;
    }
    
    int peer_idx = _findPeerIndex(mac_addr);
    if (peer_idx < 0) {
        // Serial.println("Slave not found in peer list.");
        return false;
    }
    
    network_message_t msg;
    memset(&msg, 0, sizeof(network_message_t));
    
    msg.type = COMMAND;
    memcpy(msg.sender_id, _own_mac, 6);
    msg.message_id = _message_counter++;
    msg.is_master = true;
    msg.ready = true;
    msg.data_count = min(data_count, (uint8_t)5);
    msg.battery_level = 100;  // Placeholder
    
    memcpy(msg.data, data, msg.data_count * sizeof(float));
    
    if (text) {
        strncpy(msg.text, text, sizeof(msg.text) - 1);
    }
    
    return _sendMessage(mac_addr, msg);
}

// Broadcast a discovery message to find other devices
void FloodAlertNetwork::broadcastDiscovery() {
    uint8_t broadcast_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    // Add broadcast address as a peer for discovery
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcast_addr, 6);
    peerInfo.channel = _channel;
    peerInfo.encrypt = false;
    
    // For this version, esp_now_get_peer expects a peer_info pointer, not a double pointer
    // We'll simplify by just trying to add the peer directly
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        // If adding fails, try to modify instead
        esp_now_mod_peer(&peerInfo);
    }
    
    // Create and send discovery message
    network_message_t msg;
    memset(&msg, 0, sizeof(network_message_t));
    
    msg.type = DISCOVERY;
    memcpy(msg.sender_id, _own_mac, 6);
    msg.message_id = _message_counter++;
    msg.is_master = _is_master;
    msg.ready = true;
    msg.battery_level = 100;  // Placeholder
    
    // Add device name to text field
    strncpy(msg.text, _device_name, sizeof(msg.text) - 1);
    
    _sendMessage(broadcast_addr, msg);
    _last_discovery = millis();
    
    // Remove broadcast peer after sending (ESP-NOW spec recommends this)
    esp_now_del_peer(broadcast_addr);
}

// Check if a peer is master
bool FloodAlertNetwork::isPeerMaster(const uint8_t* mac_addr) {
    int idx = _findPeerIndex(mac_addr);
    return (idx >= 0) ? _peers[idx].is_master : false;
}

// Check if the network is ready (enough peers connected)
bool FloodAlertNetwork::isNetworkReady() {
    if (_is_master) {
        return _peer_count >= _min_peers;
    } else {
        return _master_found;
    }
}

// Check if connected to master (for slave devices)
bool FloodAlertNetwork::isConnectedToMaster() {
    return _master_found;
}

// Get count of connected peers
uint8_t FloodAlertNetwork::getPeerCount() {
    return _peer_count;
}

// Get minimum required peers setting
uint8_t FloodAlertNetwork::getMinPeers() {
    return _min_peers;
}

// Set minimum required peers
void FloodAlertNetwork::setMinPeers(uint8_t min_peers) {
    _min_peers = min_peers;
}

// Process network tasks (call this regularly in loop())
void FloodAlertNetwork::update() {
    uint32_t now = millis();
    
    // Periodic discovery broadcasts (more frequent during initial setup)
    if ((!isNetworkReady() && now - _last_discovery > 2000) || 
        (isNetworkReady() && now - _last_discovery > 30000)) {
        broadcastDiscovery();
    }
    
    // If master, send regular status updates
    if (_is_master && isNetworkReady() && now - _last_status_send > 5000) {
        float data[1] = {0.0f};  // Placeholder data
        sendToAllSlaves(data, 1, 0, "Status OK");
        _last_status_send = now;
    }
    
    // Check for peer timeouts (30 seconds without communication)
    for (int i = 0; i < MAX_PEERS; i++) {
        if (_peers[i].in_use && now - _peers[i].last_seen > 30000) {
            const uint8_t* mac_addr = _peers[i].peer_info.peer_addr;
            
            // If the master timed out, reset master_found flag
            if (_peers[i].is_master && _master_found && _compareMac(mac_addr, _master_mac)) {
                _master_found = false;
                memset(_master_mac, 0, 6);
            }
            
            _removePeer(mac_addr);
        }
    }
    
    // Track network readiness
    if (isNetworkReady() && _network_ready_time == 0) {
        _network_ready_time = now;
        // Serial.println("Network is ready!");
    } else if (!isNetworkReady() && _network_ready_time != 0) {
        _network_ready_time = 0;
        // Serial.println("Network is no longer ready.");
    }
}

// Print network status for debugging
void FloodAlertNetwork::printNetworkStatus() {
    Logger::info("\n--- État du Réseau ---");
    Logger::infoF("Rôle: %s", _is_master ? "MASTER" : "SLAVE");
    Logger::infoF("Pairs connectés: %d (minimum: %d)", _peer_count, _min_peers);
    Logger::infoF("Réseau prêt: %s", isNetworkReady() ? "OUI" : "NON");
    
    if (!_is_master) {
        Logger::infoF("Connecté au master: %s", _master_found ? "OUI" : "NON");
    }
    
    Logger::info("--- Fin État ---\n");
}

// Print list of connected peers
void FloodAlertNetwork::printPeers() {
    Logger::ui("\n--- Pairs Connectés ---");
    int pairsCount = 0;
    
    for (int i = 0; i < MAX_PEERS; i++) {
        if (_peers[i].in_use) {
            pairsCount++;
            const uint8_t* mac = _peers[i].peer_info.peer_addr;
            Logger::ui("Pair MAC: ");
            Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X", 
                        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            Logger::uiF(" Rôle: %s", _peers[i].is_master ? "MASTER" : "SLAVE");
            Logger::uiF(" Prêt: %s", _peers[i].is_ready ? "OUI" : "NON");
            Logger::uiF(" Dernière connexion: %d secondes",
                      (millis() - _peers[i].last_seen) / 1000);
        }
    }
    
    if (pairsCount == 0) {
        Logger::ui("Aucun pair connecté");
    }
    
    Logger::ui("--- Fin Pairs ---\n");
}

// Set device name for identification
void FloodAlertNetwork::setDeviceName(const char* name) {
    strncpy(_device_name, name, sizeof(_device_name) - 1);
}

// Get master MAC address if known
bool FloodAlertNetwork::getMasterMac(uint8_t* mac_out) {
    if (_master_found) {
        memcpy(mac_out, _master_mac, 6);
        return true;
    }
    return false;
}

// Get own MAC address
void FloodAlertNetwork::getOwnMac(uint8_t* mac_out) {
    memcpy(mac_out, _own_mac, 6);
}

// Process a new peer discovery
void FloodAlertNetwork::_processPeerDiscovery(const network_message_t& msg, const uint8_t* mac_addr) {
    // If we're a slave and we find a master, register it
    if (!_is_master && msg.is_master && !_master_found) {
        _addPeer(mac_addr, true);
        memcpy(_master_mac, mac_addr, 6);
        _master_found = true;
        // Serial.println("Master found!");
    }
    // If we're the master, add all slaves
    else if (_is_master && !msg.is_master) {
        _addPeer(mac_addr, false);
    }
}

// Add a new peer to the network
bool FloodAlertNetwork::_addPeer(const uint8_t* mac_addr, bool is_master) {
    // Check if peer already exists
    int idx = _findPeerIndex(mac_addr);
    if (idx >= 0) {
        // Update last seen time
        _peers[idx].last_seen = millis();
        return true;
    }
    
    // Find a free slot
    int slot = _findFreePeerSlot();
    if (slot < 0) {
        Logger::info("No free slot for new peer.");
        return false;
    }
    
    // Create new peer
    memcpy(_peers[slot].peer_info.peer_addr, mac_addr, 6);
    _peers[slot].peer_info.channel = _channel;
    _peers[slot].peer_info.encrypt = false;
    _peers[slot].is_master = is_master;
    _peers[slot].is_ready = true;
    _peers[slot].last_seen = millis();
    _peers[slot].in_use = true;
    
    // Add peer to ESP-NOW
    esp_err_t result = esp_now_add_peer(&_peers[slot].peer_info);
    if (result != ESP_OK) {
        // Serial.print("Failed to add peer: ");
        // Serial.println(result);
        _peers[slot].in_use = false;
        return false;
    }
    
    _peer_count++;
    
    Logger::info("Nouveau pair ajouté: ");
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", mac_addr[i]);
        if (i < 5) Serial.print(":");
    }
    Logger::infoF(" Rôle: %s", is_master ? "MASTER" : "SLAVE");
    
    return true;
}

// Remove a peer from the network
bool FloodAlertNetwork::_removePeer(const uint8_t* mac_addr) {
    int idx = _findPeerIndex(mac_addr);
    if (idx < 0) {
        return false;  // Peer not found
    }
    
    // Remove peer from ESP-NOW
    esp_err_t result = esp_now_del_peer(mac_addr);
    if (result != ESP_OK) {
        Serial.print("Failed to remove peer: ");
        Serial.println(result);
        // Continue anyway to remove from our list
    }
    
    // Clear peer slot
    _peers[idx].in_use = false;
    _peer_count--;
    
    Serial.print("Removed peer: ");
    for (int i = 0; i < 6; i++) {
        Serial.print(mac_addr[i], HEX);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
    
    return true;
}

// Send a message to a specific peer
bool FloodAlertNetwork::_sendMessage(const uint8_t* mac_addr, network_message_t& msg) {
    if (!_initialized) {
        Serial.println("Cannot send message, network not initialized.");
        return false;
    }
    
    esp_err_t result = esp_now_send(mac_addr, (uint8_t*)&msg, sizeof(network_message_t));
    return (result == ESP_OK);
}

// Find peer index by MAC address
int FloodAlertNetwork::_findPeerIndex(const uint8_t* mac_addr) {
    for (int i = 0; i < MAX_PEERS; i++) {
        if (_peers[i].in_use && _compareMac(_peers[i].peer_info.peer_addr, mac_addr)) {
            return i;
        }
    }
    return -1;  // Not found
}

// Find an empty slot in the peers array
int FloodAlertNetwork::_findFreePeerSlot() {
    for (int i = 0; i < MAX_PEERS; i++) {
        if (!_peers[i].in_use) {
            return i;
        }
    }
    return -1;  // No free slots
}

// Compare two MAC addresses
bool FloodAlertNetwork::_compareMac(const uint8_t* mac1, const uint8_t* mac2) {
    return memcmp(mac1, mac2, 6) == 0;
}

// Static callback for ESP-NOW receive
void FloodAlertNetwork::_onReceiveHandler(const uint8_t* mac_addr, const uint8_t* data, int data_len) {
    if (!_instance) return;
    
    if (data_len != sizeof(network_message_t)) {
        Serial.println("Received message with invalid size.");
        return;
    }
    
    network_message_t msg;
    memcpy(&msg, data, sizeof(network_message_t));
    
    // Update peer information
    int peer_idx = _instance->_findPeerIndex(mac_addr);
    if (peer_idx >= 0) {
        _instance->_peers[peer_idx].last_seen = millis();
        _instance->_peers[peer_idx].is_master = msg.is_master;
        _instance->_peers[peer_idx].is_ready = msg.ready;
    } else {
        // New peer, process discovery if it's a discovery message
        if (msg.type == DISCOVERY) {
            _instance->_processPeerDiscovery(msg, mac_addr);
        }
    }
    
    // Handle message based on type
    switch (msg.type) {
        case DISCOVERY:
            // Already handled above
            break;
            
        case SENSOR_DATA:
            // Only master processes sensor data
            if (_instance->_is_master && _instance->_data_ready_callback) {
                _instance->_data_ready_callback(msg.data, msg.data_count);
            }
            break;
            
        case ALERT:
        case STATUS_UPDATE:
        case COMMAND:
            // These are primarily handled by the user callback
            break;
            
        case PING:
            // Just update the last_seen time (already done above)
            break;
            
        default:
            Serial.print("Unknown message type: ");
            Serial.println(msg.type);
            break;
    }
    
    // Call user callback if registered
    if (_instance->_message_callback) {
        _instance->_message_callback(msg, mac_addr);
    }
}

// Static callback for ESP-NOW send status
void FloodAlertNetwork::_onSendHandler(const uint8_t* mac_addr, esp_now_send_status_t status) {
    if (!_instance) return;
    
    bool success = (status == ESP_NOW_SEND_SUCCESS);
    
    // Update retry count for failed sends
    if (!success && mac_addr) {
        int peer_idx = _instance->_findPeerIndex(mac_addr);
        if (peer_idx >= 0) {
            _instance->_peers[peer_idx].retry_count++;
            
            // If too many failures, consider removing the peer
            if (_instance->_peers[peer_idx].retry_count > 5) {
                Serial.print("Too many failed sends to peer, removing: ");
                for (int i = 0; i < 6; i++) {
                    Serial.print(mac_addr[i], HEX);
                    if (i < 5) Serial.print(":");
                }
                Serial.println();
                
                _instance->_removePeer(mac_addr);
                
                // If the master was removed, reset master_found flag
                if (!_instance->_is_master && 
                    _instance->_master_found && 
                    _instance->_compareMac(mac_addr, _instance->_master_mac)) {
                    _instance->_master_found = false;
                    memset(_instance->_master_mac, 0, 6);
                }
            }
        }
    }
    
    // Reset retry count for successful sends
    if (success && mac_addr) {
        int peer_idx = _instance->_findPeerIndex(mac_addr);
        if (peer_idx >= 0) {
            _instance->_peers[peer_idx].retry_count = 0;
        }
    }
    
    // Call user callback if registered
    if (_instance->_delivery_callback) {
        _instance->_delivery_callback(success, mac_addr);
    }
}