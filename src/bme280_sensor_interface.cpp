#include "bme280_sensor_interface.h"
#include <Wire.h>
#include <esp_log.h>

static const char* TAG = "BME280";

BME280SensorInterface::BME280SensorInterface()
    : temperature(0)
    , humidity(0)
    , pressure(0)
    , temperatureOffset(0)
    , humidityOffset(0)
    , pressureOffset(0)
    , updateInterval(30000)  // Default 30 seconds
    , lastUpdateTime(0)
    , firstErrorTime(0)
    , sensorAvailable(false)
    , stopErrorMessages(false)
    , lastError(ThermostatStatus::OK) {
    memset(lastErrorMessage, 0, sizeof(lastErrorMessage));
}

bool BME280SensorInterface::begin() {
    ESP_LOGI(TAG, "Initializing BME280 sensor...");
    
    // Initialize I2C bus
    Wire.begin(BME280_SDA_PIN, BME280_SCL_PIN);
    delay(100); // Brief delay to ensure I2C bus is ready
    
    // Simple initialization approach like in the test script
    if (bme.begin(0x76)) {
        ESP_LOGI(TAG, "BME280 sensor found and initialized!");
        sensorAvailable = true;
        stopErrorMessages = false;
        clearError();
        return true;
    }
    
    // Try alternate address as fallback
    if (bme.begin(0x77)) {
        ESP_LOGI(TAG, "BME280 sensor found at alternate address and initialized!");
        sensorAvailable = true;
        stopErrorMessages = false;
        clearError();
        return true;
    }
    
    // Sensor not found - log once and continue
    ESP_LOGE(TAG, "Could not find BME280 sensor! Check your wiring.");
    ESP_LOGW(TAG, "Continuing without BME280 sensor - other functionality will work");
    sensorAvailable = false;
    stopErrorMessages = true; // Don't log any more errors
    lastError = ThermostatStatus::ERROR_SENSOR;
    snprintf(lastErrorMessage, sizeof(lastErrorMessage)-1, "Could not find BME280 sensor");
    
    return false; // Return false but system will continue
}

void BME280SensorInterface::setSensorMode() {
    // Use default settings - the sensor works fine with these in the test script
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                   Adafruit_BME280::SAMPLING_X16,  // temperature
                   Adafruit_BME280::SAMPLING_X16,  // pressure
                   Adafruit_BME280::SAMPLING_X16,  // humidity
                   Adafruit_BME280::FILTER_X16,
                   Adafruit_BME280::STANDBY_MS_0_5);
}

void BME280SensorInterface::loop() {
    // If sensor is not available, just return without doing anything
    // This allows other system functionality to continue
    if (!sensorAvailable) {
        return;
    }
    
    // Only update readings at the specified interval if sensor is available
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= updateInterval) {
        updateReadings();
        lastUpdateTime = currentTime;
    }
}

void BME280SensorInterface::updateReadings() {
    // Skip if sensor is not available
    if (!sensorAvailable) {
        return; // Just return silently
    }
    
    // Simple reading approach like in the test script
    float tempValue = bme.readTemperature();
    float humValue = bme.readHumidity();
    float pressValue = bme.readPressure() / 100.0F; // Convert Pa to hPa
    
    // Check for invalid readings
    if (isnan(tempValue) || isnan(humValue) || isnan(pressValue)) {
        ESP_LOGW(TAG, "Invalid sensor readings detected");
        sensorAvailable = false;
        lastError = ThermostatStatus::ERROR_SENSOR;
        snprintf(lastErrorMessage, sizeof(lastErrorMessage)-1, "Invalid sensor readings");
        return;
    }
    
    // Apply calibration offsets
    temperature = tempValue + temperatureOffset;
    humidity = humValue + humidityOffset;
    if (humidity < 0) humidity = 0;
    if (humidity > 100) humidity = 100;
    pressure = pressValue + pressureOffset;
    
    // Log the readings at debug level
    ESP_LOGD(TAG, "Sensor readings: Temp=%.2f°C, Humidity=%.2f%%, Pressure=%.2fhPa", 
             temperature, humidity, pressure);
    
    // Clear any previous errors
    clearError();
}

void BME280SensorInterface::setUpdateInterval(unsigned long interval) {
    updateInterval = interval;
}

bool BME280SensorInterface::isAvailable() const {
    return sensorAvailable;
}

float BME280SensorInterface::getTemperature() const {
    return temperature;
}

float BME280SensorInterface::getHumidity() const {
    return humidity;
}

float BME280SensorInterface::getPressure() const {
    return pressure;
}

void BME280SensorInterface::setTemperatureOffset(float offset) {
    temperatureOffset = offset;
}

void BME280SensorInterface::setHumidityOffset(float offset) {
    humidityOffset = offset;
}

void BME280SensorInterface::setPressureOffset(float offset) {
    pressureOffset = offset;
}

ThermostatStatus BME280SensorInterface::getLastError() const {
    return lastError;
}

const char* BME280SensorInterface::getLastErrorMessage() const {
    return lastErrorMessage;
}

void BME280SensorInterface::clearError() {
    lastError = ThermostatStatus::OK;
    memset(lastErrorMessage, 0, sizeof(lastErrorMessage));
} 