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

window.saveConfig = async function saveConfig() {
    try {
        // Build JSON directly from form fields using exact IDs
        const jsonData = {
            device: {
                name: document.getElementById('device.name').value,
                sendInterval: parseInt(document.getElementById('device.sendInterval').value)
            },
            knx: {
                enabled: document.getElementById('knx.enabled').checked,
                physical: {
                    area: parseInt(document.getElementById('knx.physical.area').value),
                    line: parseInt(document.getElementById('knx.physical.line').value),
                    member: parseInt(document.getElementById('knx.physical.member').value)
                }
            },
            mqtt: {
                enabled: document.getElementById('mqtt.enabled').checked,
                server: document.getElementById('mqtt.server').value,
                port: parseInt(document.getElementById('mqtt.port').value),
                username: document.getElementById('mqtt.username').value,
                password: document.getElementById('mqtt.password').value,
                clientId: document.getElementById('mqtt.clientId').value,
                topicPrefix: document.getElementById('mqtt.topicPrefix').value
            },
            web: {
                username: "admin",
                password: "admin"
            }
        };
        
        console.log('Saving configuration:', jsonData);
        
        const csrfToken = document.querySelector('meta[name="csrf-token"]').content;
        
        const response = await fetch('/save', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'X-CSRF-Token': csrfToken
            },
            body: JSON.stringify(jsonData)
        });
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const result = await response.json();
        alert("Configuration saved successfully");
        location.reload();
    } catch (error) {
        console.error('Failed to save configuration:', error);
        alert('Error saving configuration: ' + error.message);
    }
}

// Alternative direct configuration approach
window.saveConfigDirect = async function saveConfigDirect() {
    try {
        // Create a test configuration to bypass form processing
        const testConfig = {
            device: {
                name: "Thermostat-Test",
                sendInterval: 1000
            },
            knx: {
                enabled: false,
                physical: {
                    area: 1,
                    line: 1,
                    member: 159
                }
            },
            mqtt: {
                enabled: false,
                server: "192.168.178.35",
                port: 1883,
                username: "",
                password: "",
                clientId: "edummy",
                topicPrefix: "esp32/thermostat/"
            },
            web: {
                username: "admin",
                password: "admin"
            }
        };
        
        console.log('Saving test configuration:', testConfig);
        
        const csrfToken = document.querySelector('meta[name="csrf-token"]').content;
        
        const response = await fetch('/save', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'X-CSRF-Token': csrfToken
            },
            body: JSON.stringify(testConfig)
        });
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        alert("Test configuration saved successfully");
        location.reload();
    } catch (error) {
        console.error('Failed to save test configuration:', error);
        alert('Error saving test configuration: ' + error.message);
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
        
        const rawContent = await response.text();
        
        // Log the raw content to the browser console
        console.log('Raw config file content:', rawContent);
        
        // Display the raw content in the preformatted block
        document.getElementById('configContents').textContent = rawContent;
    } catch (error) {
        console.error('Failed to fetch config:', error);
        document.getElementById('configContents').textContent = 'Error: ' + error.message;
    }
}

// Save a minimal config file to get started
window.createMinimalConfig = async function createMinimalConfig() {
    try {
        const minimalConfig = {
            deviceName: "Thermostat",
            updateInterval: 1000,
            knxEnabled: false,
            mqttEnabled: false,
            web: {
                username: "admin",
                password: "admin"
            },
            pid: {
                kp: 2.0,
                ki: 0.5,
                kd: 1.0,
                minOutput: 0,
                maxOutput: 100,
                sampleTime: 30000
            }
        };
        
        const csrfToken = document.querySelector('meta[name="csrf-token"]').content;
        
        // Use a different endpoint that directly creates the file
        const response = await fetch('/create_config', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'X-CSRF-Token': csrfToken
            },
            body: JSON.stringify(minimalConfig)
        });
        
        alert("Created minimal config file");
        location.reload();
    } catch (error) {
        console.error('Failed to create config:', error);
        alert('Error creating config: ' + error.message);
    }
}
