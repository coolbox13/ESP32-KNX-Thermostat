#include "web_interface.h"
#include "web/html_generator.h"  // <-- Added include for HtmlGenerator
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
    
    // Generate CSRF token for the session
    String csrfToken = generateCSRFToken(request);
    
    // Generate the full HTML page dynamically using HtmlGenerator
    String html = HtmlGenerator::generatePage(thermostatState, configManager, pidController, csrfToken);
    
    // Create and send the response with security headers
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);
    addSecurityHeaders(response);
    request->send(response);
}


void WebInterface::handleSave(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {
        requestAuthentication(request);
        return;
    }

    if (!validateCSRFToken(request)) {
        ESP_LOGW(TAG, "Invalid CSRF token from IP: %s", request->client()->remoteIP().toString().c_str());
        request->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Invalid CSRF token\"}");
        return;
    }

    // Check if we're receiving form data or JSON
    bool isFormData = request->contentType().equals("application/x-www-form-urlencoded");
    bool isJsonData = request->contentType().equals("application/json");
    
    if (!isFormData && !isJsonData) {
        ESP_LOGW(TAG, "Unsupported content type: %s", request->contentType().c_str());
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Unsupported content type\"}");
        return;
    }
    
    // Process device settings
    if (request->hasParam("deviceName", true)) {
        configManager->setDeviceName(request->getParam("deviceName", true)->value().c_str());
        ESP_LOGI(TAG, "Device name updated to: %s", request->getParam("deviceName", true)->value().c_str());
    } else if (isJsonData && request->hasParam("plain", true)) {
        // Process JSON data
        String jsonData = request->getParam("plain", true)->value();
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, jsonData);
        
        if (error) {
            ESP_LOGW(TAG, "Invalid JSON from IP: %s", request->client()->remoteIP().toString().c_str());
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        
        // Process device name from JSON
        if (doc.containsKey("deviceName")) {
            configManager->setDeviceName(doc["deviceName"]);
            ESP_LOGI(TAG, "Device name updated to: %s", doc["deviceName"].as<const char*>());
        }
        
        // Process update interval
        if (doc.containsKey("updateInterval")) {
            configManager->setSendInterval(doc["updateInterval"]);
            ESP_LOGI(TAG, "Update interval set to: %lu", doc["updateInterval"].as<unsigned long>());
        }
        
        // Process KNX address
        if (doc.containsKey("knxAddress")) {
            String addr = doc["knxAddress"];
            // Parse KNX address (format: area.line.member)
            int firstDot = addr.indexOf('.');
            int secondDot = addr.indexOf('.', firstDot + 1);
            
            if (firstDot > 0 && secondDot > firstDot) {
                uint8_t area = addr.substring(0, firstDot).toInt();
                uint8_t line = addr.substring(firstDot + 1, secondDot).toInt();
                uint8_t member = addr.substring(secondDot + 1).toInt();
                
                configManager->setKnxPhysicalAddress(area, line, member);
                ESP_LOGI(TAG, "KNX address set to: %d.%d.%d", area, line, member);
            }
        }
        
        // Process KNX enabled flag
        if (doc.containsKey("knxEnabled")) {
            configManager->setKnxEnabled(doc["knxEnabled"]);
            ESP_LOGI(TAG, "KNX %s", doc["knxEnabled"].as<bool>() ? "enabled" : "disabled");
        }
        
        // Process MQTT settings
        if (doc.containsKey("mqttEnabled")) {
            configManager->setMQTTEnabled(doc["mqttEnabled"]);
            ESP_LOGI(TAG, "MQTT %s", doc["mqttEnabled"].as<bool>() ? "enabled" : "disabled");
        }
        
        if (doc.containsKey("mqttServer")) {
            configManager->setMQTTServer(doc["mqttServer"]);
            ESP_LOGI(TAG, "MQTT server set to: %s", doc["mqttServer"].as<const char*>());
        }
        
        if (doc.containsKey("mqttPort")) {
            configManager->setMQTTPort(doc["mqttPort"]);
            ESP_LOGI(TAG, "MQTT port set to: %d", doc["mqttPort"].as<uint16_t>());
        }
        
        if (doc.containsKey("mqttUser")) {
            configManager->setMQTTUser(doc["mqttUser"]);
        }
        
        if (doc.containsKey("mqttPassword")) {
            configManager->setMQTTPassword(doc["mqttPassword"]);
        }
        
        if (doc.containsKey("mqttClientId")) {
            configManager->setMQTTClientId(doc["mqttClientId"]);
        }
        
        if (doc.containsKey("mqttTopicPrefix")) {
            configManager->setMQTTTopicPrefix(doc["mqttTopicPrefix"]);
        }
    }
    
    // Process form data
    if (isFormData) {
        if (request->hasParam("updateInterval", true)) {
            uint32_t interval = request->getParam("updateInterval", true)->value().toInt();
            if (interval > 0) {
                configManager->setSendInterval(interval);
                ESP_LOGI(TAG, "Update interval set to: %u ms", interval);
            }
        }
        
        if (request->hasParam("knxAddress", true)) {
            String addr = request->getParam("knxAddress", true)->value();
            // Parse KNX address (format: area.line.member)
            int firstDot = addr.indexOf('.');
            int secondDot = addr.indexOf('.', firstDot + 1);
            
            if (firstDot > 0 && secondDot > firstDot) {
                uint8_t area = addr.substring(0, firstDot).toInt();
                uint8_t line = addr.substring(firstDot + 1, secondDot).toInt();
                uint8_t member = addr.substring(secondDot + 1).toInt();
                
                configManager->setKnxPhysicalAddress(area, line, member);
                ESP_LOGI(TAG, "KNX address set to: %d.%d.%d", area, line, member);
            }
        }
    }
    
    // Save configuration to file
    configManager->saveConfig();
    ESP_LOGI(TAG, "Configuration saved successfully");
    
    // Return a JSON response
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Configuration saved\"}");
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

void WebInterface::handlePID(AsyncWebServerRequest* request) {
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
        ESP_LOGW(TAG, "Missing JSON data from IP: %s", request->client()->remoteIP().toString().c_str());
        request->send(400, "text/plain", "Missing JSON data");
        return;
    }

    String jsonData = request->getParam("plain", true)->value();
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        ESP_LOGW(TAG, "Invalid JSON from IP: %s - %s", 
                 request->client()->remoteIP().toString().c_str(), error.c_str());
        request->send(400, "text/plain", "Invalid JSON data");
        return;
    }
    
    bool updated = false;
    
    if (doc.containsKey("kp") && pidController) {
        float kp = doc["kp"].as<float>();
        pidController->setKp(kp);
        updated = true;
        ESP_LOGI(TAG, "PID Kp updated to: %.2f", kp);
    }
    
    if (doc.containsKey("ki") && pidController) {
        float ki = doc["ki"].as<float>();
        pidController->setKi(ki);
        updated = true;
        ESP_LOGI(TAG, "PID Ki updated to: %.2f", ki);
    }
    
    if (doc.containsKey("kd") && pidController) {
        float kd = doc["kd"].as<float>();
        pidController->setKd(kd);
        updated = true;
        ESP_LOGI(TAG, "PID Kd updated to: %.2f", kd);
    }
    
    if (doc.containsKey("active") && pidController) {
        bool active = doc["active"].as<bool>();
        pidController->setActive(active);
        updated = true;
        ESP_LOGI(TAG, "PID active state set to: %s", active ? "true" : "false");
    }
    
    if (!updated) {
        ESP_LOGW(TAG, "No valid PID parameters found in request from IP: %s", 
                 request->client()->remoteIP().toString().c_str());
        request->send(400, "text/plain", "No valid PID parameters provided");
        return;
    }
    
    request->send(200, "application/json", "{\"status\":\"ok\"}");
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