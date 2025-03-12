#ifndef PROTOCOL_INTERFACE_H
#define PROTOCOL_INTERFACE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "thermostat_types.h"
#include "protocol_types.h"

// Forward declarations
class ThermostatState;
class ProtocolManager;

class ProtocolInterface {
public:
    virtual ~ProtocolInterface() = default;

    // Core functionality
    virtual bool begin() = 0;
    virtual void loop() = 0;
    virtual bool isConnected() const = 0;
    virtual void disconnect() = 0;
    virtual bool reconnect() = 0;

    // Connection configuration
    virtual bool configure(const JsonDocument& config) = 0;
    virtual bool validateConfig() const = 0;
    virtual void getConfig(JsonDocument& config) const = 0;

    // Data transmission
    virtual bool sendTemperature(float value) = 0;
    virtual bool sendHumidity(float value) = 0;
    virtual bool sendPressure(float value) = 0;
    virtual bool sendSetpoint(float value) = 0;
    virtual bool sendValvePosition(float value) = 0;
    virtual bool sendMode(ThermostatMode mode) = 0;
    virtual bool sendHeatingState(bool isHeating) = 0;

    // Error handling
    virtual ThermostatStatus getLastError() const = 0;
    virtual const char* getLastErrorMessage() const = 0;
    virtual void clearError() = 0;

    // Protocol registration
    virtual void registerCallbacks(ThermostatState* state, ProtocolManager* manager) = 0;
    virtual void unregisterCallbacks() = 0;

    // Protocol identification
    virtual const char* getProtocolName() const = 0;
    virtual CommandSource getCommandSource() const = 0;

protected:
    // Helper methods for derived classes
    void setLastError(ThermostatStatus status, const char* message) {
        lastError = status;
        if (message) {
            strncpy(lastErrorMessage, message, sizeof(lastErrorMessage) - 1);
            lastErrorMessage[sizeof(lastErrorMessage) - 1] = '\0';
        } else {
            lastErrorMessage[0] = '\0';
        }
    }

    void clearLastError() {
        lastError = ThermostatStatus::OK;
        lastErrorMessage[0] = '\0';
    }

    ThermostatStatus lastError = ThermostatStatus::OK;
    char lastErrorMessage[128] = {0};
    bool connected = false;
    ThermostatState* thermostatState = nullptr;
    ProtocolManager* protocolManager = nullptr;
};

#endif // PROTOCOL_INTERFACE_H 