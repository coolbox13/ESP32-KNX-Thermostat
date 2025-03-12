#pragma once

#include <memory>
#include <ArduinoJson.h>
#include "protocol_interface.h"
#include "protocol_types.h"
#include "thermostat_types.h"
#include "thermostat_state.h"

// Forward declaration of AsyncWebServer to avoid compilation errors
// This is a workaround for the esp-knx-ip library which expects AsyncWebServer
#ifndef AsyncWebServer
class AsyncWebServer;
#endif

// Now include the KNX library
#include "esp-knx-ip.h"

// Forward declarations
class ThermostatState;
class ProtocolManager;

struct KnxGroupAddress {
    uint8_t main;
    uint8_t middle;
    uint8_t sub;
};

class KNXInterface : public ProtocolInterface {
public:
    // Forward declaration of implementation class
    class Impl;
    
    KNXInterface(ThermostatState* state);
    virtual ~KNXInterface();
    
    // ProtocolInterface methods
    virtual bool begin() override;
    virtual void loop() override;
    virtual bool configure(const JsonDocument& config) override;
    virtual bool isConnected() const override;
    virtual ThermostatStatus getLastError() const override;
    
    // KNX specific methods
    void setTemperatureGA(const KnxGroupAddress& ga);
    void setHumidityGA(const KnxGroupAddress& ga);
    void setPressureGA(const KnxGroupAddress& ga);
    void setSetpointGA(const KnxGroupAddress& ga);
    void setValvePositionGA(const KnxGroupAddress& ga);
    void setModeGA(const KnxGroupAddress& ga);
    void setHeatingStateGA(const KnxGroupAddress& ga);
    
    bool sendTemperature(float value);
    bool sendHumidity(float value);
    bool sendPressure(float value);
    bool sendSetpoint(float value);
    bool sendValvePosition(float value);
    bool sendMode(ThermostatMode mode);
    bool sendHeatingState(bool isHeating);
    
    // Core functionality
    void disconnect() override;
    bool reconnect() override;
    
    // Connection configuration
    bool validateConfig() const override;
    void getConfig(JsonDocument& config) const override;
    
    // Protocol registration
    void registerCallbacks(ThermostatState* state, ProtocolManager* manager) override;
    void unregisterCallbacks() override;
    
    // Protocol identification
    const char* getProtocolName() const override { return "KNX"; }
    CommandSource getCommandSource() const override { return CommandSource::SOURCE_KNX; }
    
    // Protocol manager registration
    void registerProtocolManager(ProtocolManager* manager);
    
    // Error handling
    virtual const char* getLastErrorMessage() const override;
    virtual void clearError() override;
    
private:
    ThermostatState* state;
    std::unique_ptr<Impl> pimpl;
    ProtocolManager* protocolManager;
    
    // Helper methods
    bool validateGroupAddress(const KnxGroupAddress& ga) const;
    void setupCallbacks();
    void cleanupCallbacks();
    uint8_t modeToKnx(ThermostatMode mode) const;
    ThermostatMode knxToMode(uint8_t value) const;
};