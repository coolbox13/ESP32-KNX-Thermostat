#include "thermostat_state.h"
#include "protocol_manager.h"
#include "knx_interface.h"
#include "mqtt_interface.h"

ProtocolManager::ProtocolManager(ThermostatState* state) :
    thermostatState(state),
    knxInterface(nullptr),
    mqttInterface(nullptr),
    lastCommandSource(SOURCE_INTERNAL),
    lastCommandTime(0) {
}

void ProtocolManager::registerProtocols(KNXInterface* knx, MQTTInterface* mqtt) {
    knxInterface = knx;
    mqttInterface = mqtt;
    
    // Register callbacks if state is available
    if (thermostatState && knxInterface) {
        knxInterface->registerCallbacks(thermostatState, this);
    }
    
    if (thermostatState && mqttInterface) {
        mqttInterface->registerProtocolManager(this);
    }
}

bool ProtocolManager::handleIncomingCommand(CommandSource source, CommandType cmd, float value) {
    if (!thermostatState) {
        return false;
    }
    
    // Check command priority
    if (!hasHigherPriority(source, lastCommandSource)) {
        return false;
    }
    
    bool success = false;
    
    switch (cmd) {
        case CMD_SET_TEMPERATURE:
            thermostatState->setTargetTemperature(value);
            // Propagate to other protocols (except the source)
            if (source != SOURCE_KNX && knxInterface) {
                knxInterface->sendSetpoint(value);
            }
            if (source != SOURCE_MQTT && mqttInterface) {
                mqttInterface->sendSetpoint(value);
            }
            success = true;
            break;
            
        case CMD_SET_MODE:
            thermostatState->setMode(static_cast<ThermostatMode>(static_cast<int>(value)));
            // Propagate to other protocols
            if (source != SOURCE_KNX && knxInterface) {
                knxInterface->sendMode(static_cast<ThermostatMode>(static_cast<int>(value)));
            }
            if (source != SOURCE_MQTT && mqttInterface) {
                mqttInterface->sendMode(static_cast<ThermostatMode>(static_cast<int>(value)));
            }
            success = true;
            break;
            
        case CMD_SET_VALVE:
            thermostatState->setValvePosition(value);
            // Propagate to other protocols
            if (source != SOURCE_KNX && knxInterface) {
                knxInterface->sendValvePosition(value);
            }
            if (source != SOURCE_MQTT && mqttInterface) {
                mqttInterface->sendValvePosition(value);
            }
            success = true;
            break;
    }
    
    if (success) {
        lastCommandSource = source;
        lastCommandTime = millis();
    }
    
    return success;
}

void ProtocolManager::sendTemperature(float temperature) {
    // Send to KNX
    if (knxInterface) {
        knxInterface->sendTemperature(temperature);
    }
    
    // Send to MQTT
    if (mqttInterface) {
        mqttInterface->sendTemperature(temperature);
    }
}

void ProtocolManager::sendSetpoint(float setpoint) {
    // Send to KNX
    if (knxInterface) {
        knxInterface->sendSetpoint(setpoint);
    }
    
    // Send to MQTT
    if (mqttInterface) {
        mqttInterface->sendSetpoint(setpoint);
    }
}

void ProtocolManager::sendValvePosition(float position) {
    // Send to KNX
    if (knxInterface) {
        knxInterface->sendValvePosition(position);
    }
    
    // Send to MQTT
    if (mqttInterface) {
        mqttInterface->sendValvePosition(position);
    }
}

void ProtocolManager::sendMode(ThermostatMode mode) {
    // Send to KNX
    if (knxInterface) {
        knxInterface->sendMode(mode);
    }
    
    // Send to MQTT
    if (mqttInterface) {
        mqttInterface->sendMode(mode);
    }
}

void ProtocolManager::sendHeatingState(bool isHeating) {
    // Send to KNX
    if (knxInterface) {
        knxInterface->sendHeatingState(isHeating);
    }
    
    // Send to MQTT
    if (mqttInterface) {
        mqttInterface->sendHeatingState(isHeating);
    }
}

bool ProtocolManager::hasHigherPriority(CommandSource newSource, CommandSource currentSource) {
    // If enough time has passed, any source can take control
    if (millis() - lastCommandTime > PRIORITY_TIMEOUT) {
        return true;
    }
    
    // Priority order: KNX > MQTT > WEB_API > INTERNAL
    return newSource <= currentSource;
}