/**
 * Flood Alert System - Alerts Management JavaScript
 * Handles alert creation, management, and configuration
 */

// DOM Elements
const refreshBtn = document.getElementById('refresh-btn');
const lastUpdatedEl = document.getElementById('last-updated');
const networkIndicator = document.getElementById('network-indicator');
const networkText = document.getElementById('network-text');
const systemTimeEl = document.getElementById('system-time');
const activeAlertsContainer = document.getElementById('active-alerts-container');
const noActiveAlerts = document.getElementById('no-active-alerts');
const alertsHistoryBody = document.getElementById('alerts-history-body');
const alertFilter = document.getElementById('alert-filter');
const testAlertBtn = document.getElementById('test-alert-btn');
const saveAlertSettingsBtn = document.getElementById('save-alert-settings');
const resetAlertSettingsBtn = document.getElementById('reset-alert-settings');

// Form fields
const warningThreshold = document.getElementById('warning-threshold');
const criticalThreshold = document.getElementById('critical-threshold');
const tempWarning = document.getElementById('temp-warning');
const enableSound = document.getElementById('enable-sound');
const enableVisual = document.getElementById('enable-visual');
const enableEmail = document.getElementById('enable-email');
const emailAddress = document.getElementById('email-address');

// Data storage for alert settings
let alertSettings = {
    warningThreshold: 50,
    criticalThreshold: 75,
    tempWarning: 35,
    enableSound: true,
    enableVisual: true,
    enableEmail: false,
    emailAddress: ''
};

// Mock active alerts data (in a real implementation, this would come from an API)
let activeAlerts = [
    {
        id: 1,
        title: 'Critical Water Level',
        description: 'Sensor 2 has reported a critical water level of 85.5 cm.',
        type: 'critical',
        source: 'Sensor 2',
        sourceMac: 'AA:BB:CC:DD:EE:FF',
        timestamp: Date.now() - 300000, // 5 minutes ago
        value: 85.5,
        acknowledged: false
    },
    {
        id: 2,
        title: 'Warning: Rising Water Level',
        description: 'Sensor 3 has reported a rising water level, currently at 65.2 cm.',
        type: 'warning',
        source: 'Sensor 3',
        sourceMac: 'AA:BB:CC:DD:EE:33',
        timestamp: Date.now() - 900000, // 15 minutes ago
        value: 65.2,
        acknowledged: false
    }
];

// Mock alert history (in a real implementation, this would come from an API)
let alertHistory = [
    {
        id: 3,
        title: 'Critical Water Level',
        description: 'Critical water level (85.5 cm)',
        type: 'critical',
        source: 'Sensor 2',
        startTime: Date.now() - 3000000, // 50 minutes ago
        endTime: Date.now() - 2100000, // 35 minutes ago
        duration: 900000, // 15 minutes
        status: 'resolved'
    },
    {
        id: 4,
        title: 'Warning: Rising Water Level',
        description: 'Rising water level (65.2 cm)',
        type: 'warning',
        source: 'Sensor 3',
        startTime: Date.now() - 2400000, // 40 minutes ago
        endTime: Date.now() - 1200000, // 20 minutes ago
        duration: 1200000, // 20 minutes
        status: 'resolved'
    },
    {
        id: 5,
        title: 'Temperature Warning',
        description: 'Temperature spike (38.5°C)',
        type: 'warning',
        source: 'Sensor 1',
        startTime: Date.now() - 1800000, // 30 minutes ago
        endTime: Date.now() - 900000, // 15 minutes ago
        duration: 900000, // 15 minutes
        status: 'resolved'
    },
    {
        id: 6,
        title: 'Critical Water Level',
        description: 'Critical water level (90.2 cm)',
        type: 'critical',
        source: 'Sensor 4',
        startTime: Date.now() - 86400000 - 10800000, // Yesterday + 3 hours
        endTime: Date.now() - 86400000, // 24 hours ago
        duration: 10800000, // 3 hours
        status: 'resolved'
    }
];

// Initialize the alerts page
function initAlertsPage() {
    // Set up event listeners
    refreshBtn.addEventListener('click', fetchAlertData);
    alertFilter.addEventListener('change', filterAlertHistory);
    testAlertBtn.addEventListener('click', createTestAlert);
    saveAlertSettingsBtn.addEventListener('click', saveSettings);
    resetAlertSettingsBtn.addEventListener('click', resetSettings);
    
    // Update time every second
    setInterval(updateTime, 1000);
    
    // Load saved settings (in a real implementation, this would come from localStorage or an API)
    loadSettings();
    
    // Initial data fetch
    fetchAlertData();
    
    // Auto-refresh data every 30 seconds
    setInterval(fetchAlertData, 30000);
    
    // Add event listeners for alert acknowledge/dismiss buttons
    document.addEventListener('click', function(e) {
        // Acknowledge alert button
        if (e.target.closest('.btn') && e.target.closest('.btn').innerHTML.includes('Acknowledge')) {
            const alertCard = e.target.closest('.alert-card');
            if (alertCard) {
                const alertId = parseInt(alertCard.getAttribute('data-id'));
                acknowledgeAlert(alertId);
            }
        }
        
        // Dismiss alert button
        if (e.target.closest('.btn') && e.target.closest('.btn').innerHTML.includes('Dismiss')) {
            const alertCard = e.target.closest('.alert-card');
            if (alertCard) {
                const alertId = parseInt(alertCard.getAttribute('data-id'));
                dismissAlert(alertId);
            }
        }
    });
}

// Update system time display
function updateTime() {
    const now = new Date();
    systemTimeEl.textContent = now.toLocaleTimeString();
}

// Format time since given timestamp
function formatTimeSince(timestamp) {
    const seconds = Math.floor((Date.now() - timestamp) / 1000);
    
    if (seconds < 60) {
        return `${seconds} seconds ago`;
    } else if (seconds < 3600) {
        return `${Math.floor(seconds / 60)} minutes ago`;
    } else if (seconds < 86400) {
        return `${Math.floor(seconds / 3600)} hours ago`;
    } else {
        return `${Math.floor(seconds / 86400)} days ago`;
    }
}

// Format timestamp
function formatDateTime(timestamp) {
    const date = new Date(timestamp);
    return date.toLocaleString();
}

// Format duration in human-readable format
function formatDuration(milliseconds) {
    const seconds = Math.floor(milliseconds / 1000);
    
    if (seconds < 60) {
        return `${seconds} seconds`;
    } else if (seconds < 3600) {
        return `${Math.floor(seconds / 60)} minutes`;
    } else if (seconds < 86400) {
        return `${Math.floor(seconds / 3600)} hours`;
    } else {
        return `${Math.floor(seconds / 86400)} days`;
    }
}

// Fetch alert data
function fetchAlertData() {
    // In a real implementation, this would fetch data from an API
    // For this demo, we'll use the mock data
    
    // Fetch sensors data to update active alerts
    fetch('/api/sensors')
        .then(response => response.json())
        .then(data => {
            updateNetworkStatus(data.networkReady);
            lastUpdatedEl.textContent = `Last updated: ${new Date().toLocaleTimeString()}`;
            
            // In a real implementation, we would process the sensor data
            // to create or update alerts based on thresholds
            
            // For this demo, we'll just use the mock active alerts
            updateActiveAlerts();
            updateAlertHistory();
        })
        .catch(error => {
            console.error('Error fetching sensor data:', error);
            updateNetworkStatus(false);
            
            // Even if the fetch fails, update the UI with mock data for demo
            updateActiveAlerts();
            updateAlertHistory();
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

// Update active alerts display
function updateActiveAlerts() {
    if (activeAlerts.length === 0) {
        activeAlertsContainer.innerHTML = '';
        noActiveAlerts.classList.remove('hidden');
        return;
    }
    
    noActiveAlerts.classList.add('hidden');
    let html = '';
    
    activeAlerts.forEach(alert => {
        const alertClass = alert.type === 'critical' ? 'bg-danger' : 'bg-warning';
        const alertIcon = alert.type === 'critical' ? 'fas fa-radiation' : 'fas fa-exclamation-triangle';
        const animationClass = alert.type === 'critical' ? 'animate__animated animate__pulse animate__infinite' : '';
        
        html += `
            <div class="alert-card ${alertClass} ${animationClass}" data-id="${alert.id}">
                <div class="alert-icon">
                    <i class="${alertIcon}"></i>
                </div>
                <div class="alert-content">
                    <h3 class="alert-title">${alert.title}</h3>
                    <p class="alert-description">${alert.description}</p>
                    <div class="alert-meta">
                        <span class="alert-time">${formatTimeSince(alert.timestamp)}</span>
                        <span class="alert-source">${alert.source} (${alert.sourceMac})</span>
                    </div>
                </div>
                <div class="alert-actions">
                    <button class="btn btn-sm btn-light"><i class="fas fa-check"></i> Acknowledge</button>
                    <button class="btn btn-sm btn-light"><i class="fas fa-times"></i> Dismiss</button>
                </div>
            </div>
        `;
    });
    
    activeAlertsContainer.innerHTML = html;
}

// Update alert history table
function updateAlertHistory() {
    // Filter alerts based on current filter selection
    const filterValue = alertFilter.value;
    let filteredHistory = [...alertHistory];
    
    if (filterValue === 'warning') {
        filteredHistory = alertHistory.filter(alert => alert.type === 'warning');
    } else if (filterValue === 'critical') {
        filteredHistory = alertHistory.filter(alert => alert.type === 'critical');
    }
    
    // Sort by start time (most recent first)
    filteredHistory.sort((a, b) => b.startTime - a.startTime);
    
    let html = '';
    filteredHistory.forEach(alert => {
        const typeClass = alert.type === 'critical' ? 'danger' : 'warning';
        const typeText = alert.type === 'critical' ? 'Critical' : 'Warning';
        
        html += `
            <tr class="alert-row ${typeClass}">
                <td><span class="badge bg-${typeClass}">${typeText}</span></td>
                <td>${alert.description}</td>
                <td>${alert.source}</td>
                <td>${formatDateTime(alert.startTime)}</td>
                <td>${formatDuration(alert.duration)}</td>
                <td><span class="badge bg-secondary">Resolved</span></td>
            </tr>
        `;
    });
    
    if (filteredHistory.length === 0) {
        html = `
            <tr>
                <td colspan="6" class="text-center">
                    <div class="no-data-message">
                        <i class="fas fa-history"></i>
                        <p>No alert history found</p>
                    </div>
                </td>
            </tr>
        `;
    }
    
    alertsHistoryBody.innerHTML = html;
}

// Filter alert history based on selection
function filterAlertHistory() {
    updateAlertHistory();
}

// Acknowledge an alert
function acknowledgeAlert(alertId) {
    // Find the alert
    const alertIndex = activeAlerts.findIndex(alert => alert.id === alertId);
    if (alertIndex !== -1) {
        // In a real implementation, this would send an acknowledgment to the API
        
        // Update local state
        activeAlerts[alertIndex].acknowledged = true;
        
        // For this demo, we'll just move it to history after a delay
        setTimeout(() => {
            // Move to history
            const alert = activeAlerts[alertIndex];
            alertHistory.unshift({
                id: alert.id,
                title: alert.title,
                description: alert.description,
                type: alert.type,
                source: alert.source,
                startTime: alert.timestamp,
                endTime: Date.now(),
                duration: Date.now() - alert.timestamp,
                status: 'resolved'
            });
            
            // Remove from active alerts
            activeAlerts.splice(alertIndex, 1);
            
            // Update UI
            updateActiveAlerts();
            updateAlertHistory();
        }, 2000);
    }
}

// Dismiss an alert
function dismissAlert(alertId) {
    // Find the alert
    const alertIndex = activeAlerts.findIndex(alert => alert.id === alertId);
    if (alertIndex !== -1) {
        // In a real implementation, this would send a dismissal to the API
        
        // Remove from active alerts
        activeAlerts.splice(alertIndex, 1);
        
        // Update UI
        updateActiveAlerts();
    }
}

// Create a test alert for demonstration
function createTestAlert() {
    // Generate new alert ID
    const newId = Math.max(...activeAlerts.map(a => a.id), ...alertHistory.map(a => a.id)) + 1;
    
    // Create random alert type
    const isWarning = Math.random() > 0.5;
    const type = isWarning ? 'warning' : 'critical';
    const title = isWarning ? 'Warning: Water Level Rising' : 'Critical Water Level Alert';
    
    // Create random value
    const waterLevel = isWarning ? 
        (alertSettings.warningThreshold + Math.random() * 10).toFixed(1) : 
        (alertSettings.criticalThreshold + Math.random() * 10).toFixed(1);
    
    // Create random source
    const sensorNumber = Math.floor(Math.random() * 4) + 1;
    const source = `Sensor ${sensorNumber}`;
    const sourceMac = `AA:BB:CC:DD:EE:${sensorNumber.toString(16).padStart(2, '0')}`;
    
    // Create alert
    const newAlert = {
        id: newId,
        title: title,
        description: `${source} has reported a water level of ${waterLevel} cm.`,
        type: type,
        source: source,
        sourceMac: sourceMac,
        timestamp: Date.now(),
        value: parseFloat(waterLevel),
        acknowledged: false
    };
    
    // Add to active alerts
    activeAlerts.push(newAlert);
    
    // Update UI
    updateActiveAlerts();
    
    // Play sound if enabled
    if (alertSettings.enableSound) {
        playAlertSound(type);
    }
    
    // Notify (in a real app, this would send an actual notification)
    if (type === 'critical') {
        notifyUser(title, newAlert.description);
    }
}

// Play alert sound (mock implementation)
function playAlertSound(type) {
    // In a real implementation, this would play an actual sound
    console.log(`Playing ${type} alert sound`);
}

// Send notification (mock implementation)
function notifyUser(title, message) {
    // In a real implementation, this would show a browser notification
    // or send an email based on settings
    if (alertSettings.enableEmail && alertSettings.emailAddress) {
        console.log(`Sending email to ${alertSettings.emailAddress}: ${title} - ${message}`);
    }
    
    // Show browser notification if supported and enabled
    if (alertSettings.enableVisual && 'Notification' in window) {
        // Check if we have permission
        if (Notification.permission === 'granted') {
            new Notification(title, { body: message });
        } else if (Notification.permission !== 'denied') {
            Notification.requestPermission().then(permission => {
                if (permission === 'granted') {
                    new Notification(title, { body: message });
                }
            });
        }
    }
}

// Load saved settings
function loadSettings() {
    // In a real implementation, this would load from localStorage or an API
    // For this demo, we'll use the default settings
    
    // Apply settings to form fields
    warningThreshold.value = alertSettings.warningThreshold;
    criticalThreshold.value = alertSettings.criticalThreshold;
    tempWarning.value = alertSettings.tempWarning;
    enableSound.checked = alertSettings.enableSound;
    enableVisual.checked = alertSettings.enableVisual;
    enableEmail.checked = alertSettings.enableEmail;
    emailAddress.value = alertSettings.emailAddress;
    
    // Update email field enabled state
    emailAddress.disabled = !alertSettings.enableEmail;
}

// Save settings
function saveSettings() {
    // Get values from form fields
    alertSettings.warningThreshold = parseInt(warningThreshold.value);
    alertSettings.criticalThreshold = parseInt(criticalThreshold.value);
    alertSettings.tempWarning = parseInt(tempWarning.value);
    alertSettings.enableSound = enableSound.checked;
    alertSettings.enableVisual = enableVisual.checked;
    alertSettings.enableEmail = enableEmail.checked;
    alertSettings.emailAddress = emailAddress.value;
    
    // In a real implementation, this would save to localStorage or an API
    
    // Show confirmation
    showToast('Settings saved successfully');
    
    // Update email field enabled state
    emailAddress.disabled = !alertSettings.enableEmail;
}

// Reset settings to defaults
function resetSettings() {
    // Reset to defaults
    alertSettings = {
        warningThreshold: 50,
        criticalThreshold: 75,
        tempWarning: 35,
        enableSound: true,
        enableVisual: true,
        enableEmail: false,
        emailAddress: ''
    };
    
    // Apply to form
    loadSettings();
    
    // Show confirmation
    showToast('Settings reset to defaults');
}

// Show toast message (mock implementation)
function showToast(message) {
    // In a real implementation, this would show a toast notification
    alert(message);
}

// Toggle email field based on checkbox
enableEmail.addEventListener('change', function() {
    emailAddress.disabled = !this.checked;
});

// Start the page when DOM is ready
document.addEventListener('DOMContentLoaded', initAlertsPage);