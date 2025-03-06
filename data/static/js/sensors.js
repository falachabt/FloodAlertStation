/**
 * Flood Alert System - Sensors Management JavaScript
 * Handles data fetching, visualization, and UI interaction for the sensors page
 */

// DOM Elements
const refreshBtn = document.getElementById('refresh-btn');
const lastUpdatedEl = document.getElementById('last-updated');
const networkIndicator = document.getElementById('network-indicator');
const networkText = document.getElementById('network-text');
const systemTimeEl = document.getElementById('system-time');
const sensorsTableBody = document.getElementById('sensors-table-body');
const searchInput = document.getElementById('search-sensors');
const sensorDetailsPanel = document.getElementById('sensor-details-panel');
const closeDetailsBtn = document.getElementById('close-details');
const detailSensorName = document.getElementById('detail-sensor-name');
const detailName = document.getElementById('detail-name');
const detailMac = document.getElementById('detail-mac');
const detailStatus = document.getElementById('detail-status');
const detailLastSeen = document.getElementById('detail-last-seen');
const detailWaterLevel = document.getElementById('detail-water-level');
const detailTemperature = document.getElementById('detail-temperature');

// Chart instance
let historyChart = null;

// Data storage
let sensorsData = [];
let selectedSensor = null;

// Mock history data (In a real implementation, this would come from an API)
const mockHistoryData = {
    timestamps: [
        Date.now() - 86400000, // 24 hours ago
        Date.now() - 72000000, // 20 hours ago
        Date.now() - 57600000, // 16 hours ago
        Date.now() - 43200000, // 12 hours ago
        Date.now() - 28800000, // 8 hours ago
        Date.now() - 14400000, // 4 hours ago
        Date.now()              // now
    ],
    waterLevels: [25, 30, 40, 45, 52, 60, 65],
    temperatures: [22, 22.5, 23, 23.5, 24, 24.5, 25]
};

// Initialize the sensors page
function initSensorsPage() {
    // Set up event listeners
    refreshBtn.addEventListener('click', fetchSensorsData);
    searchInput.addEventListener('input', filterSensors);
    closeDetailsBtn.addEventListener('click', hideSensorDetails);
    
    // Setup map sensor location clicks
    document.querySelectorAll('.sensor-location').forEach(location => {
        location.addEventListener('click', () => {
            const sensorName = location.getAttribute('data-sensor');
            // Find sensor by name and show details
            const sensor = sensorsData.find(s => s.name === sensorName);
            if (sensor) {
                showSensorDetails(sensor);
            } else {
                // If real data is not available, use mock data for demo
                showSensorDetails({
                    name: sensorName,
                    mac: "00:00:00:00:00:00",
                    waterLevel: 45.2,
                    temperature: 23.5,
                    category: location.querySelector('.location-dot').classList.contains('warning') ? 1 : 0,
                    lastSeenSeconds: 180
                });
            }
        });
    });
    
    // Update time every second
    setInterval(updateTime, 1000);
    
    // Initial data fetch
    fetchSensorsData();
    
    // Auto-refresh data every 30 seconds
    setInterval(fetchSensorsData, 30000);
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

// Fetch sensor data
function fetchSensorsData() {
    fetch('/api/sensors')
        .then(response => response.json())
        .then(data => {
            // Update data store
            sensorsData = data.sensors || [];
            
            // Update UI with fetched data
            updateSensorsTable(sensorsData);
            
            // Update map indicators
            updateSensorMap(sensorsData);
            
            // Update network status
            updateNetworkStatus(data.networkReady);
            
            // Update last updated time
            lastUpdatedEl.textContent = formatLastUpdated(data.timestamp);
            
            // If a sensor is selected, refresh its details
            if (selectedSensor) {
                const updatedSensor = sensorsData.find(s => s.mac === selectedSensor.mac);
                if (updatedSensor) {
                    showSensorDetails(updatedSensor);
                }
            }
        })
        .catch(error => {
            console.error('Error fetching sensor data:', error);
            updateNetworkStatus(false);
            
            // If the API fails, show mock data for demo purposes
            const mockSensors = [
                { name: "Sensor 1", mac: "AA:BB:CC:DD:EE:01", waterLevel: 35.2, temperature: 22.5, category: 0, lastSeenSeconds: 120 },
                { name: "Sensor 2", mac: "AA:BB:CC:DD:EE:02", waterLevel: 62.7, temperature: 23.8, category: 1, lastSeenSeconds: 45 },
                { name: "Sensor 3", mac: "AA:BB:CC:DD:EE:03", waterLevel: 42.1, temperature: 21.3, category: 0, lastSeenSeconds: 180 },
                { name: "Sensor 4", mac: "AA:BB:CC:DD:EE:04", waterLevel: 38.5, temperature: 22.0, category: 0, lastSeenSeconds: 300 }
            ];
            
            sensorsData = mockSensors;
            updateSensorsTable(mockSensors);
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

// Update sensors table
function updateSensorsTable(sensors) {
    if (!sensors || sensors.length === 0) {
        sensorsTableBody.innerHTML = `
            <tr>
                <td colspan="7" class="text-center">
                    <div class="no-data-message">
                        <i class="fas fa-water"></i>
                        <p>No sensors connected</p>
                    </div>
                </td>
            </tr>
        `;
        return;
    }
    
    let html = '';
    sensors.forEach(sensor => {
        const statusClass = sensor.category === 0 ? 'success' : (sensor.category === 1 ? 'warning' : 'danger');
        const statusText = sensor.category === 0 ? 'Normal' : (sensor.category === 1 ? 'Warning' : 'Alert');
        
        html += `
            <tr data-mac="${sensor.mac}">
                <td><span class="badge bg-${statusClass}">${statusText}</span></td>
                <td>${sensor.name}</td>
                <td>${sensor.mac}</td>
                <td>${sensor.waterLevel.toFixed(1)} cm</td>
                <td>${sensor.temperature.toFixed(1)}°C</td>
                <td>${formatTimeSince(sensor.lastSeenSeconds)}</td>
                <td>
                    <button class="btn btn-sm btn-light view-details-btn" data-mac="${sensor.mac}">
                        <i class="fas fa-info-circle"></i> Details
                    </button>
                </td>
            </tr>
        `;
    });
    
    sensorsTableBody.innerHTML = html;
    
    // Add event listeners to view details buttons
    document.querySelectorAll('.view-details-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            e.preventDefault();
            const mac = btn.getAttribute('data-mac');
            const sensor = sensors.find(s => s.mac === mac);
            if (sensor) {
                showSensorDetails(sensor);
            }
        });
    });
}

// Update sensor map locations
function updateSensorMap(sensors) {
    // For a real implementation, this would place sensors on an actual map
    // For this demo, we'll just update the existing placeholder dots
    const sensorLocations = document.querySelectorAll('.sensor-location');
    
    // If we have real sensors data, update the mock locations with real data
    if (sensors && sensors.length > 0) {
        // Update the first few locations with real data
        for (let i = 0; i < Math.min(sensorLocations.length, sensors.length); i++) {
            const location = sensorLocations[i];
            const sensor = sensors[i];
            
            // Update location name
            location.setAttribute('data-sensor', sensor.name);
            location.querySelector('.location-label').textContent = sensor.name;
            
            // Update status dot
            const dot = location.querySelector('.location-dot');
            dot.className = 'location-dot';
            
            switch(sensor.category) {
                case 0: dot.classList.add('normal'); break;
                case 1: dot.classList.add('warning'); break;
                case 2: dot.classList.add('alert'); break;
                default: dot.classList.add('normal');
            }
        }
    }
}

// Filter sensors based on search input
function filterSensors() {
    const searchTerm = searchInput.value.toLowerCase();
    
    // If no search term, show all
    if (!searchTerm) {
        updateSensorsTable(sensorsData);
        return;
    }
    
    // Filter sensors by name or MAC address
    const filteredSensors = sensorsData.filter(sensor => 
        sensor.name.toLowerCase().includes(searchTerm) || 
        sensor.mac.toLowerCase().includes(searchTerm)
    );
    
    updateSensorsTable(filteredSensors);
}

// Show sensor details
function showSensorDetails(sensor) {
    selectedSensor = sensor;
    
    // Update panel title
    detailSensorName.textContent = sensor.name;
    
    // Update basic information
    detailName.textContent = sensor.name;
    detailMac.textContent = sensor.mac;
    
    const statusClass = sensor.category === 0 ? 'success' : (sensor.category === 1 ? 'warning' : 'danger');
    const statusText = sensor.category === 0 ? 'Normal' : (sensor.category === 1 ? 'Warning' : 'Alert');
    
    detailStatus.textContent = statusText;
    detailStatus.className = 'info-value';
    detailStatus.classList.add(`text-${statusClass}`);
    
    detailLastSeen.textContent = formatTimeSince(sensor.lastSeenSeconds);
    
    // Update current readings
    detailWaterLevel.textContent = sensor.waterLevel.toFixed(1);
    detailTemperature.textContent = sensor.temperature.toFixed(1);
    
    // Update history chart
    updateSensorHistoryChart(sensor);
    
    // Show the panel
    sensorDetailsPanel.classList.remove('hidden');
    
    // Scroll to panel
    sensorDetailsPanel.scrollIntoView({ behavior: 'smooth' });
}

// Hide sensor details panel
function hideSensorDetails() {
    sensorDetailsPanel.classList.add('hidden');
    selectedSensor = null;
}

// Update sensor history chart
function updateSensorHistoryChart(sensor) {
    const ctx = document.getElementById('sensor-history-chart').getContext('2d');
    
    // In a real implementation, we would fetch historical data for this specific sensor
    // For this demo, we'll use mock data with some randomization based on current values
    
    // Generate semi-random history data based on current values
    const historyData = {
        timestamps: [...mockHistoryData.timestamps],
        waterLevels: [],
        temperatures: []
    };
    
    // Generate water levels trending toward current level
    const currentWaterLevel = sensor.waterLevel;
    const startingWaterLevel = currentWaterLevel * 0.7; // 70% of current level
    
    for (let i = 0; i < historyData.timestamps.length - 1; i++) {
        const progress = i / (historyData.timestamps.length - 1);
        const level = startingWaterLevel + (currentWaterLevel - startingWaterLevel) * progress;
        // Add some randomness
        historyData.waterLevels.push(level + (Math.random() * 5 - 2.5));
    }
    // Add current level
    historyData.waterLevels.push(currentWaterLevel);
    
    // Generate temperatures trending toward current temperature
    const currentTemp = sensor.temperature;
    const startingTemp = currentTemp - 2;
    
    for (let i = 0; i < historyData.timestamps.length - 1; i++) {
        const progress = i / (historyData.timestamps.length - 1);
        const temp = startingTemp + (currentTemp - startingTemp) * progress;
        // Add some randomness
        historyData.temperatures.push(temp + (Math.random() * 0.8 - 0.4));
    }
    // Add current temperature
    historyData.temperatures.push(currentTemp);
    
    // Format timestamps
    const labels = historyData.timestamps.map(ts => {
        const date = new Date(ts);
        return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
    });
    
    // Create or update chart
    if (historyChart) {
        historyChart.data.labels = labels;
        historyChart.data.datasets[0].data = historyData.waterLevels;
        historyChart.data.datasets[1].data = historyData.temperatures;
        historyChart.update();
    } else {
        historyChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [
                    {
                        label: 'Water Level (cm)',
                        data: historyData.waterLevels,
                        borderColor: '#0466c8',
                        backgroundColor: 'rgba(4, 102, 200, 0.1)',
                        borderWidth: 2,
                        fill: true,
                        tension: 0.4,
                        yAxisID: 'y'
                    },
                    {
                        label: 'Temperature (°C)',
                        data: historyData.temperatures,
                        borderColor: '#ef4444',
                        backgroundColor: 'rgba(239, 68, 68, 0.05)',
                        borderWidth: 2,
                        fill: true,
                        tension: 0.4,
                        yAxisID: 'y1'
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                interaction: {
                    mode: 'index',
                    intersect: false,
                },
                plugins: {
                    tooltip: {
                        enabled: true,
                    },
                    legend: {
                        position: 'top',
                    }
                },
                scales: {
                    x: {
                        title: {
                            display: true,
                            text: 'Time'
                        }
                    },
                    y: {
                        type: 'linear',
                        display: true,
                        position: 'left',
                        title: {
                            display: true,
                            text: 'Water Level (cm)'
                        },
                        min: Math.max(0, Math.floor(startingWaterLevel * 0.8)),
                    },
                    y1: {
                        type: 'linear',
                        display: true,
                        position: 'right',
                        title: {
                            display: true,
                            text: 'Temperature (°C)'
                        },
                        grid: {
                            drawOnChartArea: false,
                        },
                    }
                }
            }
        });
    }
}

// Start the page when DOM is ready
document.addEventListener('DOMContentLoaded', initSensorsPage);