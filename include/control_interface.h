#ifndef CONTROL_INTERFACE_H
#define CONTROL_INTERFACE_H

#include "thermostat_types.h"

class ControlInterface {
public:
    virtual ~ControlInterface() = default;

    // Initialization and update
    virtual bool begin() = 0;
    virtual void loop() = 0;
    virtual void setUpdateInterval(unsigned long interval) = 0;

    // Control parameters
    virtual void setSetpoint(float value) = 0;
    virtual void setInput(float value) = 0;
    virtual float getOutput() const = 0;

    // PID parameters
    virtual float getKp() const = 0;
    virtual float getKi() const = 0;
    virtual float getKd() const = 0;

    // Status
    virtual bool isActive() const = 0;
    virtual void setActive(bool active) = 0;
    virtual ThermostatStatus getLastError() const = 0;
    virtual const char* getLastErrorMessage() const = 0;
    virtual void clearError() = 0;

    // Configuration
    virtual void reset() = 0;
    virtual void configure(const void* config) = 0;
    virtual bool saveConfig() = 0;
};

#endif // CONTROL_INTERFACE_H 