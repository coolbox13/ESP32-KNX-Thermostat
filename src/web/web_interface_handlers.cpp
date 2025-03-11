#include "web/web_interface.h"
#include "config_manager.h"
#include "sensor_interface.h"
#include "thermostat_state.h"
#include "pid_controller.h"
#include "protocol_manager.h"
#include "communication/knx/knx_interface.h"
#include "communication/mqtt/mqtt_interface.h"
#include <ArduinoJson.h>
#include "esp_log.h"
#include <LittleFS.h>
#include <WiFi.h>

static const char* TAG = "WebInterface";

void WebInterface::handleRoot(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {
        requestAuthentication(request);
        return;
    }

    ESP_LOGI(TAG, "Generating HTML for root page");
    String html = generateHtml();
    if (html.startsWith("Error:")) {
        ESP_LOGE(TAG, "Failed to generate HTML: %s", html.c_str());
    } else {
        ESP_LOGI(TAG, "HTML generated successfully, length: %d bytes", html.length());
    }

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);
    addSecurityHeaders(response);
    request->send(response);
    ESP_LOGD(TAG, "Serving root page to IP: %s", request->client()->remoteIP().toString().c_str());
}

void WebInterface::handleSave(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {
        requestAuthentication(request);
        return;
    }

    if (!validateCSRFToken(request)) {
        ESP_LOGW(TAG, "Invalid CSRF token from IP: %s", request->client()->remoteIP().toString().c_str());
        request->send(403, "text/plain", "Invalid CSRF token");
        return;
    }

    if (!configManager) {
        ESP_LOGE(TAG, "Configuration manager not available");
        request->send(500, "text/plain", "Configuration manager not available");
        return;
    }

    // Handle device name
    if (request->hasParam("deviceName", true)) {
        configManager->setDeviceName(request->getParam("deviceName", true)->value().c_str());
        ESP_LOGI(TAG, "Device name updated to: %s", request->getParam("deviceName", true)->value().c_str());
    }

    // Handle send interval
    if (request->hasParam("sendInterval", true)) {
        uint32_t interval = request->getParam("sendInterval", true)->value().toInt();
        if (interval > 0) {
            sensorInterface->setUpdateInterval(interval);
            ESP_LOGI(TAG, "Send interval updated to: %u ms", interval);
        }
    }

    // Handle PID interval
    if (request->hasParam("pidInterval", true)) {
        uint32_t interval = request->getParam("pidInterval", true)->value().toInt();
        if (interval > 0) {
            pidController->setUpdateInterval(interval);
            ESP_LOGI(TAG, "PID interval updated to: %u ms", interval);
        }
    }

    // Handle KNX settings
    if (request->hasParam("knxArea", true) && request->hasParam("knxLine", true) && request->hasParam("knxMember", true)) {
        uint8_t area = request->getParam("knxArea", true)->value().toInt();
        uint8_t line = request->getParam("knxLine", true)->value().toInt();
        uint8_t member = request->getParam("knxMember", true)->value().toInt();
        configManager->setKnxPhysicalAddress(area, line, member);
        ESP_LOGI(TAG, "KNX physical address updated to: %u.%u.%u", area, line, member);
    }

    configManager->setKnxEnabled(request->hasParam("knxEnabled", true));
    ESP_LOGI(TAG, "KNX %s", request->hasParam("knxEnabled", true) ? "enabled" : "disabled");

    // Handle KNX temperature GA
    if (request->hasParam("knxTempArea", true) && request->hasParam("knxTempLine", true) && request->hasParam("knxTempMember", true)) {
        uint8_t area = request->getParam("knxTempArea", true)->value().toInt();
        uint8_t line = request->getParam("knxTempLine", true)->value().toInt();
        uint8_t member = request->getParam("knxTempMember", true)->value().toInt();
        configManager->setKnxTemperatureGA(area, line, member);
    }

    // Handle KNX setpoint GA
    if (request->hasParam("knxSetpointArea", true) && request->hasParam("knxSetpointLine", true) && request->hasParam("knxSetpointMember", true)) {
        uint8_t area = request->getParam("knxSetpointArea", true)->value().toInt();
        uint8_t line = request->getParam("knxSetpointLine", true)->value().toInt();
        uint8_t member = request->getParam("knxSetpointMember", true)->value().toInt();
        configManager->setKnxSetpointGA(area, line, member);
    }

    // Handle KNX valve GA
    if (request->hasParam("knxValveArea", true) && request->hasParam("knxValveLine", true) && request->hasParam("knxValveMember", true)) {
        uint8_t area = request->getParam("knxValveArea", true)->value().toInt();
        uint8_t line = request->getParam("knxValveLine", true)->value().toInt();
        uint8_t member = request->getParam("knxValveMember", true)->value().toInt();
        configManager->setKnxValveGA(area, line, member);
    }

    // Handle KNX mode GA
    if (request->hasParam("knxModeArea", true) && request->hasParam("knxModeLine", true) && request->hasParam("knxModeMember", true)) {
        uint8_t area = request->getParam("knxModeArea", true)->value().toInt();
        uint8_t line = request->getParam("knxModeLine", true)->value().toInt();
        uint8_t member = request->getParam("knxModeMember", true)->value().toInt();
        configManager->setKnxModeGA(area, line, member);
    }

    // Handle MQTT settings
    configManager->setMQTTEnabled(request->hasParam("mqttEnabled", true));

    if (request->hasParam("mqttServer", true)) {
        configManager->setMQTTServer(request->getParam("mqttServer", true)->value().c_str());
    }

    if (request->hasParam("mqttPort", true)) {
        uint16_t port = request->getParam("mqttPort", true)->value().toInt();
        configManager->setMQTTPort(port);
    }

    if (request->hasParam("mqttUser", true)) {
        configManager->setMQTTUser(request->getParam("mqttUser", true)->value().c_str());
    }

    if (request->hasParam("mqttPassword", true)) {
        configManager->setMQTTPassword(request->getParam("mqttPassword", true)->value().c_str());
    }

    if (request->hasParam("mqttClientId", true)) {
        configManager->setMQTTClientId(request->getParam("mqttClientId", true)->value().c_str());
    }

    // Save configuration
    configManager->saveConfig();
    ESP_LOGI(TAG, "Configuration saved successfully");

    request->send(200, "text/plain", "Settings saved");
}

void WebInterface::handleGetStatus(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {
        requestAuthentication(request);
        return;
    }

    if (!thermostatState) {
        ESP_LOGE(TAG, "Thermostat state not available");
        request->send(500, "text/plain", "Internal server error");
        return;
    }

    StaticJsonDocument<512> doc;
    doc["temperature"] = thermostatState->getCurrentTemperature();
    doc["humidity"] = thermostatState->getCurrentHumidity();
    doc["pressure"] = thermostatState->getCurrentPressure();
    doc["setpoint"] = thermostatState->getTargetTemperature();
    doc["enabled"] = thermostatState->isEnabled();
    doc["error"] = thermostatState->getStatus();

    String response;
    serializeJson(doc, response);

    AsyncWebServerResponse *jsonResponse = request->beginResponse(200, "application/json", response);
    addSecurityHeaders(jsonResponse);
    request->send(jsonResponse);
    ESP_LOGD(TAG, "Status sent to IP: %s", request->client()->remoteIP().toString().c_str());
}

void WebInterface::handleSetpoint(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {
        requestAuthentication(request);
        return;
    }

    if (!validateCSRFToken(request)) {
        ESP_LOGW(TAG, "Invalid CSRF token from IP: %s", request->client()->remoteIP().toString().c_str());
        request->send(403, "text/plain", "Invalid CSRF token");
        return;
    }

    if (!request->hasParam("setpoint", true)) {
        ESP_LOGW(TAG, "Missing setpoint parameter from IP: %s", request->client()->remoteIP().toString().c_str());
        request->send(400, "text/plain", "Missing setpoint parameter");
        return;
    }

    float setpoint = request->getParam("setpoint", true)->value().toFloat();
    thermostatState->setTargetTemperature(setpoint);
    configManager->setSetpoint(setpoint);
    configManager->saveConfig();
    ESP_LOGI(TAG, "Setpoint updated to: %.1f°C", setpoint);

    request->send(200, "text/plain", "Setpoint updated");
}

void WebInterface::handleSaveConfig(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {
        requestAuthentication(request);
        return;
    }

    if (!validateCSRFToken(request)) {
        ESP_LOGW(TAG, "Invalid CSRF token from IP: %s", request->client()->remoteIP().toString().c_str());
        request->send(403, "text/plain", "Invalid CSRF token");
        return;
    }

    if (!request->hasParam("plain", true)) {
        ESP_LOGW(TAG, "Missing configuration data from IP: %s", request->client()->remoteIP().toString().c_str());
        request->send(400, "text/plain", "Missing configuration data");
        return;
    }

    String json = request->getParam("plain", true)->value();
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        ESP_LOGW(TAG, "Invalid JSON from IP: %s", request->client()->remoteIP().toString().c_str());
        request->send(400, "text/plain", "Invalid JSON");
        return;
    }

    // Apply configuration
    if (doc.containsKey("deviceName")) {
        configManager->setDeviceName(doc["deviceName"]);
        ESP_LOGI(TAG, "Device name updated to: %s", doc["deviceName"].as<const char*>());
    }

    // Save configuration
    configManager->saveConfig();
    ESP_LOGI(TAG, "Configuration saved successfully");

    request->send(200, "text/plain", "Configuration saved");
}

void WebInterface::handleReboot(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {
        requestAuthentication(request);
        return;
    }

    ESP_LOGI(TAG, "Reboot requested from IP: %s", request->client()->remoteIP().toString().c_str());
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Device will reboot in 5 seconds...");
    addSecurityHeaders(response);
    request->send(response);

    delay(5000);
    ESP.restart();
}

void WebInterface::handleFactoryReset(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {
        requestAuthentication(request);
        return;
    }

    if (!validateCSRFToken(request)) {
        ESP_LOGW(TAG, "Invalid CSRF token from IP: %s", request->client()->remoteIP().toString().c_str());
        request->send(403, "text/plain", "Invalid CSRF token");
        return;
    }

    ESP_LOGI(TAG, "Factory reset requested from IP: %s", request->client()->remoteIP().toString().c_str());
    // Perform factory reset
    configManager->resetToDefaults();
    
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Factory reset complete. Device will reboot in 5 seconds...");
    addSecurityHeaders(response);
    request->send(response);

    delay(5000);
    ESP.restart();
}

void WebInterface::handleMode(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        requestAuthentication(request);
        return;
    }

    if (!validateCSRFToken(request)) {
        ESP_LOGW(TAG, "Invalid CSRF token from IP: %s", request->client()->remoteIP().toString().c_str());
        request->send(403, "text/plain", "Invalid CSRF token");
        return;
    }

    if (!request->hasParam("mode", true)) {
        ESP_LOGW(TAG, "Missing mode parameter from IP: %s", request->client()->remoteIP().toString().c_str());
        request->send(400, "text/plain", "Missing mode parameter");
        return;
    }

    String mode = request->getParam("mode", true)->value();
    if (mode == "on") {
        thermostatState->setEnabled(true);
    } else if (mode == "off") {
        thermostatState->setEnabled(false);
    } else {
        ESP_LOGW(TAG, "Invalid mode value: %s from IP: %s", mode.c_str(), request->client()->remoteIP().toString().c_str());
        request->send(400, "text/plain", "Invalid mode value");
        return;
    }

    ESP_LOGI(TAG, "Mode updated to: %s", mode.c_str());
    request->send(200, "text/plain", "Mode updated");
}

void WebInterface::handleNotFound(AsyncWebServerRequest *request) {
    if (!handleFileRead(request, request->url())) {
        ESP_LOGW(TAG, "File not found: %s from IP: %s", request->url().c_str(), request->client()->remoteIP().toString().c_str());
        request->send(404, "text/plain", "File Not Found");
    }
}