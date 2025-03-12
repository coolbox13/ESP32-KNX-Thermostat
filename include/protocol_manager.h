#pragma once

#include <vector>
#include "protocol_interface.h"
#include "thermostat_state.h"
#include "knx_interface.h"
#include "mqtt_interface.h"
#include "protocol_types.h"
#include <mutex>
// Forward declarations
class KNXInterface;
class MQTTInterface;

class ProtocolManager {
public:
    ProtocolManager(ThermostatState* state);
    virtual ~ProtocolManager() = default;

    // Core functionality
    bool begin();
    void loop();
    void update() { loop(); }  // Alias for loop()
    void addProtocol(ProtocolInterface* protocol);
    void removeProtocol(ProtocolInterface* protocol);

    // Protocol registration
    void registerProtocols(KNXInterface* knx, MQTTInterface* mqtt);

    // Protocol command handling
    bool handleIncomingCommand(CommandSource source, CommandType cmd, float value);
    void propagateCommand(CommandSource source, CommandType cmd, float value);

    // State updates
    void sendTemperature(float temperature);
    void sendSetpoint(float setpoint);
    void sendValvePosition(float position);
    void sendMode(ThermostatMode mode);
    void sendHeatingState(bool isHeating);

private:
    ThermostatState* thermostatState;
    std::vector<ProtocolInterface*> protocols;
    
    // Protocol instances
    KNXInterface* knxInterface;
    MQTTInterface* mqttInterface;
    
    // Command tracking
    CommandSource lastCommandSource;
    CommandType lastCommandType;
    float lastCommandValue;

    // Helper methods
    bool hasHigherPriority(CommandSource newSource, CommandSource currentSource);

    // avoid race conditions
    std::mutex commandMutex;
};