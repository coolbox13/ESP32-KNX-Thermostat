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
async function setSetpoint() {
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
async function setMode() {
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
async function saveConfig() {
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
async function factoryReset() {
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
async function updatePID() {
    try {
        const kp = document.getElementById('kp').value;
        const ki = document.getElementById('ki').value;
        const kd = document.getElementById('kd').value;
        const active = document.getElementById('pidActive').checked;
        const csrfToken = document.querySelector('meta[name="csrf-token"]').content;

        const payload = {
            kp: parseFloat(kp),
            ki: parseFloat(ki),
            kd: parseFloat(kd),
            active: active
        };

        const response = await fetch('/pid', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'X-CSRF-Token': csrfToken
            },
            body: JSON.stringify(payload)
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        console.log('PID update successful');
        location.reload();

    } catch (error) {
        logError('Failed to update PID: ' + error);
    }
}

// Initial status update and periodic refresh
updateStatus();
setInterval(updateStatus, 10000);
