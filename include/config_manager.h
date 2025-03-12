#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <DNSServer.h>
#include "thermostat_types.h"
#include "pid_controller.h"
#include "config_interface.h"

// Forward declarations
class ThermostatState;
class ProtocolManager;

struct KNXPhysicalAddress {
    uint8_t area;
    uint8_t line;
    uint8_t member;
};

class ConfigManager : public ConfigInterface {
public:
    ConfigManager();
    virtual ~ConfigManager() = default;
    
    // ConfigInterface implementation
    bool begin() override;
    bool load() override { return loadConfig(); }
    bool save() override { return saveConfig(); }
    void reset() override { resetToDefaults(); }
    
    // Additional methods
    void end();
    bool setupWiFi();
    bool loadConfig();
    bool saveConfig();
    void resetToDefaults();

    // WiFi settings
    const char* getWiFiSSID() const { return wifiSSID; }
    const char* getWiFiPassword() const { return wifiPassword; }
    void setWiFiSSID(const char* ssid);
    void setWiFiPassword(const char* password);

    // Device settings
    const char* getDeviceName() const override;
    void setDeviceName(const char* name) override;
    unsigned long getSendInterval() const override;
    void setSendInterval(unsigned long interval) override;
    void setPidInterval(uint32_t interval);
    uint32_t getPidInterval() const;

    // Web interface settings
    const char* getWebUsername() const override;
    void setWebUsername(const char* username) override;
    const char* getWebPassword() const override;
    void setWebPassword(const char* password) override;

    // KNX settings
    bool getKnxEnabled() const override;
    void setKnxEnabled(bool enabled) override;
    void setKnxPhysicalAddress(uint8_t area, uint8_t line, uint8_t member);
    void getKnxPhysicalAddress(uint8_t& area, uint8_t& line, uint8_t& member) const;
    
    // KNX group addresses
    void setKnxTemperatureGA(uint8_t area, uint8_t line, uint8_t member);
    void getKnxTemperatureGA(uint8_t& area, uint8_t& line, uint8_t& member) const;
    
    void setKnxSetpointGA(uint8_t area, uint8_t line, uint8_t member);
    void getKnxSetpointGA(uint8_t& area, uint8_t& line, uint8_t& member) const;
    
    void setKnxValveGA(uint8_t area, uint8_t line, uint8_t member);
    void getKnxValveGA(uint8_t& area, uint8_t& line, uint8_t& member) const;
    
    void setKnxModeGA(uint8_t area, uint8_t line, uint8_t member);
    void getKnxModeGA(uint8_t& area, uint8_t& line, uint8_t& member) const;
    
    // MQTT settings
    bool getMqttEnabled() const override;
    //bool getMqttEnabled() const override { return mqttEnabled; }
    void setMqttEnabled(bool enabled) override { setMQTTEnabled(enabled); }
    void setMQTTEnabled(bool enabled);
    uint16_t getMQTTPort() const { return mqttPort; }
    const char* getMQTTServer() const { return mqttServer; }
    const char* getMQTTUser() const { return mqttUser; }
    const char* getMQTTPassword() const { return mqttPassword; }
    const char* getMQTTClientId() const { return mqttClientId; }
    const char* getMQTTTopicPrefix() const { return mqttTopicPrefix; }
    void setMQTTServer(const char* server);
    void setMQTTPort(uint16_t port);
    void setMQTTUser(const char* user);
    void setMQTTPassword(const char* password);
    void setMQTTClientId(const char* clientId);
    void setMQTTTopicPrefix(const char* prefix);
    
    // Control parameters
    float getSetpoint() const override;
    void setSetpoint(float setpoint) override;
    float getKp() const override { return pidConfig.kp; }
    void setKp(float value) override { pidConfig.kp = value; }
    float getKi() const override { return pidConfig.ki; }
    void setKi(float value) override { pidConfig.ki = value; }
    float getKd() const override { return pidConfig.kd; }
    void setKd(float value) override { pidConfig.kd = value; }
    
    const PIDConfig& getPidConfig() const;
    void setPidConfig(const PIDConfig& config);
    
    // Status
    ThermostatStatus getLastError() const override { return lastError; }

private:
    // Preferences storage
    Preferences prefs;
    
    // Configuration file path
    static const char* configFilePath;
    
    // WiFi settings
    char wifiSSID[32];
    char wifiPassword[64];
    
    // Device settings
    char deviceName[32];
    uint32_t sendInterval;
    uint32_t pidUpdateInterval;
    
    // Web interface settings
    char webUsername[32];
    char webPassword[32];
    
    // KNX settings
    bool knxEnabled;
    KNXPhysicalAddress knxPhysicalAddress;
    KNXPhysicalAddress knxTemperatureGA;
    KNXPhysicalAddress knxSetpointGA;
    KNXPhysicalAddress knxValveGA;
    KNXPhysicalAddress knxModeGA;
    
    // MQTT settings
    bool mqttEnabled;
    char mqttServer[64];
    uint16_t mqttPort;
    char mqttUser[32];
    char mqttPassword[32];
    char mqttClientId[32];
    char mqttTopicPrefix[32];
    
    // Control parameters
    float setpoint;
    PIDConfig pidConfig;
    
    // Status
    ThermostatStatus lastError;
    
    // Helper methods
    void loadDefaults();
    bool saveJsonConfig();
    bool loadJsonConfig();
};