#define ARDUINO_USB_MODE 1
#define ARDUINO_USB_CDC_ON_BOOT 1
#define CORE_DEBUG_LEVEL 5
#include <Arduino.h>
#include <Preferences.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <esp_log.h>
#include "config_manager.h"
#include "thermostat_state.h"
#include "protocol_manager.h"
#include "mqtt_interface.h"
#include "knx_interface.h"
#include "bme280_sensor_interface.h"
#include "pid_controller.h"
#include "main.h"

static const char* TAG = "Main";

// Global objects
ConfigManager configManager;
ThermostatState thermostatState;
ProtocolManager protocolManager(&thermostatState);
BME280SensorInterface sensorInterface;
PIDController pidController(&thermostatState);
KNXInterface knxInterface(&thermostatState);
MQTTInterface mqttInterface(&thermostatState);

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    ESP_LOGI(TAG, "Starting ESP32 KNX Thermostat (Minimal Version)...");
    
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {  // Format on failure
        ESP_LOGE(TAG, "Failed to mount LittleFS - formatting");
        if (!LittleFS.format()) {
            ESP_LOGE(TAG, "Failed to format LittleFS");
            return;
        }
        if (!LittleFS.begin()) {
            ESP_LOGE(TAG, "Failed to mount LittleFS after formatting");
            return;
        }
    }
    ESP_LOGI(TAG, "LittleFS mounted successfully");
    
    // List files in LittleFS
    listFiles();
    
    // Initialize ConfigManager
    if (!configManager.begin()) {
        ESP_LOGE(TAG, "Failed to initialize configuration");
        return;
    }

    // Setup WiFi
    if (!configManager.setupWiFi()) {
        Serial.println("Failed to connect to WiFi");
        return;
    }

    // Initialize sensor - try once at startup
    if (!sensorInterface.begin()) {
        Serial.println("BME280 sensor not available - continuing with other functionality");
        ESP_LOGW(TAG, "System will operate without temperature/humidity/pressure readings");
    } else {
        ESP_LOGI(TAG, "BME280 sensor initialized successfully");
    }

    // Initialize PID controller
    if (!pidController.begin()) {
        Serial.println("Failed to initialize PID controller");
        return;
    }

    // Initialize protocol manager
    if (!protocolManager.begin()) {
        Serial.println("Failed to initialize protocol manager");
        return;
    }

    // MQTT setup
    if (configManager.getMqttEnabled()) {
        Serial.println("MQTT is enabled");
        mqttInterface.setServer(configManager.getMQTTServer(), configManager.getMQTTPort());
        if (configManager.getMQTTUser()[0] != '\0') {
            mqttInterface.setCredentials(configManager.getMQTTUser(), configManager.getMQTTPassword());
        }
        
        // Add to protocol manager
        mqttInterface.begin();
        protocolManager.addProtocol(&mqttInterface);
        Serial.println("MQTT interface added");
    }

    // KNX setup
    if (configManager.getKnxEnabled()) {
        ESP_LOGI(TAG, "Initializing KNX interface");
        // Configure KNX settings
        DynamicJsonDocument knxConfig(512);
        uint8_t area, line, member;
        configManager.getKnxPhysicalAddress(area, line, member);
        knxConfig["physical"]["area"] = area;
        knxConfig["physical"]["line"] = line;
        knxConfig["physical"]["device"] = member;
        
        // Add more KNX configuration...
        knxInterface.configure(knxConfig);
        protocolManager.addProtocol(&knxInterface);
        Serial.println("KNX interface configured and added");
    }

    ESP_LOGI(TAG, "ESP32 KNX Thermostat (Minimal Version) initialized successfully");
}

void loop() {
    // Update sensor readings if available
    if (sensorInterface.isAvailable()) {
        sensorInterface.loop();
        
        // Update thermostat state with sensor readings
        thermostatState.setCurrentTemperature(sensorInterface.getTemperature());
        thermostatState.setCurrentHumidity(sensorInterface.getHumidity());
        thermostatState.setCurrentPressure(sensorInterface.getPressure());
    } else {
        // Sensor not available - use default values
        // This allows the system to continue functioning with default values
        thermostatState.setCurrentTemperature(21.0); // Default room temperature
        thermostatState.setCurrentHumidity(50.0);    // Default humidity
        thermostatState.setCurrentPressure(1013.25); // Default pressure (sea level)
    }

    // Update KNX interface if enabled
    if (configManager.getKnxEnabled()) {
        knxInterface.loop();
    }

    // Update PID controller
    pidController.update(sensorInterface.getTemperature());

    // Update thermostat state
    thermostatState.setCurrentTemperature(sensorInterface.getTemperature());
    thermostatState.setCurrentHumidity(sensorInterface.getHumidity());
    thermostatState.setCurrentPressure(sensorInterface.getPressure());
    thermostatState.setValvePosition(pidController.getOutput());

    // Handle protocol updates
    protocolManager.update();

    // Small delay to prevent tight looping
    delay(10);
}

// Helper function to list files in LittleFS
void listFiles() {
    File root = LittleFS.open("/");
    if (!root) {
        ESP_LOGE(TAG, "Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        ESP_LOGE(TAG, "Not a directory");
        return;
    }

    ESP_LOGI(TAG, "Listing files in LittleFS:");
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            ESP_LOGI(TAG, "File: %s, Size: %d bytes", file.name(), file.size());
        }
        file = root.openNextFile();
    }
} 