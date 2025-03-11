#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp-knx-ip.h>
#include "communication/knx/knx_interface.h"
#include "protocol_manager.h"
#include "thermostat_state.h"
#include <esp_log.h>
#include <map>
#include <string>
#include <WiFiUdp.h>
#include <esp_wifi.h>

static const char* TAG = "KNXInterface";

// Implementation class definition
class KNXInterface::Impl {
public:
    Impl(ThermostatState* state) : state(state), enabled(false), lastError(ThermostatStatus::OK) {
        memset(lastErrorMessage, 0, sizeof(lastErrorMessage));
    }
    
    WiFiUDP udp;

    // KNX interface initialization
    bool begin() {
        if (!enabled) {
            snprintf(lastErrorMessage, sizeof(lastErrorMessage), "KNX interface not enabled");
            lastError = ThermostatStatus::ERROR_CONFIGURATION;
            return false;
        }
        
        // Configure multicast
        IPAddress multicastIP(224, 0, 23, 12); // KNX multicast address
        uint16_t knxPort = 3671; // Standard KNX port
        
        // Check WiFi
        if (!WiFi.isConnected()) {
            ESP_LOGE(TAG, "WiFi not connected, can't initialize KNX");
            lastError = ThermostatStatus::ERROR_COMMUNICATION;
            return false;
        }
        
        ESP_LOGI(TAG, "Setting up KNX with multicast IP %s on port %d", 
                multicastIP.toString().c_str(), knxPort);
        
        // Use the correct method signature
        if (this->udp.beginMulticast(multicastIP, knxPort) != 1) {
            ESP_LOGE(TAG, "Failed to begin multicast");
            lastError = ThermostatStatus::ERROR_COMMUNICATION;
            return false;
        }
        
        knx.start(nullptr);
        ESP_LOGI(TAG, "KNX interface started successfully");
        return true;
    }
    
    void loop() {
        if (enabled) {
            knx.loop();
        }
    }
    
    bool configure(const JsonDocument& config) {
        // Configure KNX interface
        if (config.containsKey("physical")) {
            // Direct access without conversion to JsonObject
            uint8_t area = 1;    // Default value
            uint8_t line = 1;    // Default value
            uint8_t member = 1;  // Default value
            
            // Read values if they exist
            if (config["physical"].containsKey("area")) {
                area = config["physical"]["area"];
            }
            if (config["physical"].containsKey("line")) {
                line = config["physical"]["line"];
            }
            if (config["physical"].containsKey("member")) {
                member = config["physical"]["member"];
            }
            
            knx.physical_address_set(knx.PA_to_address(area, line, member));
        }
        
        // Configure group addresses - avoid using JsonObject entirely
        if (config.containsKey("ga")) {
            // We'll need to manually check each key we're interested in
            // This is a crude approach but should work without JsonObject conversion
            
            // Example: Check for specific known group addresses
            if (config["ga"].containsKey("temperature")) {
                uint8_t main = config["ga"]["temperature"]["main"] | 0;
                uint8_t middle = config["ga"]["temperature"]["middle"] | 0;
                uint8_t sub = config["ga"]["temperature"]["sub"] | 0;
                address_t addr = knx.GA_to_address(main, middle, sub);
                groupAddresses["temperature"] = addr;
                ESP_LOGI(TAG, "Added temperature group address: %d/%d/%d", main, middle, sub);
            }
            
            if (config["ga"].containsKey("humidity")) {
                uint8_t main = config["ga"]["humidity"]["main"] | 0;
                uint8_t middle = config["ga"]["humidity"]["middle"] | 0;
                uint8_t sub = config["ga"]["humidity"]["sub"] | 0;
                address_t addr = knx.GA_to_address(main, middle, sub);
                groupAddresses["humidity"] = addr;
                ESP_LOGI(TAG, "Added humidity group address: %d/%d/%d", main, middle, sub);
            }
            
            if (config["ga"].containsKey("pressure")) {
                uint8_t main = config["ga"]["pressure"]["main"] | 0;
                uint8_t middle = config["ga"]["pressure"]["middle"] | 0;
                uint8_t sub = config["ga"]["pressure"]["sub"] | 0;
                address_t addr = knx.GA_to_address(main, middle, sub);
                groupAddresses["pressure"] = addr;
                ESP_LOGI(TAG, "Added pressure group address: %d/%d/%d", main, middle, sub);
            }
            
            if (config["ga"].containsKey("setpoint")) {
                uint8_t main = config["ga"]["setpoint"]["main"] | 0;
                uint8_t middle = config["ga"]["setpoint"]["middle"] | 0;
                uint8_t sub = config["ga"]["setpoint"]["sub"] | 0;
                address_t addr = knx.GA_to_address(main, middle, sub);
                groupAddresses["setpoint"] = addr;
                ESP_LOGI(TAG, "Added setpoint group address: %d/%d/%d", main, middle, sub);
            }
            
            if (config["ga"].containsKey("valve")) {
                uint8_t main = config["ga"]["valve"]["main"] | 0;
                uint8_t middle = config["ga"]["valve"]["middle"] | 0;
                uint8_t sub = config["ga"]["valve"]["sub"] | 0;
                address_t addr = knx.GA_to_address(main, middle, sub);
                groupAddresses["valve"] = addr;
                ESP_LOGI(TAG, "Added valve group address: %d/%d/%d", main, middle, sub);
            }
            
            if (config["ga"].containsKey("mode")) {
                uint8_t main = config["ga"]["mode"]["main"] | 0;
                uint8_t middle = config["ga"]["mode"]["middle"] | 0;
                uint8_t sub = config["ga"]["mode"]["sub"] | 0;
                address_t addr = knx.GA_to_address(main, middle, sub);
                groupAddresses["mode"] = addr;
                ESP_LOGI(TAG, "Added mode group address: %d/%d/%d", main, middle, sub);
            }
            
            if (config["ga"].containsKey("heating")) {
                uint8_t main = config["ga"]["heating"]["main"] | 0;
                uint8_t middle = config["ga"]["heating"]["middle"] | 0;
                uint8_t sub = config["ga"]["heating"]["sub"] | 0;
                address_t addr = knx.GA_to_address(main, middle, sub);
                groupAddresses["heating"] = addr;
                ESP_LOGI(TAG, "Added heating group address: %d/%d/%d", main, middle, sub);
            }
        }
        
        enabled = true;
        return true;
    }
    
    bool isConnected() const {
        return enabled;
    }
    
    ThermostatStatus getLastError() const {
        return lastError;
    }
    
    const char* getLastErrorMessage() const {
        return lastErrorMessage;
    }
    
    void clearError() {
        lastError = ThermostatStatus::OK;
        memset(lastErrorMessage, 0, sizeof(lastErrorMessage));
    }
    
    bool setPhysicalAddress(uint8_t area, uint8_t line, uint8_t member) {
        knx.physical_address_set(knx.PA_to_address(area, line, member));
        return true;
    }
    
    bool setGroupAddress(const char* name, uint8_t main, uint8_t middle, uint8_t sub) {
        address_t addr = knx.GA_to_address(main, middle, sub);
        groupAddresses[name] = addr;
        return true;
    }
    
    bool sendValue(const char* gaName, float value) {
        auto it = groupAddresses.find(gaName);
        if (it == groupAddresses.end()) {
            snprintf(lastErrorMessage, sizeof(lastErrorMessage), "Group address %s not found", gaName);
            lastError = ThermostatStatus::ERROR_CONFIGURATION;
            return false;
        }
        
        knx.write_2byte_float(it->second, value);
        return true;
    }
    
    bool sendStatus(const char* gaName, bool status) {
        auto it = groupAddresses.find(gaName);
        if (it == groupAddresses.end()) {
            snprintf(lastErrorMessage, sizeof(lastErrorMessage), "Group address %s not found", gaName);
            lastError = ThermostatStatus::ERROR_CONFIGURATION;
            return false;
        }
        
        knx.write_1bit(it->second, status);
        return true;
    }
    
    // Expose group addresses to KNXInterface
    std::map<std::string, address_t> groupAddresses;
    
    // Make KNXInterface a friend class so it can access private members
    friend class KNXInterface;
    
    // Make these accessible for KNXInterface
    ESPKNXIP knx;
    bool enabled;
    
private:
    ThermostatStatus lastError;
    char lastErrorMessage[128];
    ThermostatState* state;
};

// Use raw pointer initialization
KNXInterface::KNXInterface(ThermostatState* state) : pimpl(new Impl(state)) {}
KNXInterface::~KNXInterface() = default;

bool KNXInterface::begin() {
    return pimpl->begin();
}

void KNXInterface::loop() {
    pimpl->loop();
}

bool KNXInterface::configure(const JsonDocument& config) {
    return pimpl->configure(config);
}

bool KNXInterface::isConnected() const {
    return pimpl->isConnected();
}

ThermostatStatus KNXInterface::getLastError() const {
    return pimpl->getLastError();
}

const char* KNXInterface::getLastErrorMessage() const {
    return pimpl->getLastErrorMessage();
}

void KNXInterface::clearError() {
    pimpl->clearError();
}

void KNXInterface::getConfig(JsonDocument& config) const {
    // Create the knx object if it doesn't exist
    JsonObject knx;
    if (config.containsKey("knx")) {
        knx = config["knx"];
    } else {
        knx = config.createNestedObject("knx");
    }
    
    // Add physical address information
    JsonObject physical;
    if (knx.containsKey("physical")) {
        physical = knx["physical"];
    } else {
        physical = knx.createNestedObject("physical");
    }
    
    address_t addr = pimpl->knx.physical_address_get();
    physical["area"] = addr.pa.area;
    physical["line"] = addr.pa.line;
    physical["member"] = addr.pa.member;
    
    // Add group addresses
    JsonObject ga;
    if (knx.containsKey("ga")) {
        ga = knx["ga"];
    } else {
        ga = knx.createNestedObject("ga");
    }
    
    for (const auto& pair : pimpl->groupAddresses) {
        JsonObject obj;
        if (ga.containsKey(pair.first.c_str())) {
            obj = ga[pair.first.c_str()];
        } else {
            obj = ga.createNestedObject(pair.first.c_str());
        }
        
        // Extract KNX group address components
        obj["main"] = pair.second.ga.area;
        obj["middle"] = pair.second.ga.line;
        obj["sub"] = pair.second.ga.member;
    }
}

void KNXInterface::setTemperatureGA(const KnxGroupAddress& ga) {
    pimpl->setGroupAddress("temperature", ga.main, ga.middle, ga.sub);
}

void KNXInterface::setHumidityGA(const KnxGroupAddress& ga) {
    pimpl->setGroupAddress("humidity", ga.main, ga.middle, ga.sub);
}

void KNXInterface::setPressureGA(const KnxGroupAddress& ga) {
    pimpl->setGroupAddress("pressure", ga.main, ga.middle, ga.sub);
}

void KNXInterface::setSetpointGA(const KnxGroupAddress& ga) {
    pimpl->setGroupAddress("setpoint", ga.main, ga.middle, ga.sub);
}

void KNXInterface::setValvePositionGA(const KnxGroupAddress& ga) {
    pimpl->setGroupAddress("valve", ga.main, ga.middle, ga.sub);
}

void KNXInterface::setModeGA(const KnxGroupAddress& ga) {
    pimpl->setGroupAddress("mode", ga.main, ga.middle, ga.sub);
}

void KNXInterface::setHeatingStateGA(const KnxGroupAddress& ga) {
    pimpl->setGroupAddress("heating", ga.main, ga.middle, ga.sub);
}

bool KNXInterface::sendTemperature(float value) {
    return pimpl->sendValue("temperature", value);
}

bool KNXInterface::sendHumidity(float value) {
    return pimpl->sendValue("humidity", value);
}

bool KNXInterface::sendPressure(float value) {
    return pimpl->sendValue("pressure", value);
}

bool KNXInterface::sendSetpoint(float value) {
    return pimpl->sendValue("setpoint", value);
}

bool KNXInterface::sendValvePosition(float value) {
    return pimpl->sendValue("valve", value);
}

bool KNXInterface::sendMode(ThermostatMode mode) {
    return pimpl->sendValue("mode", static_cast<float>(mode));
}

bool KNXInterface::sendHeatingState(bool isHeating) {
    return pimpl->sendStatus("heating", isHeating);
}

// Core functionality
bool KNXInterface::reconnect() {
    return begin();
}

void KNXInterface::disconnect() {
    if (pimpl) {
        pimpl->enabled = false;
    }
}

bool KNXInterface::validateConfig() const {
    // Simplified implementation
    return true;
}

// Protocol registration
void KNXInterface::registerCallbacks(ThermostatState* thermostatState, ProtocolManager* manager) {
    if (pimpl) {
        this->state = thermostatState;
        this->registerProtocolManager(manager);
    }
}

void KNXInterface::unregisterCallbacks() {
    if (pimpl) {
        this->state = nullptr;
        this->registerProtocolManager(nullptr);
    }
}

void KNXInterface::registerProtocolManager(ProtocolManager* manager) {
    if (pimpl) {
        this->protocolManager = manager;
    }
}

// Helper methods
bool KNXInterface::validateGroupAddress(const KnxGroupAddress& ga) const {
    // Simplified implementation
    return true;
}

void KNXInterface::setupCallbacks() {
    // Simplified implementation
}

void KNXInterface::cleanupCallbacks() {
    // Simplified implementation
}

uint8_t KNXInterface::modeToKnx(ThermostatMode mode) const {
    // Simplified implementation
    return static_cast<uint8_t>(mode);
}

ThermostatMode KNXInterface::knxToMode(uint8_t value) const {
    // Simplified implementation
    return static_cast<ThermostatMode>(value);
}