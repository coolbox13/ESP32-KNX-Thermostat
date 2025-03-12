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
        request->send(403, "text/plain", "Invalid CSRF token");
        return;
    }

    // Process JSON data
    if (request->hasParam("plain", true)) {
        String jsonData = request->getParam("plain", true)->value();
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, jsonData);
        
        if (error) {
            ESP_LOGW(TAG, "Invalid JSON from IP: %s", request->client()->remoteIP().toString().c_str());
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        
        // Process device settings
        if (doc.containsKey("device")) {
            JsonObject device = doc["device"];
            if (device.containsKey("name")) {
                configManager->setDeviceName(device["name"]);
                ESP_LOGI(TAG, "Device name updated to: %s", device["name"].as<const char*>());
            }
            
            if (device.containsKey("sendInterval")) {
                configManager->setSendInterval(device["sendInterval"]);
                ESP_LOGI(TAG, "Update interval set to: %lu", device["sendInterval"].as<unsigned long>());
            }
        }
        
        // Process web credentials
        if (doc.containsKey("web")) {
            JsonObject web = doc["web"];
            if (web.containsKey("username")) {
                configManager->setWebUsername(web["username"]);
            }
            
            if (web.containsKey("password")) {
                configManager->setWebPassword(web["password"]);
            }
        }
        
        // Process KNX settings
        if (doc.containsKey("knx")) {
            JsonObject knx = doc["knx"];
            
            if (knx.containsKey("enabled")) {
                configManager->setKnxEnabled(knx["enabled"]);
                ESP_LOGI(TAG, "KNX %s", knx["enabled"].as<bool>() ? "enabled" : "disabled");
            }
            
            if (knx.containsKey("physical")) {
                JsonObject physical = knx["physical"];
                if (physical.containsKey("area") && physical.containsKey("line") && physical.containsKey("member")) {
                    uint8_t area = physical["area"].as<uint8_t>();
                    uint8_t line = physical["line"].as<uint8_t>();
                    uint8_t member = physical["member"].as<uint8_t>();
                    
                    configManager->setKnxPhysicalAddress(area, line, member);
                    ESP_LOGI(TAG, "KNX address set to: %d.%d.%d", area, line, member);
                }
            }
        }
        
        // Process MQTT settings
        if (doc.containsKey("mqtt")) {
            JsonObject mqtt = doc["mqtt"];
            
            if (mqtt.containsKey("enabled")) {
                configManager->setMQTTEnabled(mqtt["enabled"]);
                ESP_LOGI(TAG, "MQTT %s", mqtt["enabled"].as<bool>() ? "enabled" : "disabled");
            }
            
            if (mqtt.containsKey("server")) {
                configManager->setMQTTServer(mqtt["server"]);
                ESP_LOGI(TAG, "MQTT server set to: %s", mqtt["server"].as<const char*>());
            }
            
            if (mqtt.containsKey("port")) {
                configManager->setMQTTPort(mqtt["port"]);
                ESP_LOGI(TAG, "MQTT port set to: %d", mqtt["port"].as<uint16_t>());
            }
            
            if (mqtt.containsKey("username")) {
                configManager->setMQTTUser(mqtt["username"]);
            }
            
            if (mqtt.containsKey("password")) {
                configManager->setMQTTPassword(mqtt["password"]);
            }
            
            if (mqtt.containsKey("clientId")) {
                configManager->setMQTTClientId(mqtt["clientId"]);
            }
            
            if (mqtt.containsKey("topicPrefix")) {
                configManager->setMQTTTopicPrefix(mqtt["topicPrefix"]);
            }
        }
        
        // Process PID settings
        if (doc.containsKey("pid")) {
            JsonObject pid = doc["pid"];
            if (pid.containsKey("kp")) configManager->setKp(pid["kp"]);
            if (pid.containsKey("ki")) configManager->setKi(pid["ki"]);
            if (pid.containsKey("kd")) configManager->setKd(pid["kd"]);
            
            // Update PID controller with new values
            if (pidController) {
                PIDConfig config = {
                    .kp = configManager->getKp(),
                    .ki = configManager->getKi(),
                    .kd = configManager->getKd(),
                    .minOutput = pidController->getMinOutput(),
                    .maxOutput = pidController->getMaxOutput(),
                    .sampleTime = pidController->getSampleTime()
                };
                pidController->configure(&config);
            }
        }
        
        // Save configuration to flash
        if (!configManager->saveConfig()) {
            ESP_LOGE(TAG, "Failed to save configuration");
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to save configuration\"}");
            return;
        }
        
        ESP_LOGI(TAG, "Configuration saved successfully");
    }
    
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

void WebInterface::handleSetpoint(AsyncWebServerRequest* request) {
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
    
    // Update both thermostat state and config manager
    thermostatState->setTargetTemperature(setpoint);
    configManager->setSetpoint(setpoint);
    
    // Save configuration to flash
    configManager->saveConfig();
    
    ESP_LOGI(TAG, "Setpoint updated to: %.1f°C", setpoint);
    request->send(200, "text/plain", "Setpoint updated");
}

// In web_interface_handlers.cpp, around line 250 (after handleMode)
void WebInterface::handlePID(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        requestAuthentication(request);
        return;
    }

    if (!validateCSRFToken(request)) {
        ESP_LOGW(TAG, "Invalid CSRF token from IP: %s", request->client()->remoteIP().toString().c_str());
        request->send(403, "application/json", "{\"error\":\"Invalid CSRF token\"}");
        return;
    }

    // For AsyncWebServer, JSON comes in the "plain" parameter
    if (!request->hasParam("plain", true)) {
        ESP_LOGW(TAG, "Missing JSON data from IP: %s", request->client()->remoteIP().toString().c_str());
        request->send(400, "application/json", "{\"error\":\"Missing JSON data\"}");
        return;
    }

    String jsonData = request->getParam("plain", true)->value();
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        ESP_LOGW(TAG, "Invalid JSON from IP: %s - %s", 
                 request->client()->remoteIP().toString().c_str(), error.c_str());
        request->send(400, "application/json", "{\"error\":\"Invalid JSON: " + String(error.c_str()) + "\"}");
        return;
    }
    
    bool updated = false;
    
    // Get current PID configuration
    PIDConfig config = {
        .kp = pidController->getKp(),
        .ki = pidController->getKi(),
        .kd = pidController->getKd(),
        .minOutput = pidController->getMinOutput(),
        .maxOutput = pidController->getMaxOutput(),
        .sampleTime = pidController->getSampleTime()
    };
    
    // Update individual parameters
    if (doc.containsKey("kp")) {
        config.kp = doc["kp"].as<float>();
        configManager->setKp(config.kp);  // Update ConfigManager
        updated = true;
        ESP_LOGI(TAG, "PID Kp updated to: %.2f", config.kp);
    }
    
    if (doc.containsKey("ki")) {
        config.ki = doc["ki"].as<float>();
        configManager->setKi(config.ki);  // Update ConfigManager
        updated = true;
        ESP_LOGI(TAG, "PID Ki updated to: %.2f", config.ki);
    }
    
    if (doc.containsKey("kd")) {
        config.kd = doc["kd"].as<float>();
        configManager->setKd(config.kd);  // Update ConfigManager
        updated = true;
        ESP_LOGI(TAG, "PID Kd updated to: %.2f", config.kd);
    }
    
    // Apply the configuration if parameters were updated
    if (updated) {
        pidController->configure(&config);
    }
    
    // Set active state separately
    if (doc.containsKey("active")) {
        bool active = doc["active"].as<bool>();
        pidController->setActive(active);
        updated = true;
        ESP_LOGI(TAG, "PID active state set to: %s", active ? "true" : "false");
    }
    
    if (!updated) {
        ESP_LOGW(TAG, "No valid PID parameters found in request from IP: %s", 
                 request->client()->remoteIP().toString().c_str());
        request->send(400, "application/json", "{\"error\":\"No valid PID parameters provided\"}");
        return;
    }
    
    // Save configuration to flash
    configManager->saveConfig();
    
    // Return success response
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"PID parameters updated successfully\"}");
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

    // Save configuration to ensure mode persists
    configManager->saveConfig();
    
    ESP_LOGI(TAG, "Mode updated to: %s", mode.c_str());
    request->send(200, "text/plain", "Mode updated");
}

void WebInterface::handleNotFound(AsyncWebServerRequest *request) {
    if (!handleFileRead(request, request->url())) {
        ESP_LOGW(TAG, "File not found: %s from IP: %s", request->url().c_str(), request->client()->remoteIP().toString().c_str());
        request->send(404, "text/plain", "File Not Found");
    }
}