#include "mqtt_interface.h"
#include "thermostat_state.h"
#include "protocol_manager.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <memory>
#include <esp_log.h>

static const char* TAG = "MQTTInterface";

// Define make_unique for C++11 compatibility
#if __cplusplus < 201402L
namespace std {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
#endif

// Static instance pointer for callbacks
static MQTTInterface* instance = nullptr;

// Implementation class
class MQTTInterface::Impl {
public:
    // MQTT client
    WiFiClient espClient;
    PubSubClient client;
    
    // Connection settings
    char server[64] = {0};
    uint16_t port = 1883;
    char username[32] = {0};
    char password[32] = {0};
    char clientId[32] = "esp32_thermostat";
    char topicPrefix[64] = "esp32/thermostat/";
    
    // State
    bool connected = false;
    ThermostatStatus lastError = ThermostatStatus::OK;
    char lastErrorMessage[128] = {0};
    unsigned long lastReconnectAttempt = 0;
    ThermostatState* thermostatState = nullptr;
    ProtocolManager* protocolManager = nullptr;
    
    // Topics
    char temperatureTopic[128] = {0};
    char humidityTopic[128] = {0};
    char pressureTopic[128] = {0};
    char setpointTopic[128] = {0};
    char modeTopic[128] = {0};
    char valveTopic[128] = {0};
    char heatingTopic[128] = {0};
    char statusTopic[128] = {0};
    
    // Constructor
    Impl() : client(espClient) {
        client.setBufferSize(512); // Increase buffer size for larger messages
        // Initialize with default values
        enabled = false;
        connected = false;
        lastError = ThermostatStatus::OK;
        
        // Set default MQTT topics
        strcpy(topicPrefix, "esp32/thermostat/");
        strcpy(temperatureTopic, "temperature");
        strcpy(humidityTopic, "humidity");
        strcpy(setpointTopic, "setpoint");
        strcpy(modeTopic, "mode");
        strcpy(valveTopic, "valve");
        strcpy(heatingTopic, "heating");
        strcpy(statusTopic, "status");
        
        // Initialize client
        client.setCallback([](char* topic, byte* payload, unsigned int length) {
            if (instance) {
                instance->handleMessage(topic, payload, length);
            }
        });
    }

    // Configuration
    bool enabled;
    
    // Message handler
    void handleMessage(char* topic, byte* payload, unsigned int length);
};

// Constructor implementation
MQTTInterface::MQTTInterface(ThermostatState* state) : pimpl(new Impl()) {
    pimpl->thermostatState = state;
    instance = this;
}

MQTTInterface::~MQTTInterface() {
    // Disconnect if connected
    if (pimpl->connected) {
        pimpl->client.disconnect();
    }
    // Clean up static instance pointer
    if (instance == this) {
        instance = nullptr;
    }
}

// Connection management
bool MQTTInterface::begin() {
    if (!pimpl->enabled) {
        ESP_LOGI(TAG, "MQTT disabled, not connecting");
        return false;
    }
    
    ESP_LOGI(TAG, "Connecting to MQTT broker at %s:%d", pimpl->server, pimpl->port);
    pimpl->client.setServer(pimpl->server, pimpl->port);
    pimpl->client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->handleMessage(topic, payload, length);
    });
    
    return reconnect();
}

void MQTTInterface::loop() {
    if (!pimpl->enabled) {
        return;
    }
    
    // Check if connected
    if (!pimpl->connected) {
        // Try to reconnect every 5 seconds
        static unsigned long lastReconnectAttempt = 0;
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            if (reconnect()) {
                lastReconnectAttempt = 0;
            }
        }
        return;
    }
    
    // Process MQTT messages
    pimpl->client.loop();
}

bool MQTTInterface::isConnected() const {
    return pimpl->connected;
}

void MQTTInterface::disconnect() {
    pimpl->client.disconnect();
    pimpl->connected = false;
}

bool MQTTInterface::reconnect() {
    if (!pimpl->enabled) {
        ESP_LOGI(TAG, "MQTT disabled, not attempting reconnection");
        return false;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        pimpl->lastError = ThermostatStatus::ERROR_COMMUNICATION;
        snprintf(pimpl->lastErrorMessage, sizeof(pimpl->lastErrorMessage), 
                 "Cannot connect to MQTT: WiFi not connected");
        ESP_LOGW(TAG, "%s", pimpl->lastErrorMessage);
        return false;
    }
    
    ESP_LOGI(TAG, "Attempting MQTT connection to %s:%d...", pimpl->server, pimpl->port);
    
    // Set connection timeout
    pimpl->client.setSocketTimeout(10); // 10 seconds timeout
    
    // Try to connect with username/password or just client ID
    bool result = false;
    int attempts = 0;
    const int maxAttempts = 3;
    
    while (!result && attempts < maxAttempts) {
        attempts++;
        
        if (strlen(pimpl->username) > 0) {
            result = pimpl->client.connect(pimpl->clientId, pimpl->username, pimpl->password);
        } else {
            result = pimpl->client.connect(pimpl->clientId);
        }
        
        if (!result && attempts < maxAttempts) {
            ESP_LOGW(TAG, "Connection attempt %d failed, retrying...", attempts);
            delay(1000); // Brief delay between attempts
        }
    }
    
    if (result) {
        ESP_LOGI(TAG, "Connected to MQTT broker");
        pimpl->connected = true;
        
        // Subscribe to topics with error handling
        String setpointTopicFull = String(pimpl->topicPrefix) + pimpl->setpointTopic + "/set";
        String modeTopicFull = String(pimpl->topicPrefix) + pimpl->modeTopic + "/set";
        
        if (!pimpl->client.subscribe(setpointTopicFull.c_str())) {
            ESP_LOGW(TAG, "Failed to subscribe to %s", setpointTopicFull.c_str());
        }
        
        if (!pimpl->client.subscribe(modeTopicFull.c_str())) {
            ESP_LOGW(TAG, "Failed to subscribe to %s", modeTopicFull.c_str());
        }
        
        // Publish initial status
        if (!publish(pimpl->statusTopic, "online", true)) {
            ESP_LOGW(TAG, "Failed to publish initial status");
        }
        
        pimpl->lastError = ThermostatStatus::OK;
        memset(pimpl->lastErrorMessage, 0, sizeof(pimpl->lastErrorMessage));
        return true;
    } else {
        int state = pimpl->client.state();
        pimpl->connected = false;
        pimpl->lastError = ThermostatStatus::ERROR_COMMUNICATION;
        
        // Provide detailed error message based on state code
        const char* errorMessage;
        switch (state) {
            case -4: errorMessage = "Connection timeout"; break;
            case -3: errorMessage = "Connection lost"; break;
            case -2: errorMessage = "Connection failed"; break;
            case -1: errorMessage = "Disconnected"; break;
            case 1: errorMessage = "Bad protocol"; break;
            case 2: errorMessage = "Bad client ID"; break;
            case 3: errorMessage = "Server unavailable"; break;
            case 4: errorMessage = "Bad credentials"; break;
            case 5: errorMessage = "Unauthorized"; break;
            default: errorMessage = "Unknown error"; break;
        }
        
        snprintf(pimpl->lastErrorMessage, sizeof(pimpl->lastErrorMessage),
                "MQTT connection failed: %s (code %d)", errorMessage, state);
        ESP_LOGE(TAG, "%s", pimpl->lastErrorMessage);
        
        return false;
    }
}

// Configuration
bool MQTTInterface::configure(const JsonDocument& config) {
    if (!config["server"].is<const char*>() || !config["port"].is<uint16_t>()) {
        ESP_LOGE(TAG, "Invalid MQTT configuration");
        pimpl->lastError = ThermostatStatus::ERROR_CONFIGURATION;
        return false;
    }
    
    // Copy configuration
    strlcpy(pimpl->server, config["server"] | "localhost", sizeof(pimpl->server));
    pimpl->port = config["port"] | 1883;
    
    if (config["username"].is<const char*>()) {
        strlcpy(pimpl->username, config["username"], sizeof(pimpl->username));
    }
    
    if (config["password"].is<const char*>()) {
        strlcpy(pimpl->password, config["password"], sizeof(pimpl->password));
    }
    
    if (config["clientId"].is<const char*>()) {
        strlcpy(pimpl->clientId, config["clientId"], sizeof(pimpl->clientId));
    }
    
    if (config["topicPrefix"].is<const char*>()) {
        strlcpy(pimpl->topicPrefix, config["topicPrefix"], sizeof(pimpl->topicPrefix));
    }
    
    // Enable MQTT
    pimpl->enabled = config["enabled"] | false;
    
    return true;
}

void MQTTInterface::getConfig(JsonDocument& config) const {
    config["enabled"] = pimpl->enabled;
    config["server"] = pimpl->server;
    config["port"] = pimpl->port;
    config["username"] = pimpl->username;
    config["password"] = pimpl->password;
    config["clientId"] = pimpl->clientId;
    config["topicPrefix"] = pimpl->topicPrefix;
}

// Data transmission
bool MQTTInterface::sendTemperature(float temperature) {
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%.2f", temperature);
    return publish(pimpl->temperatureTopic, buffer);
}

bool MQTTInterface::sendHumidity(float humidity) {
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%.2f", humidity);
    return publish(pimpl->humidityTopic, buffer);
}

bool MQTTInterface::sendPressure(float value) {
    char payload[16];
    snprintf(payload, sizeof(payload), "%.2f", value);
    return publish(getFullTopic("pressure").c_str(), payload);
}

bool MQTTInterface::sendSetpoint(float setpoint) {
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%.2f", setpoint);
    return publish(pimpl->setpointTopic, buffer);
}

bool MQTTInterface::sendValvePosition(float position) {
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%.2f", position);
    return publish(pimpl->valveTopic, buffer);
}

bool MQTTInterface::sendMode(ThermostatMode mode) {
    if (!isConnected()) {
        return false;
    }

    const char* modeStr = getThermostatModeName(mode);
    return publish(pimpl->modeTopic, modeStr);
}

bool MQTTInterface::sendHeatingState(bool isHeating) {
    return publish(pimpl->heatingTopic, isHeating ? "ON" : "OFF");
}

// Error handling
ThermostatStatus MQTTInterface::getLastError() const {
    return pimpl->lastError;
}

const char* MQTTInterface::getLastErrorMessage() const {
    return pimpl->lastErrorMessage;
}

void MQTTInterface::clearError() {
    pimpl->lastError = ThermostatStatus::OK;
    pimpl->lastErrorMessage[0] = '\0';
}

// Protocol registration
void MQTTInterface::registerCallbacks(ThermostatState* state, ProtocolManager* manager) {
    pimpl->thermostatState = state;
    pimpl->protocolManager = manager;
}

void MQTTInterface::unregisterCallbacks() {
    pimpl->thermostatState = nullptr;
    pimpl->protocolManager = nullptr;
}

// MQTT specific configuration
void MQTTInterface::setServer(const char* server, uint16_t port) {
    strlcpy(pimpl->server, server, sizeof(pimpl->server));
    pimpl->port = port;
}

void MQTTInterface::setCredentials(const char* username, const char* password) {
    strlcpy(pimpl->username, username, sizeof(pimpl->username));
    strlcpy(pimpl->password, password, sizeof(pimpl->password));
}

void MQTTInterface::setClientId(const char* clientId) {
    strlcpy(pimpl->clientId, clientId, sizeof(pimpl->clientId));
}

void MQTTInterface::setTopicPrefix(const char* prefix) {
    strlcpy(pimpl->topicPrefix, prefix, sizeof(pimpl->topicPrefix));
}

// Internal helpers
bool MQTTInterface::validateConnection() const {
    if (strlen(pimpl->server) == 0) {
        return false;
    }
    if (pimpl->port == 0) {
        return false;
    }
    if (strlen(pimpl->clientId) == 0) {
        return false;
    }
    return true;
}

bool MQTTInterface::validateTopics() const {
    return strlen(pimpl->topicPrefix) > 0;
}

bool MQTTInterface::publish(const char* topic, const char* payload, bool retain) {
    if (!pimpl->enabled || !pimpl->connected) {
        ESP_LOGE(TAG, "MQTT not connected, can't publish");
        pimpl->lastError = ThermostatStatus::ERROR_COMMUNICATION;
        return false;
    }
    
    String fullTopic = String(pimpl->topicPrefix) + topic;
    if (!pimpl->client.publish(fullTopic.c_str(), payload, retain)) {
        ESP_LOGE(TAG, "Failed to publish to %s", fullTopic.c_str());
        pimpl->lastError = ThermostatStatus::ERROR_COMMUNICATION;
        return false;
    }
    
    return true;
}

void MQTTInterface::handleMessage(char* topic, byte* payload, unsigned int length) {
    if (!pimpl->thermostatState || !pimpl->protocolManager) {
        return;
    }

    String topicStr(topic);
    String payloadStr;
    payloadStr.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }

    // Handle setpoint changes
    if (topicStr.endsWith("/setpoint/set")) {
        float setpoint = payloadStr.toFloat();
        pimpl->thermostatState->setTargetTemperature(setpoint);
    }
    // Handle mode changes
    else if (topicStr.endsWith("/mode/set")) {
        ThermostatMode mode;
        String modeStr = String((char*)payload).substring(0, length);
        
        if (modeStr == "off") mode = ThermostatMode::OFF;
        else if (modeStr == "comfort") mode = ThermostatMode::COMFORT;
        else if (modeStr == "eco") mode = ThermostatMode::ECO;
        else if (modeStr == "away") mode = ThermostatMode::AWAY;
        else if (modeStr == "boost") mode = ThermostatMode::BOOST;
        else if (modeStr == "antifreeze") mode = ThermostatMode::ANTIFREEZE;
        else {
            ESP_LOGW(TAG, "Invalid mode received: %s", modeStr.c_str());
            return;
        }
        
        if (pimpl->protocolManager) {
            pimpl->protocolManager->handleIncomingCommand(CommandSource::SOURCE_MQTT, CommandType::CMD_MODE, static_cast<float>(mode));
        }
    }
}

String MQTTInterface::getFullTopic(const char* suffix) const {
    String fullTopic = pimpl->topicPrefix;
    fullTopic += suffix;
    return fullTopic;
}

void MQTTInterface::setEnabled(bool enabled) {
    if (enabled && !isConnected()) {
        reconnect();
    } else if (!enabled && isConnected()) {
        disconnect();
    }
}

bool MQTTInterface::isEnabled() const {
    return pimpl->enabled;
}

bool MQTTInterface::validateConfig() const {
    if (!pimpl->enabled) {
        return true;  // If disabled, config is valid
    }
    
    // Check required fields
    if (strlen(pimpl->server) == 0) {
        return false;
    }
    if (pimpl->port == 0) {
        return false;
    }
    if (strlen(pimpl->clientId) == 0) {
        return false;
    }
    
    // Optional fields don't need validation
    return true;
}

void MQTTInterface::registerProtocolManager(ProtocolManager* manager) {
    pimpl->protocolManager = manager;
}