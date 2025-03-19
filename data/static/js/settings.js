/**
 * Flood Alert System - Settings Management JavaScript
 * Handles device configuration, WiFi scanning, and connection setup
 */

// DOM Elements - Global UI
const refreshBtn = document.getElementById('refresh-btn');
const lastUpdatedEl = document.getElementById('last-updated');
const networkIndicator = document.getElementById('network-indicator');
const networkText = document.getElementById('network-text');
const systemTimeEl = document.getElementById('system-time');

// WiFi Status Elements
const wifiConnectionStatus = document.getElementById('wifi-connection-status');
const wifiSSID = document.getElementById('wifi-ssid');
const wifiIP = document.getElementById('wifi-ip');
const wifiSignal = document.getElementById('wifi-signal');

// WiFi Scanning Elements
const scanWifiBtn = document.getElementById('scan-wifi-btn');
const networksLoading = document.getElementById('networks-loading');
const networksList = document.getElementById('networks-list');
const wifiNetworksBody = document.getElementById('wifi-networks-body');
const noNetworksMessage = document.getElementById('no-networks-message');

// Device Settings Elements
const deviceNameInput = document.getElementById('device-name');
const apSSIDInput = document.getElementById('ap-ssid');
const apPasswordInput = document.getElementById('ap-password');
const minPeersInput = document.getElementById('min-peers');
const wifiChannelSelect = document.getElementById('wifi-channel');
const saveSystemSettingsBtn = document.getElementById('save-system-settings');
const resetSystemSettingsBtn = document.getElementById('reset-system-settings');

// WiFi Connect Modal Elements
const wifiConnectModal = document.getElementById('wifi-connect-modal');
const connectModalTitle = document.getElementById('connect-modal-title');
const connectModalClose = document.getElementById('connect-modal-close');
const connectSSIDInput = document.getElementById('connect-ssid');
const connectPasswordInput = document.getElementById('connect-password');
const connectStatus = document.getElementById('connect-status');
const connectStatusMessage = document.getElementById('connect-status-message');
const wifiConnectBtn = document.getElementById('wifi-connect-btn');
const connectCancelBtn = document.getElementById('connect-cancel-btn');

// Success Modal Elements
const connectionSuccessModal = document.getElementById('connection-success-modal');
const successModalClose = document.getElementById('success-modal-close');
const successSSID = document.getElementById('success-ssid');
const successIPAddress = document.getElementById('success-ip-address');
const successOkBtn = document.getElementById('success-ok-btn');

// Failed Modal Elements
const connectionFailedModal = document.getElementById('connection-failed-modal');
const failedModalClose = document.getElementById('failed-modal-close');
const failedSSID = document.getElementById('failed-ssid');
const failureReason = document.getElementById('failure-reason');
const retryBtn = document.getElementById('retry-btn');
const failedCancelBtn = document.getElementById('failed-cancel-btn');

// Data storage
let systemSettings = {
    deviceName: 'AlertStation',
    apSSID: 'FloodAlertSystem',
    apPassword: 'floodalert123',
    minPeers: 1,
    wifiChannel: 6
};

let availableNetworks = [];
let selectedNetwork = null;
let connectionAttemptInProgress = false;

// Initialize the settings page
function initSettingsPage() {
    // Set up event listeners
    refreshBtn.addEventListener('click', refreshAllData);
    scanWifiBtn.addEventListener('click', scanWifiNetworks);
    
    // System settings buttons
    saveSystemSettingsBtn.addEventListener('click', saveSystemSettings);
    resetSystemSettingsBtn.addEventListener('click', resetSystemSettings);
    
    // WiFi connect modal
    connectModalClose.addEventListener('click', closeConnectModal);
    wifiConnectBtn.addEventListener('click', connectToWifi);
    connectCancelBtn.addEventListener('click', closeConnectModal);
    
    // Success modal
    successModalClose.addEventListener('click', closeSuccessModal);
    successOkBtn.addEventListener('click', closeSuccessModal);
    
    // Failed modal
    failedModalClose.addEventListener('click', closeFailedModal);
    retryBtn.addEventListener('click', reopenConnectModal);
    failedCancelBtn.addEventListener('click', closeFailedModal);
    
    // Update time every second
    setInterval(updateTime, 1000);
    
    // Initial data fetch
    refreshAllData();
    
    // Hide loading indicators initially
    networksLoading.classList.add('hidden');
    
    // For demonstration, scan networks automatically on page load
    setTimeout(scanWifiNetworks, 1000);
}

// Update system time display
function updateTime() {
    const now = new Date();
    systemTimeEl.textContent = now.toLocaleTimeString();
}

// Format timestamp
function formatLastUpdated() {
    return `Last updated: ${new Date().toLocaleTimeString()}`;
}

// Refresh all data from APIs
function refreshAllData() {
    lastUpdatedEl.textContent = formatLastUpdated();
    
    // Fetch system status for WiFi info
    fetch('/api/status')
        .then(response => response.json())
        .then(data => {
            updateWifiStatus(data);
            updateNetworkStatus(data.networkReady);
            
            // Update form fields with current settings
            deviceNameInput.value = data.deviceName || systemSettings.deviceName;
            minPeersInput.value = data.minPeers || systemSettings.minPeers;
            
            if (data.wifi) {
                apSSIDInput.value = data.wifi.apSSID || systemSettings.apSSID;
                // We don't update password as we don't want to expose it
                
                // Update channel if available
                if (data.wifi.channel) {
                    wifiChannelSelect.value = data.wifi.channel;
                }
            }
        })
        .catch(error => {
            console.error('Error fetching system status:', error);
            updateNetworkStatus(false);
        });
}

// Update network status indicator
function updateNetworkStatus(isOnline) {
    if (isOnline) {
        networkIndicator.className = 'indicator online';
        networkText.textContent = 'Network Online';
    } else {
        networkIndicator.className = 'indicator offline';
        networkText.textContent = 'Network Offline';
    }
}

// Update WiFi status display
function updateWifiStatus(data) {
    if (data.wifi && data.wifi.staConnected) {
        wifiConnectionStatus.textContent = 'Connected';
        wifiConnectionStatus.className = 'info-value text-success';
        
        wifiSSID.textContent = data.wifi.staSSID || 'Unknown Network';
        wifiIP.textContent = data.wifi.staIP || '-';
        
        // Format signal strength
        const rssi = data.wifi.rssi || 0;
        let signalQuality = 'Poor';
        let signalClass = 'text-danger';
        
        if (rssi > -60) {
            signalQuality = 'Excellent';
            signalClass = 'text-success';
        } else if (rssi > -70) {
            signalQuality = 'Good';
            signalClass = 'text-success';
        } else if (rssi > -80) {
            signalQuality = 'Fair';
            signalClass = 'text-warning';
        }
        
        wifiSignal.textContent = `${rssi} dBm (${signalQuality})`;
        wifiSignal.className = `info-value ${signalClass}`;
    } else {
        wifiConnectionStatus.textContent = 'Not connected';
        wifiConnectionStatus.className = 'info-value text-danger';
        
        wifiSSID.textContent = '-';
        wifiIP.textContent = '-';
        wifiSignal.textContent = '-';
        wifiSignal.className = 'info-value';
    }
}

// Scan for WiFi networks
function scanWifiNetworks() {
    // Show loading
    networksLoading.classList.remove('hidden');
    networksList.classList.add('hidden');
    noNetworksMessage.classList.add('hidden');
    scanWifiBtn.disabled = true;
    
    // Fetch scan results from API
    fetch('/api/wifi/scan')
        .then(response => response.json())
        .then(data => {
            availableNetworks = data.networks || [];
            updateWifiNetworksList();
            
            // Hide loading, show results
            networksLoading.classList.add('hidden');
            
            if (availableNetworks.length > 0) {
                networksList.classList.remove('hidden');
            } else {
                noNetworksMessage.classList.remove('hidden');
            }
            
            scanWifiBtn.disabled = false;
        })
        .catch(error => {
            console.error('Error scanning WiFi networks:', error);
            
            // For development/demo, use mock data
            console.log('Using mock WiFi scan data');
            availableNetworks = getMockWifiNetworks();
            updateWifiNetworksList();
            
            // Hide loading, show results
            networksLoading.classList.add('hidden');
            
            if (availableNetworks.length > 0) {
                networksList.classList.remove('hidden');
            } else {
                noNetworksMessage.classList.remove('hidden');
            }
            
            scanWifiBtn.disabled = false;
        });
}

// Update WiFi networks list
function updateWifiNetworksList() {
    let html = '';
    
    availableNetworks.sort((a, b) => b.rssi - a.rssi);
    
    availableNetworks.forEach(network => {
        const signalStrength = getSignalStrengthIcon(network.rssi);
        const securityType = getSecurityTypeText(network.encryption);
        
        html += `
            <tr data-ssid="${escapeHTML(network.ssid)}">
                <td>${escapeHTML(network.ssid)}</td>
                <td>${signalStrength}</td>
                <td>${securityType}</td>
                <td>
                    <button class="btn btn-sm btn-primary connect-wifi-btn" data-ssid="${escapeHTML(network.ssid)}">
                        <i class="fas fa-plug"></i> Connect
                    </button>
                </td>
            </tr>
        `;
    });
    
    wifiNetworksBody.innerHTML = html;
    
    // Add event listeners to connect buttons
    document.querySelectorAll('.connect-wifi-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const ssid = e.target.closest('button').getAttribute('data-ssid');
            openConnectModal(ssid);
        });
    });
}

// Get signal strength icon based on RSSI
function getSignalStrengthIcon(rssi) {
    let iconClass = 'fas fa-wifi';
    let strengthText = '';
    let colorClass = '';
    
    if (rssi >= -50) {
        strengthText = 'Excellent';
        colorClass = 'text-success';
    } else if (rssi >= -60) {
        strengthText = 'Good';
        colorClass = 'text-success';
    } else if (rssi >= -70) {
        strengthText = 'Fair';
        colorClass = 'text-warning';
    } else {
        strengthText = 'Poor';
        colorClass = 'text-danger';
    }
    
    return `<span class="${colorClass}"><i class="${iconClass}"></i> ${strengthText} (${rssi} dBm)</span>`;
}

// Get security type text
function getSecurityTypeText(encryption) {
    if (!encryption || encryption === 'open' || encryption === 'OPEN') {
        return '<span><i class="fas fa-lock-open"></i> Open</span>';
    }
    
    if (encryption === 'WEP') {
        return '<span><i class="fas fa-lock"></i> WEP</span>';
    }
    
    if (encryption.includes('WPA2')) {
        return '<span><i class="fas fa-lock"></i> WPA2</span>';
    }
    
    if (encryption.includes('WPA3')) {
        return '<span><i class="fas fa-lock"></i> WPA3</span>';
    }
    
    if (encryption.includes('WPA')) {
        return '<span><i class="fas fa-lock"></i> WPA</span>';
    }
    
    return '<span><i class="fas fa-lock"></i> Secured</span>';
}

// Open the connect modal for a specific network
function openConnectModal(ssid) {
    // Find the network in the available networks
    selectedNetwork = availableNetworks.find(n => n.ssid === ssid);
    
    // Update modal content
    connectModalTitle.textContent = `Connect to "${ssid}"`;
    connectSSIDInput.value = ssid;
    connectPasswordInput.value = '';
    
    // Show the modal
    wifiConnectModal.classList.add('active');
    
    // Focus the password field
    connectPasswordInput.focus();
    
    // Reset connection status
    connectStatus.classList.add('hidden');
    wifiConnectBtn.disabled = false;
}

// Close the connect modal
function closeConnectModal() {
    wifiConnectModal.classList.remove('active');
    selectedNetwork = null;
}

// Connect to the selected WiFi network
function connectToWifi() {
    if (!selectedNetwork || connectionAttemptInProgress) return;
    
    const ssid = connectSSIDInput.value;
    const password = connectPasswordInput.value;
    
    // For networks with security, password is required
    const isOpenNetwork = !selectedNetwork.encryption || 
                          selectedNetwork.encryption === 'open' || 
                          selectedNetwork.encryption === 'OPEN';
    
    if (!isOpenNetwork && !password) {
        alert('Please enter the WiFi password.');
        connectPasswordInput.focus();
        return;
    }
    
    // Show connecting status
    connectStatus.classList.remove('hidden');
    wifiConnectBtn.disabled = true;
    connectionAttemptInProgress = true;
    
    // Create request data
    const requestData = {
        ssid: ssid,
        password: password
    };
    
    // Send connection request to API
    fetch('/api/wifi/connect', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(requestData)
    })
    .then(response => response.json())
    .then(data => {
        connectionAttemptInProgress = false;
        
        if (data.success) {
            // Show success modal
            closeConnectModal();
            showSuccessModal(ssid, data.ip);
            
            // Refresh WiFi status after a delay
            setTimeout(refreshAllData, 1000);
        } else {
            // Show failure modal
            closeConnectModal();
            showFailedModal(ssid, data.message || 'Unknown error occurred');
        }
    })
    .catch(error => {
        console.error('Error connecting to WiFi:', error);
        connectionAttemptInProgress = false;
        
        // For development/demo, simulate success or failure
        const simulateSuccess = Math.random() > 0.3; // 70% success rate
        
        if (simulateSuccess) {
            closeConnectModal();
            showSuccessModal(ssid, '192.168.1.' + Math.floor(Math.random() * 255));
            
            // Update mock WiFi status
            wifiConnectionStatus.textContent = 'Connected';
            wifiConnectionStatus.className = 'info-value text-success';
            wifiSSID.textContent = ssid;
            wifiIP.textContent = '192.168.1.' + Math.floor(Math.random() * 255);
            wifiSignal.textContent = '-65 dBm (Good)';
            wifiSignal.className = 'info-value text-success';
        } else {
            closeConnectModal();
            showFailedModal(ssid, 'Connection failed. Please check the password and try again.');
        }
    });
}

// Show the success modal
function showSuccessModal(ssid, ipAddress) {
    successSSID.textContent = ssid;
    successIPAddress.textContent = ipAddress;
    connectionSuccessModal.classList.add('active');
}

// Close the success modal
function closeSuccessModal() {
    connectionSuccessModal.classList.remove('active');
}

// Show the failed modal
function showFailedModal(ssid, reason) {
    failedSSID.textContent = ssid;
    failureReason.textContent = reason;
    connectionFailedModal.classList.add('active');
}

// Close the failed modal
function closeFailedModal() {
    connectionFailedModal.classList.remove('active');
}

// Reopen connect modal (from failed modal)
function reopenConnectModal() {
    closeFailedModal();
    if (selectedNetwork) {
        openConnectModal(selectedNetwork.ssid);
    }
}

// Save system settings
function saveSystemSettings() {
    // Get values from form
    const newSettings = {
        deviceName: deviceNameInput.value,
        apSSID: apSSIDInput.value,
        apPassword: apPasswordInput.value,
        minPeers: parseInt(minPeersInput.value),
        wifiChannel: parseInt(wifiChannelSelect.value)
    };
    
    // Validate settings
    if (!newSettings.deviceName) {
        alert('Device name cannot be empty');
        return;
    }
    
    if (!newSettings.apSSID) {
        alert('Access Point SSID cannot be empty');
        return;
    }
    
    if (newSettings.apPassword && newSettings.apPassword !== '********' && newSettings.apPassword.length < 8) {
        alert('Access Point password must be at least 8 characters long');
        return;
    }
    
    // Don't send password if it's unchanged (masked with asterisks)
    if (newSettings.apPassword === '********') {
        delete newSettings.apPassword;
    }
    
    // Save settings to ESP32
    fetch('/api/settings/update', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(newSettings)
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            alert('Settings saved successfully. The device will restart to apply changes.');
            // Refresh page after short delay to allow restart
            setTimeout(() => {
                window.location.reload();
            }, 5000);
        } else {
            alert('Failed to save settings: ' + (data.message || 'Unknown error'));
        }
    })
    .catch(error => {
        console.error('Error saving settings:', error);
        // For development/demo, simulate success
        alert('Settings saved successfully (simulated). The device will restart to apply changes.');
    });
    
    // Update local copy of settings
    Object.assign(systemSettings, newSettings);
}

// Reset system settings to defaults
function resetSystemSettings() {
    if (confirm('Are you sure you want to reset all settings to default values?')) {
        // Reset form values
        deviceNameInput.value = 'AlertStation';
        apSSIDInput.value = 'FloodAlertSystem';
        apPasswordInput.value = 'floodalert123';
        minPeersInput.value = '1';
        wifiChannelSelect.value = '6';
        
        // Optional: send reset command to server
        fetch('/api/settings/reset', {
            method: 'POST'
        })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                alert('Settings reset successfully. The device will restart.');
                setTimeout(() => {
                    window.location.reload();
                }, 5000);
            } else {
                alert('Failed to reset settings: ' + (data.message || 'Unknown error'));
            }
        })
        .catch(error => {
            console.error('Error resetting settings:', error);
            // For development/demo
            alert('Settings reset successfully (simulated).');
        });
        
        // Reset local settings
        systemSettings = {
            deviceName: 'AlertStation',
            apSSID: 'FloodAlertSystem',
            apPassword: 'floodalert123',
            minPeers: 1,
            wifiChannel: 6
        };
    }
}

// Utility function to escape HTML for security
function escapeHTML(str) {
    return str
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#039;');
}

// Generate mock WiFi networks for testing when API is not available
function getMockWifiNetworks() {
    return [
        { ssid: 'Home Network', rssi: -45, encryption: 'WPA2-PSK' },
        { ssid: 'Office WiFi', rssi: -60, encryption: 'WPA2-PSK' },
        { ssid: 'Guest Network', rssi: -72, encryption: 'OPEN' },
        { ssid: 'Neighbor WiFi', rssi: -78, encryption: 'WPA3-PSK' },
        { ssid: 'IoT Network', rssi: -65, encryption: 'WPA2-PSK' },
        { ssid: 'Community WiFi', rssi: -85, encryption: 'WPA2-PSK' }
    ];
}

// Start the page when DOM is ready
document.addEventListener('DOMContentLoaded', initSettingsPage);