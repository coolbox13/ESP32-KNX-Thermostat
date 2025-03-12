// Basic includes
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <WiFi.h>

// Include the ThermostatState first since other components depend on it
#include "thermostat_state.h"

// Then include your component headers
#include "communication/knx/knx_interface.h"
#include "config_manager.h"
#include <ESPAsyncWiFiManager.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_log.h>
#include "pid_controller.h"
#include "protocol_types.h"
#include "communication/mqtt/mqtt_interface.h"

static const char* TAG = "ConfigManager";

ConfigManager::ConfigManager() {
    // Initialize default values
    strlcpy(deviceName, "ESP32 Thermostat", sizeof(deviceName));
    
    // Web interface defaults
    strlcpy(webUsername, "admin", sizeof(webUsername));
    strlcpy(webPassword, "admin", sizeof(webPassword));
    
    // KNX defaults
    knxEnabled = false;
    knxPhysicalAddress = {1, 1, 160};
    
    // MQTT defaults
    mqttEnabled = true;
    strlcpy(mqttServer, "192.168.178.32", sizeof(mqttServer));
    mqttPort = 1883;
    strlcpy(mqttUser, "", sizeof(mqttUser));
    strlcpy(mqttPassword, "", sizeof(mqttPassword));
    strlcpy(mqttClientId, "esp32_thermostat", sizeof(mqttClientId));
    strlcpy(mqttTopicPrefix, "esp32/thermostat/", sizeof(mqttTopicPrefix));
    
    // Default PID configuration
    pidConfig = {
        .kp = 2.0f,
        .ki = 0.5f,
        .kd = 1.0f,
        .minOutput = 0.0f,
        .maxOutput = 100.0f,
        .sampleTime = 30000.0f
    };
}

bool ConfigManager::begin() {
    if (!LittleFS.begin(true)) {
        ESP_LOGE(TAG, "Failed to mount file system");
        return false;
    }
    return loadConfig();
}

bool ConfigManager::loadConfig() {
    // Note: LittleFS is now initialized in main.cpp
    
    // Try to open the config file
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        ESP_LOGE(TAG, "Failed to open config file");
        lastError = ThermostatStatus::ERROR_CONFIGURATION;
        return false;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        ESP_LOGE(TAG, "Failed to parse config file: %s", error.c_str());
        lastError = ThermostatStatus::ERROR_CONFIGURATION;
        return false;
    }

    // Load device settings
    if (doc.containsKey("deviceName")) {
        strlcpy(deviceName, doc["deviceName"] | "ESP32 Thermostat", sizeof(deviceName));
    }

    // Load web interface settings
    JsonObject web = doc["web"];
    if (web) {
        strlcpy(webUsername, web["username"] | "admin", sizeof(webUsername));
        strlcpy(webPassword, web["password"] | "admin", sizeof(webPassword));
    }

    // Load KNX settings
    JsonObject knx = doc["knx"];
    if (knx) {
        knxEnabled = knx["enabled"] | true;
        JsonObject physical = knx["physical"];
        if (physical) {
            knxPhysicalAddress.area = physical["area"] | 1;
            knxPhysicalAddress.line = physical["line"] | 1;
            knxPhysicalAddress.member = physical["member"] | 1;
        }
    }

    // Load MQTT settings
    JsonObject mqtt = doc["mqtt"];
    if (mqtt) {
        mqttEnabled = mqtt["enabled"] | true;
        strlcpy(mqttServer, mqtt["server"] | "localhost", sizeof(mqttServer));
        mqttPort = mqtt["port"] | 1883;
        strlcpy(mqttUser, mqtt["username"] | "", sizeof(mqttUser));
        strlcpy(mqttPassword, mqtt["password"] | "", sizeof(mqttPassword));
        strlcpy(mqttClientId, mqtt["clientId"] | "esp32_thermostat", sizeof(mqttClientId));
        strlcpy(mqttTopicPrefix, mqtt["topicPrefix"] | "esp32/thermostat/", sizeof(mqttTopicPrefix));
    }

    // Load PID settings
    if (doc.containsKey("pid")) {
        JsonObject pid = doc["pid"];
        pidConfig.kp = pid["kp"] | 2.0f;
        pidConfig.ki = pid["ki"] | 0.5f;
        pidConfig.kd = pid["kd"] | 1.0f;
        pidConfig.minOutput = pid["minOutput"] | 0.0f;
        pidConfig.maxOutput = pid["maxOutput"] | 100.0f;
        pidConfig.sampleTime = pid["sampleTime"] | 30000.0f;
    }

    return true;
}

bool ConfigManager::saveConfig() {
    ESP_LOGI(TAG, "Attempting to save configuration...");
    
    // Create JSON document
    StaticJsonDocument<2048> doc;
    ESP_LOGI(TAG, "Created JSON document");

    // Read existing config (if it exists)
    if (LittleFS.exists("/config.json")) {
        File configFile = LittleFS.open("/config.json", "r");
        if (configFile) {
            DeserializationError error = deserializeJson(doc, configFile);
            configFile.close();
            if (error) {
                ESP_LOGE(TAG, "Failed to parse existing config file: %s", error.c_str());
                return false;
            }
            ESP_LOGI(TAG, "Loaded existing config file");
        }
    }

    // Populate JSON document with a nested structure
    
    // Web settings
    JsonObject web = doc.containsKey("web") ? doc["web"].as<JsonObject>() : doc.createNestedObject("web");
    web["username"] = webUsername;
    web["password"] = webPassword;
    
    // KNX settings
    JsonObject knx = doc.containsKey("knx") ? doc["knx"].as<JsonObject>() : doc.createNestedObject("knx");
    knx["enabled"] = knxEnabled;
    
    JsonObject knxPhysical = knx.containsKey("physical") ? knx["physical"].as<JsonObject>() : knx.createNestedObject("physical");
    knxPhysical["area"] = knxPhysicalAddress.area;
    knxPhysical["line"] = knxPhysicalAddress.line;
    knxPhysical["member"] = knxPhysicalAddress.member;
    
    // MQTT settings
    JsonObject mqtt = doc.containsKey("mqtt") ? doc["mqtt"].as<JsonObject>() : doc.createNestedObject("mqtt");
    mqtt["enabled"] = mqttEnabled;
    mqtt["server"] = mqttServer;
    mqtt["port"] = mqttPort;
    mqtt["username"] = mqttUser;
    mqtt["password"] = mqttPassword;
    mqtt["clientId"] = mqttClientId;
    mqtt["topicPrefix"] = mqttTopicPrefix;
    
    // Device settings
    JsonObject device = doc.containsKey("device") ? doc["device"].as<JsonObject>() : doc.createNestedObject("device");
    device["name"] = deviceName;
    device["sendInterval"] = sendInterval;
    
    // PID settings
    JsonObject pid = doc.containsKey("pid") ? doc["pid"].as<JsonObject>() : doc.createNestedObject("pid");
    pid["kp"] = pidConfig.kp;
    pid["ki"] = pidConfig.ki;
    pid["kd"] = pidConfig.kd;
    pid["minOutput"] = pidConfig.minOutput;
    pid["maxOutput"] = pidConfig.maxOutput;
    pid["sampleTime"] = pidConfig.sampleTime;

    // Log the JSON content for debugging
    String jsonStr;
    serializeJson(doc, jsonStr);
    ESP_LOGI(TAG, "JSON content: %s", jsonStr.c_str());

    // Open config file for writing
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        ESP_LOGE(TAG, "Failed to open config file for writing");
        return false;
    }
    ESP_LOGI(TAG, "Opened config file for writing");

    // Serialize JSON to file
    if (serializeJson(doc, configFile) == 0) {
        ESP_LOGE(TAG, "Failed to write config file");
        configFile.close();
        return false;
    }
    ESP_LOGI(TAG, "Serialized JSON to file");

    // Close file
    configFile.close();
    ESP_LOGI(TAG, "Closed config file");

    // Verify file exists
    if (!LittleFS.exists("/config.json")) {
        ESP_LOGE(TAG, "Config file does not exist after saving");
        return false;
    }
    ESP_LOGI(TAG, "Config file exists after saving");

    ESP_LOGI(TAG, "Configuration saved successfully");
    return true;
}

void ConfigManager::resetToDefaults() {
    // Reset device settings
    strlcpy(deviceName, "ESP32 Thermostat", sizeof(deviceName));
    
    // Reset web interface settings
    strlcpy(webUsername, "admin", sizeof(webUsername));
    strlcpy(webPassword, "admin", sizeof(webPassword));
    
    // Reset KNX settings
    knxEnabled = false;
    knxPhysicalAddress = {1, 1, 160};
    
    // Reset MQTT settings
    mqttEnabled = false;
    strlcpy(mqttServer, "192.168.178.32", sizeof(mqttServer));
    mqttPort = 1883;
    strlcpy(mqttUser, "", sizeof(mqttUser));
    strlcpy(mqttPassword, "", sizeof(mqttPassword));
    strlcpy(mqttClientId, "esp32_thermostat", sizeof(mqttClientId));
    strlcpy(mqttTopicPrefix, "esp32/thermostat/", sizeof(mqttTopicPrefix));
    
    // Reset PID configuration
    pidConfig = {
        .kp = 2.0f,
        .ki = 0.5f,
        .kd = 1.0f,
        .minOutput = 0.0f,
        .maxOutput = 100.0f,
        .sampleTime = 30000.0f
    };
    
    saveConfig();
}

void ConfigManager::setDeviceName(const char* name) {
    strlcpy(deviceName, name, sizeof(deviceName));
}

void ConfigManager::setWebUsername(const char* username) {
    strlcpy(webUsername, username, sizeof(webUsername));
}

void ConfigManager::setWebPassword(const char* password) {
    strlcpy(webPassword, password, sizeof(webPassword));
}

void ConfigManager::setMQTTServer(const char* server) {
    strlcpy(mqttServer, server, sizeof(mqttServer));
}

void ConfigManager::setMQTTUser(const char* user) {
    strlcpy(mqttUser, user, sizeof(mqttUser));
}

void ConfigManager::setMQTTPassword(const char* password) {
    strlcpy(mqttPassword, password, sizeof(mqttPassword));
}

void ConfigManager::setMQTTClientId(const char* clientId) {
    strlcpy(mqttClientId, clientId, sizeof(mqttClientId));
}

void ConfigManager::setMQTTTopicPrefix(const char* prefix) {
    strlcpy(mqttTopicPrefix, prefix, sizeof(mqttTopicPrefix));
}

void ConfigManager::setKnxPhysicalAddress(uint8_t area, uint8_t line, uint8_t member) {
    knxPhysicalAddress.area = area;
    knxPhysicalAddress.line = line;
    knxPhysicalAddress.member = member;
}

void ConfigManager::getKnxPhysicalAddress(uint8_t& area, uint8_t& line, uint8_t& member) const {
    area = knxPhysicalAddress.area;
    line = knxPhysicalAddress.line;
    member = knxPhysicalAddress.member;
}

bool ConfigManager::getKnxEnabled() const {
    return knxEnabled;
}

void ConfigManager::setKnxEnabled(bool enabled) {
    knxEnabled = enabled;
}

const char* ConfigManager::getDeviceName() const {
    return deviceName;
}

const char* ConfigManager::getWebUsername() const {
    return webUsername;
}

const char* ConfigManager::getWebPassword() const {
    return webPassword;
}

bool ConfigManager::getMqttEnabled() const {
    return mqttEnabled;
}

void ConfigManager::setMQTTEnabled(bool enabled) {
    mqttEnabled = enabled;
}

void ConfigManager::setMQTTPort(uint16_t port) {
    mqttPort = port;
}

unsigned long ConfigManager::getSendInterval() const {
    return static_cast<unsigned long>(sendInterval);
}

void ConfigManager::setSendInterval(unsigned long interval) {
    sendInterval = static_cast<uint32_t>(interval);
}

float ConfigManager::getSetpoint() const {
    return setpoint;
}

void ConfigManager::setSetpoint(float value) {
    setpoint = value;
}

// Add a basic WiFi setup implementation
bool ConfigManager::setupWiFi() {
    // Load credentials from storage
    if (strlen(wifiSSID) > 0 && strlen(wifiPassword) > 0) {
        // Try connecting with stored credentials
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifiSSID, wifiPassword);
        
        // Wait for connection with timeout
        for (int attempts = 0; attempts < 20; attempts++) {
            if (WiFi.status() == WL_CONNECTED) {
                ESP_LOGI(TAG, "Connected to WiFi: %s", wifiSSID);
                return true;
            }
            delay(500);
        }
    }
    
    // If we reach here, either no credentials or connection failed
    ESP_LOGI(TAG, "Starting WiFi setup portal");
    
    // Create AP for configuration
    DNSServer dns;
    AsyncWebServer server(80);
    AsyncWiFiManager wifiManager(&server, &dns);
    
    // Start portal and wait for configuration
    if (wifiManager.startConfigPortal("ESP32-Thermostat")) {
        // Successfully configured
        String newSSID = WiFi.SSID();
        String newPass = WiFi.psk();
        
        // Store new credentials
        strncpy(wifiSSID, newSSID.c_str(), sizeof(wifiSSID) - 1);
        wifiSSID[sizeof(wifiSSID) - 1] = '\0';
        
        strncpy(wifiPassword, newPass.c_str(), sizeof(wifiPassword) - 1);
        wifiPassword[sizeof(wifiPassword) - 1] = '\0';
        
        // Save to persistent storage
        saveConfig();
        return true;
    }
    
    return false;
}

void ConfigManager::getKnxTemperatureGA(uint8_t& area, uint8_t& line, uint8_t& member) const {
    area = knxTemperatureGA.area;
    line = knxTemperatureGA.line;
    member = knxTemperatureGA.member;
}

void ConfigManager::setKnxSetpointGA(uint8_t area, uint8_t line, uint8_t member) {
    knxSetpointGA = {area, line, member};
    ESP_LOGI(TAG, "KNX setpoint GA set to: %d/%d/%d", area, line, member);
}

void ConfigManager::getKnxSetpointGA(uint8_t& area, uint8_t& line, uint8_t& member) const {
    area = knxSetpointGA.area;
    line = knxSetpointGA.line;
    member = knxSetpointGA.member;
}

void ConfigManager::setKnxValveGA(uint8_t area, uint8_t line, uint8_t member) {
    knxValveGA = {area, line, member};
    ESP_LOGI(TAG, "KNX valve GA set to: %d/%d/%d", area, line, member);
}

void ConfigManager::getKnxValveGA(uint8_t& area, uint8_t& line, uint8_t& member) const {
    area = knxValveGA.area;
    line = knxValveGA.line;
    member = knxValveGA.member;
}

void ConfigManager::setKnxModeGA(uint8_t area, uint8_t line, uint8_t member) {
    knxModeGA = {area, line, member};
    ESP_LOGI(TAG, "KNX mode GA set to: %d/%d/%d", area, line, member);
}

void ConfigManager::getKnxModeGA(uint8_t& area, uint8_t& line, uint8_t& member) const {
    area = knxModeGA.area;
    line = knxModeGA.line;
    member = knxModeGA.member;
}

void ConfigManager::setKnxTemperatureGA(uint8_t area, uint8_t line, uint8_t member) {
    knxTemperatureGA.area = area;
    knxTemperatureGA.line = line;
    knxTemperatureGA.member = member;
}