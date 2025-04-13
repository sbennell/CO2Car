// Race Management System - Main JavaScript

document.addEventListener('DOMContentLoaded', function() {
    // Initialize Socket.IO connection if on a page that requires it
    initializeSocketIO();
    
    // Initialize any tooltips
    initializeTooltips();
    
    // Initialize any race control buttons
    initializeRaceControls();
});

/**
 * Initialize Socket.IO connection and event listeners
 */
function initializeSocketIO() {
    // Check if we're on a page that needs real-time updates
    if (document.getElementById('race-dashboard') || document.getElementById('race-control')) {
        // Connect to the Socket.IO server
        const socket = io();
        
        // Connection events
        socket.on('connect', function() {
            console.log('WebSocket connected');
            updateConnectionStatus(true);
        });
        
        socket.on('disconnect', function() {
            console.log('WebSocket disconnected');
            updateConnectionStatus(false);
        });
        
        // Race events
        socket.on('race_completed', function(data) {
            console.log('Race completed:', data);
            updateRaceResults(data);
        });
        
        // Device status events
        socket.on('device_status', function(data) {
            console.log('Device status update:', data);
            updateDeviceStatus(data);
        });
        
        // Error events
        socket.on('device_error', function(data) {
            console.error('Device error:', data);
            showErrorNotification(data.message);
        });
    }
}

/**
 * Update the connection status indicator
 */
function updateConnectionStatus(isConnected) {
    const statusIndicator = document.getElementById('connection-status');
    if (statusIndicator) {
        if (isConnected) {
            statusIndicator.classList.remove('text-danger');
            statusIndicator.classList.add('text-success');
            statusIndicator.innerHTML = '<i class="bi bi-wifi"></i> Connected';
        } else {
            statusIndicator.classList.remove('text-success');
            statusIndicator.classList.add('text-danger');
            statusIndicator.innerHTML = '<i class="bi bi-wifi-off"></i> Disconnected';
        }
    }
}

/**
 * Update race results on the page
 */
function updateRaceResults(data) {
    const resultsContainer = document.getElementById('race-results');
    if (resultsContainer) {
        // Find or create result row
        let resultRow = document.getElementById(`race-${data.race_id}`);
        if (!resultRow) {
            resultRow = document.createElement('tr');
            resultRow.id = `race-${data.race_id}`;
            resultsContainer.appendChild(resultRow);
        }
        
        // Format times for display
        const car1Time = (data.car1_time / 1000).toFixed(3);
        const car2Time = (data.car2_time / 1000).toFixed(3);
        
        // Determine winner
        let winnerText = "";
        if (data.winner === 'car1') {
            winnerText = "Lane 1";
        } else if (data.winner === 'car2') {
            winnerText = "Lane 2";
        } else {
            winnerText = "Tie";
        }
        
        // Update the row content
        resultRow.innerHTML = `
            <td>${data.race_id}</td>
            <td>${car1Time}s</td>
            <td>${car2Time}s</td>
            <td>${winnerText}</td>
            <td>${new Date().toLocaleTimeString()}</td>
        `;
        
        // Highlight the row to indicate it's new
        resultRow.classList.add('table-info');
        setTimeout(() => {
            resultRow.classList.remove('table-info');
        }, 3000);
    }
}

/**
 * Update device status indicators
 */
function updateDeviceStatus(data) {
    // Update sensor statuses
    if (data.sensors) {
        const sensor1Status = document.getElementById('sensor1-status');
        const sensor2Status = document.getElementById('sensor2-status');
        
        if (sensor1Status && data.sensors.sensor1 !== undefined) {
            updateStatusIndicator(sensor1Status, data.sensors.sensor1);
        }
        
        if (sensor2Status && data.sensors.sensor2 !== undefined) {
            updateStatusIndicator(sensor2Status, data.sensors.sensor2);
        }
    }
    
    // Update race state
    if (data.race_state) {
        const raceStateElement = document.getElementById('race-state');
        if (raceStateElement) {
            raceStateElement.textContent = data.race_state;
            
            // Update class based on state
            raceStateElement.className = '';
            raceStateElement.classList.add(
                data.race_state === 'racing' ? 'text-warning' :
                data.race_state === 'finished' ? 'text-success' :
                data.race_state === 'ready' ? 'text-primary' : 'text-secondary'
            );
        }
    }
}

/**
 * Update a status indicator element based on a boolean status
 */
function updateStatusIndicator(element, isOk) {
    if (isOk) {
        element.classList.remove('text-danger');
        element.classList.add('text-success');
        element.innerHTML = '<i class="bi bi-check-circle-fill"></i> OK';
    } else {
        element.classList.remove('text-success');
        element.classList.add('text-danger');
        element.innerHTML = '<i class="bi bi-exclamation-triangle-fill"></i> Error';
    }
}

/**
 * Initialize Bootstrap tooltips
 */
function initializeTooltips() {
    const tooltipTriggerList = [].slice.call(document.querySelectorAll('[data-bs-toggle="tooltip"]'));
    tooltipTriggerList.map(function (tooltipTriggerEl) {
        return new bootstrap.Tooltip(tooltipTriggerEl);
    });
}

/**
 * Initialize race control buttons
 */
function initializeRaceControls() {
    const loadButton = document.getElementById('load-button');
    const startButton = document.getElementById('start-button');
    
    if (loadButton) {
        loadButton.addEventListener('click', function() {
            fetch('/api/command/load', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                }
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    console.log('Load command sent successfully');
                } else {
                    console.error('Error sending load command:', data.error);
                    showErrorNotification(data.error);
                }
            })
            .catch(error => {
                console.error('Error sending load command:', error);
                showErrorNotification('Network error when sending command');
            });
        });
    }
    
    if (startButton) {
        startButton.addEventListener('click', function() {
            fetch('/api/command/start', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                }
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    console.log('Start command sent successfully');
                } else {
                    console.error('Error sending start command:', data.error);
                    showErrorNotification(data.error);
                }
            })
            .catch(error => {
                console.error('Error sending start command:', error);
                showErrorNotification('Network error when sending command');
            });
        });
    }
}

/**
 * Show an error notification to the user
 */
function showErrorNotification(message) {
    // Check if we have a notification container
    let notificationContainer = document.getElementById('notification-container');
    if (!notificationContainer) {
        // Create notification container if it doesn't exist
        notificationContainer = document.createElement('div');
        notificationContainer.id = 'notification-container';
        notificationContainer.style.position = 'fixed';
        notificationContainer.style.top = '20px';
        notificationContainer.style.right = '20px';
        notificationContainer.style.zIndex = '1050';
        document.body.appendChild(notificationContainer);
    }
    
    // Create the notification element
    const notification = document.createElement('div');
    notification.className = 'toast align-items-center text-white bg-danger border-0';
    notification.role = 'alert';
    notification.setAttribute('aria-live', 'assertive');
    notification.setAttribute('aria-atomic', 'true');
    
    // Set the notification content
    notification.innerHTML = `
        <div class="d-flex">
            <div class="toast-body">
                <i class="bi bi-exclamation-triangle-fill me-2"></i>
                ${message}
            </div>
            <button type="button" class="btn-close btn-close-white me-2 m-auto" data-bs-dismiss="toast" aria-label="Close"></button>
        </div>
    `;
    
    // Add the notification to the container
    notificationContainer.appendChild(notification);
    
    // Initialize and show the toast
    const toast = new bootstrap.Toast(notification, {
        autohide: true,
        delay: 5000
    });
    toast.show();
    
    // Remove the notification after it's hidden
    notification.addEventListener('hidden.bs.toast', function() {
        notification.remove();
    });
} 