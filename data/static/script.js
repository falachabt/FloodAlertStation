/**
 * Flood Alert System - Dashboard JavaScript
 * Handles data fetching, visualization, and UI interaction for the main dashboard
 */

// DOM Elements
const refreshBtn = document.getElementById('refresh-btn');
const lastUpdatedEl = document.getElementById('last-updated');
const networkIndicator = document.getElementById('network-indicator');
const networkText = document.getElementById('network-text');
const systemTimeEl = document.getElementById('system-time');
const activeSensorsCount = document.getElementById('active-sensors-count');
const normalCount = document.getElementById('normal-count');
const warningCount = document.getElementById('warning-count');
const alertCount = document.getElementById('alert-count');
const sensorsContainer = document.getElementById('sensors-container');
const deviceNameEl = document.getElementById('device-name');
const deviceMacEl = document.getElementById('device-mac');
const apIpEl = document.getElementById('ap-ip');
const wifiStatusEl = document.getElementById('wifi-status');
const systemUptimeEl = document.getElementById('system-uptime');
const connectedPeersEl = document.getElementById('connected-peers');
const expandSensors = document.getElementById('expand-sensors');
const expandChart = document.getElementById('expand-chart');
const modalContainer = document.getElementById('modal-container');
const modalTitle = document.getElementById('modal-title');
const modalBody = document.getElementById('modal-body');
const modalClose = document.getElementById('modal-close');

// Chart instance
let waterLevelChart = null;

// Data storage
let sensorsData = [];
let systemStatus = {};

// Initialize the dashboard
function initDashboard() {
    // Set up event listeners
    refreshBtn.addEventListener('click', fetchAllData);
    expandSensors.addEventListener('click', () => showModal('Sensors Overview', createSensorsModalContent()));
    expandChart.addEventListener('click', () => showModal('Water Levels Chart', createChartModalContent()));
    modalClose.addEventListener('click', hideModal);
    
    // Update time every second
    setInterval(updateTime, 1000);
    
    // Initial data fetch
    fetchAllData();
    
    // Auto-refresh data every 30 seconds
    setInterval(fetchAllData, 30000);
}

// Update system time display
function updateTime() {
    const now = new Date();
    systemTimeEl.textContent = now.toLocaleTimeString();
}

// Format time since given timestamp
function formatTimeSince(seconds) {
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
function formatLastUpdated(timestamp) {
    const date = new Date(timestamp * 1000);
    return `Last updated: ${date.toLocaleTimeString()}`;
}

// Format uptime
function formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    
    let result = '';
    if (days > 0) {
        result += `${days}d `;
    }
    if (hours > 0 || days > 0) {
        result += `${hours}h `;
    }
    result += `${minutes}m`;
    
    return result;
}

// Fetch all data (sensors and system status)
function fetchAllData() {
    Promise.all([
        fetch('/api/sensors').then(response => response.json()),
        fetch('/api/status').then(response => response.json())
    ])
    .then(([sensorsResponse, statusResponse]) => {
        // Update data stores
        sensorsData = sensorsResponse.sensors || [];
        systemStatus = statusResponse;
        
        // Update UI with fetched data
        updateDashboard(sensorsResponse, statusResponse);
        
        // Update network status
        updateNetworkStatus(sensorsResponse.networkReady);
        
        // Update last updated time
        lastUpdatedEl.textContent = formatLastUpdated(sensorsResponse.timestamp);
    })
    .catch(error => {
        console.error('Error fetching data:', error);
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

// Update dashboard with new data
function updateDashboard(sensorsResponse, statusResponse) {
    // Update counts
    const sensors = sensorsResponse.sensors || [];
    let normalSensors = 0;
    let warningSensors = 0;
    let alertSensors = 0;
    
    // Count sensors by category
    sensors.forEach(sensor => {
        switch(sensor.category) {
            case 0: normalSensors++; break;
            case 1: warningSensors++; break;
            case 2: alertSensors++; break;
        }
    });
    
    // Update count displays
    activeSensorsCount.textContent = sensors.length;
    normalCount.textContent = normalSensors;
    warningCount.textContent = warningSensors;
    alertCount.textContent = alertSensors;
    
    // Update sensor cards
    updateSensorCards(sensors);
    
    // Update water level chart
    updateWaterLevelChart(sensors);
    
    // Update system status panel
    updateSystemStatus(statusResponse);
}

// Update sensor cards
function updateSensorCards(sensors) {
    if (!sensors || sensors.length === 0) {
        sensorsContainer.innerHTML = `
            <div class="no-data-message">
                <i class="fas fa-water"></i>
                <p>No sensors connected</p>
            </div>
        `;
        return;
    }
    
    let html = '';
    sensors.forEach(sensor => {
        let categoryClass = '';
        let statusText = '';
        
        switch(sensor.category) {
            case 0: 
                categoryClass = 'normal'; 
                statusText = 'Normal';
                break;
            case 1: 
                categoryClass = 'warning'; 
                statusText = 'Warning';
                break;
            case 2: 
                categoryClass = 'alert'; 
                statusText = 'Alert';
                break;
            default: 
                categoryClass = ''; 
                statusText = 'Unknown';
        }
        
        html += `
            <div class="sensor-card ${categoryClass}" data-mac="${sensor.mac}">
                <h3>${sensor.name}</h3>
                <p><i class="fas fa-water"></i> Water Level: ${sensor.waterLevel.toFixed(1)} cm</p>
                <p><i class="fas fa-temperature-high"></i> Temperature: ${sensor.temperature.toFixed(1)}°C</p>
                <p><i class="fas fa-info-circle"></i> Status: ${statusText}</p>
                <p><i class="fas fa-clock"></i> ${formatTimeSince(sensor.lastSeenSeconds)}</p>
            </div>
        `;
    });
    
    sensorsContainer.innerHTML = html;
    
    // Add event listeners to sensor cards
    document.querySelectorAll('.sensor-card').forEach(card => {
        card.addEventListener('click', () => {
            const mac = card.getAttribute('data-mac');
            const sensor = sensors.find(s => s.mac === mac);
            if (sensor) {
                showSensorDetail(sensor);
            }
        });
    });
}

// Update water level chart
function updateWaterLevelChart(sensors) {
    const ctx = document.getElementById('water-level-chart').getContext('2d');
    
    // Sort sensors by water level (descending)
    const sortedSensors = [...sensors].sort((a, b) => b.waterLevel - a.waterLevel);
    
    // Prepare data for chart
    const labels = sortedSensors.map(sensor => sensor.name);
    const data = sortedSensors.map(sensor => sensor.waterLevel);
    const backgroundColor = sortedSensors.map(sensor => {
        switch(sensor.category) {
            case 0: return 'rgba(16, 185, 129, 0.6)'; // success
            case 1: return 'rgba(251, 191, 36, 0.6)'; // warning
            case 2: return 'rgba(239, 68, 68, 0.6)';  // danger
            default: return 'rgba(148, 163, 184, 0.6)'; // gray
        }
    });
    
    // Create or update chart
    if (waterLevelChart) {
        waterLevelChart.data.labels = labels;
        waterLevelChart.data.datasets[0].data = data;
        waterLevelChart.data.datasets[0].backgroundColor = backgroundColor;
        waterLevelChart.update();
    } else {
        waterLevelChart = new Chart(ctx, {
            type: 'bar',
            data: {
                labels: labels,
                datasets: [{
                    label: 'Water Level (cm)',
                    data: data,
                    backgroundColor: backgroundColor,
                    borderColor: 'rgba(255, 255, 255, 0.8)',
                    borderWidth: 1
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        display: false
                    },
                    tooltip: {
                        callbacks: {
                            afterLabel: function(context) {
                                const sensorIndex = context.dataIndex;
                                return `Temperature: ${sortedSensors[sensorIndex].temperature.toFixed(1)}°C`;
                            }
                        }
                    }
                },
                scales: {
                    y: {
                        beginAtZero: true,
                        title: {
                            display: true,
                            text: 'Water Level (cm)'
                        }
                    }
                }
            }
        });
    }
}

// Update system status panel
function updateSystemStatus(status) {
    if (!status) return;
    
    deviceNameEl.textContent = status.deviceName || '-';
    deviceMacEl.textContent = status.deviceMac || '-';
    apIpEl.textContent = status.wifi?.apIP || '-';
    
    // Wifi status
    if (status.wifi?.staConnected) {
        const rssiText = status.wifi.rssi ? ` (Signal: ${status.wifi.rssi} dBm)` : '';
        wifiStatusEl.innerHTML = `Connected to <strong>${status.wifi.staSSID || 'WiFi'}</strong>${rssiText}`;
    } else {
        wifiStatusEl.textContent = 'Not connected to WiFi';
    }
    
    systemUptimeEl.textContent = formatUptime(status.uptime);
    connectedPeersEl.textContent = `${status.connectedPeers || 0} / ${status.minPeers || 1}`;
}

// Show detailed sensor information in modal
function showSensorDetail(sensor) {
    const categoryClass = sensor.category === 0 ? 'success' : (sensor.category === 1 ? 'warning' : 'danger');
    const statusText = sensor.category === 0 ? 'Normal' : (sensor.category === 1 ? 'Warning' : 'Alert');
    
    const content = `
        <div class="sensor-detail">
            <div class="sensor-header bg-${categoryClass}" style="padding: 15px; color: white; border-radius: 5px;">
                <h3>${sensor.name}</h3>
                <p>Status: ${statusText}</p>
            </div>
            
            <div class="sensor-readings" style="display: flex; gap: 20px; margin-top: 20px;">
                <div class="reading-card">
                    <div class="reading-icon">
                        <i class="fas fa-water"></i>
                    </div>
                    <div class="reading-value">${sensor.waterLevel.toFixed(1)}</div>
                    <div class="reading-label">Water Level (cm)</div>
                </div>
                
                <div class="reading-card">
                    <div class="reading-icon">
                        <i class="fas fa-temperature-high"></i>
                    </div>
                    <div class="reading-value">${sensor.temperature.toFixed(1)}</div>
                    <div class="reading-label">Temperature (°C)</div>
                </div>
            </div>
            
            <div class="sensor-meta" style="margin-top: 20px;">
                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 10px;">
                    <div>
                        <h4>MAC Address</h4>
                        <p>${sensor.mac}</p>
                    </div>
                    <div>
                        <h4>Last Seen</h4>
                        <p>${formatTimeSince(sensor.lastSeenSeconds)}</p>
                    </div>
                </div>
            </div>
        </div>
    `;
    
    showModal(`Sensor: ${sensor.name}`, content);
}

// Show modal with dynamic content
function showModal(title, content) {
    modalTitle.textContent = title;
    modalBody.innerHTML = content;
    modalContainer.classList.add('active');
}

// Hide modal
function hideModal() {
    modalContainer.classList.remove('active');
}

// Create sensors overview for modal
function createSensorsModalContent() {
    if (!sensorsData || sensorsData.length === 0) {
        return `
            <div class="no-data-message">
                <i class="fas fa-water"></i>
                <p>No sensors connected</p>
            </div>
        `;
    }
    
    let content = `
        <div class="sensors-table-container">
            <table class="sensors-table">
                <thead>
                    <tr>
                        <th>Status</th>
                        <th>Name</th>
                        <th>Water Level</th>
                        <th>Temperature</th>
                        <th>Last Seen</th>
                    </tr>
                </thead>
                <tbody>
    `;
    
    sensorsData.forEach(sensor => {
        const categoryClass = sensor.category === 0 ? 'success' : (sensor.category === 1 ? 'warning' : 'danger');
        const statusText = sensor.category === 0 ? 'Normal' : (sensor.category === 1 ? 'Warning' : 'Alert');
        
        content += `
            <tr>
                <td><span class="badge bg-${categoryClass}">${statusText}</span></td>
                <td>${sensor.name}</td>
                <td>${sensor.waterLevel.toFixed(1)} cm</td>
                <td>${sensor.temperature.toFixed(1)}°C</td>
                <td>${formatTimeSince(sensor.lastSeenSeconds)}</td>
            </tr>
        `;
    });
    
    content += `
                </tbody>
            </table>
        </div>
    `;
    
    return content;
}

// Create chart modal content
function createChartModalContent() {
    return `
        <div style="height: 400px;">
            <canvas id="modal-water-chart"></canvas>
        </div>
    `;
}

// Start the dashboard when DOM is ready
document.addEventListener('DOMContentLoaded', initDashboard);