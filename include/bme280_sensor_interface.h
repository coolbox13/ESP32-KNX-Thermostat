#pragma once

#include "sensor_interface.h"  // Use the abstract sensor interface
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <esp_log.h>
#include "thermostat_types.h"

// I2C pins for BME280 sensor
#define BME280_SDA_PIN 21
#define BME280_SCL_PIN 22

class BME280SensorInterface : public SensorInterface {
public:
    BME280SensorInterface();
    virtual ~BME280SensorInterface() = default;

    bool begin() override;
    void loop() override;
    void updateReadings() override;
    void setUpdateInterval(unsigned long interval) override;
    bool isAvailable() const override;
    
    float getTemperature() const override;
    float getHumidity() const override;
    float getPressure() const override;
    
    void setTemperatureOffset(float offset) override;
    void setHumidityOffset(float offset) override;
    void setPressureOffset(float offset) override;
    
    ThermostatStatus getLastError() const override;
    const char* getLastErrorMessage() const override;
    void clearError() override;
    
private:
    Adafruit_BME280 bme;
    void setSensorMode(); // Internal helper to configure the sensor mode
    
    float temperature;
    float humidity;
    float pressure;
    float temperatureOffset;
    float humidityOffset;
    float pressureOffset;
    unsigned long updateInterval;
    unsigned long lastUpdateTime;
    unsigned long firstErrorTime;  // Time when sensor first reported as unavailable
    bool sensorAvailable;          // Track if sensor is properly initialized
    bool stopErrorMessages;        // Flag to stop continuous error messages
    ThermostatStatus lastError;
    char lastErrorMessage[128];
};