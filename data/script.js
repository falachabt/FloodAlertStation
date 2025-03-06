// Fetch sensor data from API
function fetchSensors() {
    fetch('/api/sensors')
        .then(response => response.json())
        .then(data => {
            updateSensors(data);
        })
        .catch(error => console.error('Error fetching sensors:', error));
}

// Update the sensors display
function updateSensors(data) {
    const container = document.getElementById('sensors');
    
    if (!data.sensors || data.sensors.length === 0) {
        container.innerHTML = '<p>No sensors connected</p>';
        return;
    }
    
    let html = '';
    data.sensors.forEach(sensor => {
        let categoryClass = '';
        switch(sensor.category) {
            case 0: categoryClass = 'normal'; break;
            case 1: categoryClass = 'warning'; break;
            case 2: categoryClass = 'alert'; break;
        }
        
        html += `
            <div class="sensor-card ${categoryClass}">
                <h3>${sensor.name}</h3>
                <p>Water Level: ${sensor.waterLevel.toFixed(1)} cm</p>
                <p>Temperature: ${sensor.temperature.toFixed(1)}°C</p>
                <p>Status: ${sensor.status}</p>
                <p>Last Seen: ${sensor.lastSeenSeconds} seconds ago</p>
            </div>
        `;
    });
    
    container.innerHTML = html;
    
    // Update status
    document.getElementById('status').innerHTML = `
        <p>Network: ${data.networkReady ? 'Ready' : 'Not Ready'}</p>
        <p>Connected Peers: ${data.connectedPeers}</p>
    `;
}

// Refresh data every 5 seconds
document.addEventListener('DOMContentLoaded', function() {
    fetchSensors();
    setInterval(fetchSensors, 5000);
});