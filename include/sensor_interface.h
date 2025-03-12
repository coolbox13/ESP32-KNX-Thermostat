#ifndef SENSOR_INTERFACE_H
#define SENSOR_INTERFACE_H

#include "thermostat_types.h"

class SensorInterface {
public:
    virtual ~SensorInterface() = default;

    // Initialization and update
    virtual bool begin() = 0;
    virtual void loop() = 0;
    virtual void setUpdateInterval(unsigned long interval) = 0;
    virtual void updateReadings() = 0;

    // Sensor readings
    virtual float getTemperature() const = 0;
    virtual float getHumidity() const = 0;
    virtual float getPressure() const = 0;
    
    // Status
    virtual bool isAvailable() const = 0;
    virtual ThermostatStatus getLastError() const = 0;
    virtual const char* getLastErrorMessage() const = 0;
    virtual void clearError() = 0;
    
    // Calibration
    virtual void setTemperatureOffset(float offset) = 0;
    virtual void setHumidityOffset(float offset) = 0;
    virtual void setPressureOffset(float offset) = 0;
};

#endif // SENSOR_INTERFACE_H 