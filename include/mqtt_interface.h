#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "thermostat_types.h"
#include "protocol_types.h"
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <memory>
#include "protocol_interface.h"
#include "protocol_manager.h"
#include "thermostat_state.h"

// Forward declarations
class ThermostatState;
class ProtocolManager;

class MQTTInterface : public ProtocolInterface {
public:
    MQTTInterface(ThermostatState* state);
    virtual ~MQTTInterface();

    // Core functionality
    bool begin() override;
    void loop() override;
    bool isConnected() const override;
    void disconnect() override;
    bool reconnect() override;

    // Connection configuration
    bool configure(const JsonDocument& config) override;
    bool validateConfig() const override;
    void getConfig(JsonDocument& config) const override;

    // Data transmission
    bool sendTemperature(float value) override;
    bool sendHumidity(float value) override;
    bool sendPressure(float value) override;
    bool sendSetpoint(float value) override;
    bool sendValvePosition(float value) override;
    bool sendMode(ThermostatMode mode) override;
    bool sendHeatingState(bool isHeating) override;

    // Error handling
    ThermostatStatus getLastError() const override;
    const char* getLastErrorMessage() const override;
    void clearError() override;

    // Protocol registration
    void registerCallbacks(ThermostatState* state, ProtocolManager* manager) override;
    void unregisterCallbacks() override;

    // Protocol identification
    const char* getProtocolName() const override { return "MQTT"; }
    CommandSource getCommandSource() const override { return CommandSource::SOURCE_MQTT; }

    // Enable/disable MQTT
    void setEnabled(bool enabled);

    // MQTT specific configuration
    void setServer(const char* server, uint16_t port = 1883);
    void setCredentials(const char* username, const char* password);
    void setClientId(const char* clientId);
    void setTopicPrefix(const char* prefix);
    bool isEnabled() const;

    // Protocol manager registration
    void registerProtocolManager(ProtocolManager* manager);

private:
    // Implementation details
    class Impl;
    std::unique_ptr<Impl> pimpl;

    // Message handling
    bool publish(const char* topic, const char* payload, bool retain = true);
    void setupSubscriptions();
    void cleanupSubscriptions();
    void handleMessage(char* topic, uint8_t* payload, unsigned int length);
    static void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
    String getFullTopic(const char* suffix) const;

    // Validation
    bool validateConnection() const;
    bool validateTopics() const;

    ThermostatState* state;
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    bool enabled;
    bool initialized;

    String server;
    uint16_t port;
    String username;
    String password;
    String clientId;
    String topicPrefix;

    void connect();
    void publishState();
    void handleCallback(char* topic, byte* payload, unsigned int length);
    static void callbackWrapper(char* topic, byte* payload, unsigned int length, void* arg);
};