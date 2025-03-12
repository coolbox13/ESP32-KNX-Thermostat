#ifndef CONFIG_INTERFACE_H
#define CONFIG_INTERFACE_H

#include "thermostat_types.h"
#include <Arduino.h>

class ConfigInterface {
public:
    virtual ~ConfigInterface() = default;

    // Basic operations
    virtual bool begin() = 0;
    virtual bool load() = 0;
    virtual bool save() = 0;
    virtual void reset() = 0;

    // Device configuration
    virtual const char* getDeviceName() const = 0;
    virtual void setDeviceName(const char* name) = 0;
    virtual unsigned long getSendInterval() const = 0;
    virtual void setSendInterval(unsigned long interval) = 0;

    // Web interface configuration
    virtual const char* getWebUsername() const = 0;
    virtual void setWebUsername(const char* username) = 0;
    virtual const char* getWebPassword() const = 0;
    virtual void setWebPassword(const char* password) = 0;

    // Control configuration
    virtual float getKp() const = 0;
    virtual void setKp(float value) = 0;
    virtual float getKi() const = 0;
    virtual void setKi(float value) = 0;
    virtual float getKd() const = 0;
    virtual void setKd(float value) = 0;
    virtual float getSetpoint() const = 0;
    virtual void setSetpoint(float value) = 0;

    // Protocol configuration
    virtual bool getKnxEnabled() const = 0;
    virtual void setKnxEnabled(bool enabled) = 0;
    virtual bool getMqttEnabled() const = 0;
    virtual void setMqttEnabled(bool enabled) = 0;

    // Status
    virtual ThermostatStatus getLastError() const = 0;
};

#endif // CONFIG_INTERFACE_H 