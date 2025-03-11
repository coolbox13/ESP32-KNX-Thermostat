#include "protocol_manager.h"
#include "thermostat_state.h"
#include <esp_log.h>

static const char* TAG = "ProtocolManager";

ProtocolManager::ProtocolManager(ThermostatState* state)
    : thermostatState(state)
    , knxInterface(nullptr)
    , mqttInterface(nullptr)
    , lastCommandSource(CommandSource::SOURCE_INTERNAL)
    , lastCommandType(CommandType::CMD_NONE)
    , lastCommandValue(0.0f)
{
}

void ProtocolManager::registerProtocols(KNXInterface* knx, MQTTInterface* mqtt) {
    knxInterface = knx;
    mqttInterface = mqtt;

    if (knxInterface) {
        addProtocol(knxInterface);
    }
    if (mqttInterface) {
        addProtocol(mqttInterface);
    }
}

bool ProtocolManager::begin() {
    bool success = true;
    for (auto protocol : protocols) {
        if (!protocol->begin()) {
            success = false;
            ESP_LOGE(TAG, "Failed to initialize protocol: %s", protocol->getProtocolName());
        }
    }
    return success;
}

void ProtocolManager::loop() {
    for (auto protocol : protocols) {
        protocol->loop();
    }
}

void ProtocolManager::addProtocol(ProtocolInterface* protocol) {
    if (protocol) {
        protocols.push_back(protocol);
        protocol->registerCallbacks(thermostatState, this);
    }
}

void ProtocolManager::removeProtocol(ProtocolInterface* protocol) {
    if (protocol) {
        protocol->unregisterCallbacks();
        auto it = std::find(protocols.begin(), protocols.end(), protocol);
        if (it != protocols.end()) {
            protocols.erase(it);
        }
    }
}

bool ProtocolManager::handleIncomingCommand(CommandSource source, CommandType cmd, float value) {
    // Lock the mutex during command processing
    std::lock_guard<std::mutex> lock(commandMutex);
    
    // Check if the new command has higher priority
    if (!hasHigherPriority(source, lastCommandSource)) {
        return false;
    }

    // Update state based on command type
    bool success = true;
        switch (cmd) {
            case CommandType::CMD_SETPOINT:
                if (thermostatState) {
                    thermostatState->setTargetTemperature(value);
                    lastCommandSource = source;
                    lastCommandType = cmd;
                    lastCommandValue = value;
                    // Only propagate after state changes are complete
                    propagateCommand(source, cmd, value);
                }
                break;

        case CommandType::CMD_MODE:
            if (thermostatState) {
                thermostatState->setMode(static_cast<ThermostatMode>(static_cast<int>(value)));
                lastCommandSource = source;
                lastCommandType = cmd;
                lastCommandValue = value;
                propagateCommand(source, cmd, value);
            }
            break;

        case CommandType::CMD_VALVE:
            if (thermostatState) {
                thermostatState->setValvePosition(value);
                lastCommandSource = source;
                lastCommandType = cmd;
                lastCommandValue = value;
                propagateCommand(source, cmd, value);
            }
            break;

        default:
            success = false;
            break;
    }

    return success;
}

void ProtocolManager::propagateCommand(CommandSource source, CommandType cmd, float value) {
    // Propagate to KNX if not the source
    if (knxInterface && source != CommandSource::SOURCE_KNX) {
        switch (cmd) {
            case CommandType::CMD_SETPOINT:
                knxInterface->sendSetpoint(value);
                break;
            case CommandType::CMD_MODE:
                knxInterface->sendMode(static_cast<ThermostatMode>(static_cast<int>(value)));
                break;
            case CommandType::CMD_VALVE:
                knxInterface->sendValvePosition(value);
                break;
            default:
                break;
        }
    }

    // Propagate to MQTT if not the source
    if (mqttInterface && source != CommandSource::SOURCE_MQTT) {
        switch (cmd) {
            case CommandType::CMD_SETPOINT:
                mqttInterface->sendSetpoint(value);
                break;
            case CommandType::CMD_MODE:
                mqttInterface->sendMode(static_cast<ThermostatMode>(static_cast<int>(value)));
                break;
            case CommandType::CMD_VALVE:
                mqttInterface->sendValvePosition(value);
                break;
            default:
                break;
        }
    }
}

void ProtocolManager::sendTemperature(float temperature) {
    if (knxInterface) knxInterface->sendTemperature(temperature);
    if (mqttInterface) mqttInterface->sendTemperature(temperature);
}

void ProtocolManager::sendSetpoint(float setpoint) {
    if (knxInterface) knxInterface->sendSetpoint(setpoint);
    if (mqttInterface) mqttInterface->sendSetpoint(setpoint);
}

void ProtocolManager::sendValvePosition(float position) {
    if (knxInterface) knxInterface->sendValvePosition(position);
    if (mqttInterface) mqttInterface->sendValvePosition(position);
}

void ProtocolManager::sendMode(ThermostatMode mode) {
    if (knxInterface) knxInterface->sendMode(mode);
    if (mqttInterface) mqttInterface->sendMode(mode);
}

void ProtocolManager::sendHeatingState(bool isHeating) {
    if (knxInterface) knxInterface->sendHeatingState(isHeating);
    if (mqttInterface) mqttInterface->sendHeatingState(isHeating);
}

bool ProtocolManager::hasHigherPriority(CommandSource newSource, CommandSource currentSource) {
    // Priority order: KNX > MQTT > Web > Internal
    int newPriority = 0;
    int currentPriority = 0;

    switch (newSource) {
        case CommandSource::SOURCE_KNX:
            newPriority = 4;
            break;
        case CommandSource::SOURCE_MQTT:
            newPriority = 3;
            break;
        case CommandSource::SOURCE_WEB:
            newPriority = 2;
            break;
        case CommandSource::SOURCE_INTERNAL:
            newPriority = 1;
            break;
    }

    switch (currentSource) {
        case CommandSource::SOURCE_KNX:
            currentPriority = 4;
            break;
        case CommandSource::SOURCE_MQTT:
            currentPriority = 3;
            break;
        case CommandSource::SOURCE_WEB:
            currentPriority = 2;
            break;
        case CommandSource::SOURCE_INTERNAL:
            currentPriority = 1;
            break;
    }

    return newPriority >= currentPriority;
}