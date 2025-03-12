/* /data/scripts.js */

// Simple error logging function
function logError(error) {
    console.error(error);
    const errElem = document.getElementById('errorLog');
    if (errElem) {
        errElem.textContent = error.toString();
    }
}

// Basic fetch wrapper with error handling
async function fetchWithError(url, options = {}) {
    try {
        const response = await fetch(url, options);
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        return await response.json();
    } catch (error) {
        logError(error);
        throw error;
    }
}

// Update status values
async function updateStatus() {
    try {
        const data = await fetchWithError('/status');
        // Make sure these IDs exist in your HTML
        document.getElementById('temperature').textContent = data.temperature.toFixed(1);
        document.getElementById('humidity').textContent = data.humidity.toFixed(1);
        document.getElementById('pressure').textContent = data.pressure.toFixed(1);
    } catch (error) {
        logError('Status update failed: ' + error);
    }
}

// Set temperature
window.setSetpoint = async function setSetpoint() {
    try {
        const value = document.getElementById('setpoint').value;
        const csrfToken = document.querySelector('meta[name="csrf-token"]').content;
        
        await fetch('/setpoint', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
                'X-CSRF-Token': csrfToken
            },
            body: `setpoint=${value}`
        });
        updateStatus();
    } catch (error) {
        logError('Failed to set temperature: ' + error);
    }
}

// Set mode
window.setMode = async function setMode() {
    try {
        const mode = document.getElementById('mode').value;
        const csrfToken = document.querySelector('meta[name="csrf-token"]').content;
        
        await fetch('/mode', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
                'X-CSRF-Token': csrfToken
            },
            body: `mode=${mode}`
        });
        updateStatus();
    } catch (error) {
        logError('Failed to set mode: ' + error);
    }
}

// Save configuration
window.saveConfig = async function saveConfig() {
    try {
        const form = document.getElementById('configForm');
        const data = new FormData(form);
        const json = Object.fromEntries(data.entries());
        const csrfToken = document.querySelector('meta[name="csrf-token"]').content;
        
        const response = await fetch('/save', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'X-CSRF-Token': csrfToken
            },
            body: JSON.stringify(json)
        });
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        try {
            const result = await response.json();
            if (result.status === "ok") {
                alert("Configuration saved successfully");
                location.reload();
            } else {
                alert("Error: " + (result.message || "Unknown error"));
            }
        } catch (jsonError) {
            // Fall back to text response if JSON parsing fails
            const text = await response.text();
            alert("Configuration saved");
            location.reload();
        }
    } catch (error) {
        logError('Failed to save configuration: ' + error);
    }
}

// Factory reset
window.factoryReset = async function factoryReset() {
    if (confirm('Are you sure you want to reset to factory defaults?')) {
        try {
            const csrfToken = document.querySelector('meta[name="csrf-token"]').content;
            // If your route is "/factory_reset" instead of "/reset", update here:
            await fetch('/factory_reset', { 
                method: 'POST',
                headers: {
                    'X-CSRF-Token': csrfToken
                }
            });
            location.reload();
        } catch (error) {
            logError('Failed to perform factory reset: ' + error);
        }
    }
}

// (Optional) Example updatePID function if needed
window.updatePID = async function updatePID() {
    try {
        // Get form values
        const kp = parseFloat(document.getElementById('kp').value);
        const ki = parseFloat(document.getElementById('ki').value);
        const kd = parseFloat(document.getElementById('kd').value);
        const pidActive = document.getElementById('pidActive').checked;
        const csrfToken = document.querySelector('meta[name="csrf-token"]').content;
        
        // Validate input values
        if (isNaN(kp) || isNaN(ki) || isNaN(kd)) {
            throw new Error('Invalid PID parameters. Please enter valid numbers.');
        }
        
        // Construct JSON payload
        const payload = {
            kp: kp,
            ki: ki,
            kd: kd,
            active: pidActive
        };
        
        // URL-encode the JSON payload with the key "plain"
        const bodyData = new URLSearchParams();
        bodyData.append('plain', JSON.stringify(payload));
        
        // Send the request
        const response = await fetch('/pid', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
                'X-CSRF-Token': csrfToken
            },
            body: bodyData
        });
        
        // Check for errors in the response
        if (!response.ok) {
            const errorData = await response.json();
            throw new Error(errorData.error || `HTTP error! status: ${response.status}`);
        }

        console.log('PID update successful');
        location.reload();

        alert('PID parameters updated successfully');
        
        // Optionally, update the UI dynamically
        updateUIWithCurrentConfig();
    } catch (error) {
        logError('Failed to update PID: ' + error);
        // Log the error to the console
        console.error('Failed to update PID:', error);

        // Show a user-friendly error message
        alert('Failed to update PID: ' + error.message);
    }
}

async function updateUIWithCurrentConfig() {
    try {
        const response = await fetch('/config');
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const config = await response.json();
        
        // Update the UI with the new configuration
        document.getElementById('kp').value = config.pid.kp;
        document.getElementById('ki').value = config.pid.ki;
        document.getElementById('kd').value = config.pid.kd;
        document.getElementById('pidActive').checked = config.pid.active;
    } catch (error) {
        logError('Failed to update UI: ' + error);
    }
}

// Initial status update and periodic refresh
updateStatus();
setInterval(updateStatus, 10000);

async function showConfig() {
    try {
        const response = await fetch('/config');
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const config = await response.json();
        document.getElementById('configContents').textContent = JSON.stringify(config, null, 2);
    } catch (error) {
        logError('Failed to fetch config: ' + error);
    }
}
