#include "web/html_generator.h"
#include "config_manager.h"

String HtmlGenerator::generatePage(
    ThermostatState* state,
    ConfigInterface* config,
    ControlInterface* control,
    const String& csrfToken) {
    
    String html = generateHeader(csrfToken);
    html += generateNavigation();
    html += "<div class='container'>";
    html += generateStatusSection(state);
    html += generateControlSection(state, csrfToken);
    html += generateConfigSection(config, csrfToken);
    if (control) {
        html += generatePIDSection(control, csrfToken);
    }
    html += "</div>";
    html += generateFooter();
    return html;
}

String HtmlGenerator::generateHeader(const String& csrfToken) {
    return R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <meta name="csrf-token" content=")" + csrfToken + R"(">
    <title>KNX Thermostat V1.0</title>
    <!-- Load custom CSS from LittleFS -->
    <link rel='stylesheet' href='/style.css'>
    <link rel='stylesheet' href='/style.css'>
</head>
<body>
)";
}

String HtmlGenerator::generateNavigation() {
    return R"(
<nav class="navbar navbar-expand-lg navbar-dark bg-dark">
    <div class="container-fluid">
        <a class="navbar-brand" href="#">KNX Thermostat V1.0</a>
        <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarNav">
            <span class="navbar-toggler-icon"></span>
        </button>
        <div class="collapse navbar-collapse" id="navbarNav">
            <ul class="navbar-nav">
                <li class="nav-item">
                    <a class="nav-link" href="#status">Status</a>
                </li>
                <li class="nav-item">
                    <a class="nav-link" href="#control">Control</a>
                </li>
                <li class="nav-item">
                    <a class="nav-link" href="#config">Configuration</a>
                </li>
            </ul>
        </div>
    </div>
</nav>
)";
}

String HtmlGenerator::generateStatusSection(ThermostatState* state) {
    if (!state) return "";
    
    // Increase buffer to ensure complete HTML is generated
    char buf[1024];
    snprintf(buf, sizeof(buf), R"(
<div id="status" class="section">
    <h2>Status</h2>
    <div class="row">
        <div class="col-md-4">
            <div class="card">
                <div class="card-body">
                    <h5 class="card-title">Temperature</h5>
                    <p id="temperature" class="card-text temperature">%.1f°C</p>
                </div>
            </div>
        </div>
        <div class="col-md-4">
            <div class="card">
                <div class="card-body">
                    <h5 class="card-title">Humidity</h5>
                    <p id="humidity" class="card-text humidity">%.1f%%</p>
                </div>
            </div>
        </div>
        <div class="col-md-4">
            <div class="card">
                <div class="card-body">
                    <h5 class="card-title">Pressure</h5>
                    <p id="pressure" class="card-text pressure">%.1f hPa</p>
                </div>
            </div>
        </div>
    </div>
</div>
)",
        state->getCurrentTemperature(),
        state->getCurrentHumidity(),
        state->getCurrentPressure()
    );
    return String(buf);
}

String HtmlGenerator::generateControlSection(ThermostatState* state, const String& csrfToken) {
    if (!state) return "";

    String html;
    html += "<div id='control' class='section container mt-4'>\n";
    html += "  <div class='card'>\n";
    html += "    <div class='card-header'>\n";
    html += "      <h2>Control</h2>\n";
    html += "    </div>\n";
    html += "    <div class='card-body'>\n";
    // Add hidden CSRF token field for forms if needed
    html += "      <input type='hidden' name='_csrf' value='" + csrfToken + "'>\n";
    // Mode selection control
    html += "      <div class='mb-3'>\n";
    html += "        <label class='form-label' for='mode'>Mode</label>\n";
    html += "        <select class='form-select' id='mode' name='mode'>\n";
    html += "          <option value='on'>On</option>\n";
    html += "          <option value='off'>Off</option>\n";
    html += "        </select>\n";
    html += "      </div>\n";
    // Temperature setpoint control
    html += "      <div class='mb-3'>\n";
    html += "        <label class='form-label' for='setpoint'>Temperature Setpoint</label>\n";
    html += "        <input type='number' class='form-control' id='setpoint' name='setpoint' value='" + String(state->getTargetTemperature()) + "' min='10' max='30' step='0.5'>\n";
    html += "      </div>\n";
    // Action buttons for setting mode and setpoint
    html += "      <button type='button' class='btn btn-primary' onclick='setMode()'>Set Mode</button>\n";
    html += "      <button type='button' class='btn btn-primary ms-2' onclick='setSetpoint()'>Set Temperature</button>\n";
    html += "    </div>\n";
    html += "  </div>\n";
    html += "</div>\n";
    
    return html;
}

String HtmlGenerator::generateConfigSection(ConfigInterface* config, const String& csrfToken) {
    if (!config) return "";
    
    // Retrieve KNX physical address components
    uint8_t area = 1, line = 1, member = 1;
    if (ConfigManager* cm = dynamic_cast<ConfigManager*>(config)) {
        cm->getKnxPhysicalAddress(area, line, member);
    }

    String knxAddress = String(area) + "." + String(line) + "." + String(member);
    
    // Build the HTML using Bootstrap classes
    String html;
    html += "<div id='config' class='section container mt-4'>\n";
    html += "  <div class='card'>\n";
    html += "    <div class='card-header'>\n";
    html += "      <h2>Configuration</h2>\n";
    html += "    </div>\n";
    html += "    <div class='card-body'>\n";
    html += "      <form id='configForm'>\n";
    // Include CSRF token hidden input for additional security
    html += "        <input type='hidden' name='_csrf' value='" + csrfToken + "'>\n";
    
    // Device Settings Section
    html += "        <h4>Device Settings</h4>\n";
    html += "        <div class='mb-3'>\n";
    html += "          <label class='form-label' for='deviceName'>Device Name</label>\n";
    html += "          <input type='text' class='form-control' id='deviceName' name='deviceName' value='" + String(config->getDeviceName()) + "'>\n";
    html += "        </div>\n";
    html += "        <div class='mb-3'>\n";
    html += "          <label class='form-label' for='updateInterval'>Update Interval (ms)</label>\n";
    html += "          <input type='number' class='form-control' id='updateInterval' name='updateInterval' value='" + String(config->getSendInterval()) + "' min='1000' step='1000'>\n";
    html += "        </div>\n";
    
    // KNX Settings Section
    html += "        <h4>KNX Settings</h4>\n";
    html += "        <div class='mb-3 form-check'>\n";
    html += "          <input type='checkbox' class='form-check-input' id='knxEnabled' name='knxEnabled' " + String(config->getKnxEnabled() ? "checked" : "") + ">\n";
    html += "          <label class='form-check-label' for='knxEnabled'>Enable KNX</label>\n";
    html += "        </div>\n";
    html += "        <div class='mb-3'>\n";
    html += "          <label class='form-label' for='knxAddress'>KNX Address</label>\n";
    html += "          <input type='text' class='form-control' id='knxAddress' name='knxAddress' pattern='\\d{1,2}\\.\\d{1,2}\\.\\d{1,3}' value='" + knxAddress + "'>\n";
    html += "        </div>\n";
    
    // MQTT Settings Section
    html += "        <h4>MQTT Settings</h4>\n";
    html += "        <div class='mb-3 form-check'>\n";
    html += "          <input type='checkbox' class='form-check-input' id='mqttEnabled' name='mqttEnabled' " + String(config->getMqttEnabled() ? "checked" : "") + ">\n";
    html += "          <label class='form-check-label' for='mqttEnabled'>Enable MQTT</label>\n";
    html += "        </div>\n";
    
    // Conditionally include MQTT-specific fields if available
    if (ConfigManager* configManager = dynamic_cast<ConfigManager*>(config)) {
        html += "        <div class='mb-3'>\n";
        html += "          <label class='form-label' for='mqttServer'>MQTT Server</label>\n";
        html += "          <input type='text' class='form-control' id='mqttServer' name='mqttServer' value='" + String(configManager->getMQTTServer()) + "'>\n";
        html += "        </div>\n";
        html += "        <div class='mb-3'>\n";
        html += "          <label class='form-label' for='mqttPort'>MQTT Port</label>\n";
        html += "          <input type='number' class='form-control' id='mqttPort' name='mqttPort' value='" + String(configManager->getMQTTPort()) + "'>\n";
        html += "        </div>\n";
        html += "        <div class='mb-3'>\n";
        html += "          <label class='form-label' for='mqttUser'>MQTT Username</label>\n";
        html += "          <input type='text' class='form-control' id='mqttUser' name='mqttUser' value='" + String(configManager->getMQTTUser()) + "'>\n";
        html += "        </div>\n";
        html += "        <div class='mb-3'>\n";
        html += "          <label class='form-label' for='mqttPassword'>MQTT Password</label>\n";
        html += "          <input type='password' class='form-control' id='mqttPassword' name='mqttPassword'>\n";
        html += "        </div>\n";
        html += "        <div class='mb-3'>\n";
        html += "          <label class='form-label' for='mqttClientId'>MQTT Client ID</label>\n";
        html += "          <input type='text' class='form-control' id='mqttClientId' name='mqttClientId' value='" + String(configManager->getMQTTClientId()) + "'>\n";
        html += "        </div>\n";
        html += "        <div class='mb-3'>\n";
        html += "          <label class='form-label' for='mqttTopicPrefix'>MQTT Topic Prefix</label>\n";
        html += "          <input type='text' class='form-control' id='mqttTopicPrefix' name='mqttTopicPrefix' value='" + String(configManager->getMQTTTopicPrefix()) + "'>\n";
        html += "        </div>\n";
    }
    
    // Action buttons
    html += "        <button type='button' class='btn btn-primary' onclick='saveConfig()'>Save Configuration</button>\n";
    html += "        <button type='button' class='btn btn-danger ms-2' onclick='factoryReset()'>Factory Reset</button>\n";
    html += "        <button type='button' class='btn btn-info ms-2' onclick='showConfig()'>Show Config</button>\n";
    html += "      </form>\n";
    html += "      <pre id='configContents' class='mt-3'></pre>\n";
    html += "    </div>\n";
    html += "  </div>\n";
    html += "</div>\n";
    
    return html;
}

String HtmlGenerator::generatePIDSection(ControlInterface* control, const String& csrfToken) {
    if (!control) return "";
    
    char buf[2024];
    snprintf(buf, sizeof(buf), R"PID(
<div id="pid" class="section">
    <h2>PID Control</h2>
    <div class="card">
        <div class="card-body">
            <input type="hidden" name="_csrf" value="%s">
            <div class="row">
                <div class="col-md-3">
                    <label class="form-label">Kp</label>
                    <input type="number" class="form-control" id="kp" value="%.2f" step="0.1">
                </div>
                <div class="col-md-3">
                    <label class="form-label">Ki</label>
                    <input type="number" class="form-control" id="ki" value="%.2f" step="0.01">
                </div>
                <div class="col-md-3">
                    <label class="form-label">Kd</label>
                    <input type="number" class="form-control" id="kd" value="%.2f" step="0.01">
                </div>
                <div class="col-md-3">
                    <label class="form-label">Output</label>
                    <input type="number" class="form-control" id="pidOutput" value="%.1f" readonly>
                </div>
            </div>
            <div class="form-check mt-3">
                <input class="form-check-input" type="checkbox" id="pidActive" %s>
                <label class="form-check-label" for="pidActive">PID Active</label>
            </div>
            <button class="btn btn-primary mt-3" onclick="updatePID()">Update</button>
        </div>
    </div>
</div>
)PID",
        csrfToken.c_str(),
        control->getKp(),
        control->getKi(),
        control->getKd(),
        control->getOutput(),
        control->isActive() ? "checked" : ""
    );
    return String(buf);
}


String HtmlGenerator::generateFooter() {
    return R"(
<footer>
    <p>&copy; 2025 Coolbox</p>
</footer>
<script src="/scripts.js"></script>
</body>
</html>
)";
}