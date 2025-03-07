#include "sensor_interface.h"

SensorInterface::SensorInterface() : 
  thermostatState(nullptr),
  lastUpdateTime(0),
  updateInterval(10000),
  temperature(0.0),
  humidity(0.0),
  pressure(0.0),
  sensorAvailable(false) {
}

bool SensorInterface::begin(ThermostatState* state) {
  thermostatState = state;
  
  Serial.println("Initializing BME280 sensor...");
  
  // Try to initialize BME280 sensor
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    sensorAvailable = false;
    return false;
  }
  
  bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                 Adafruit_BME280::SAMPLING_X1, // temperature
                 Adafruit_BME280::SAMPLING_X1, // pressure
                 Adafruit_BME280::SAMPLING_X1, // humidity
                 Adafruit_BME280::FILTER_OFF   );
  
  Serial.println("BME280 sensor initialized successfully");
  sensorAvailable = true;
  
  // Take initial reading
  forceUpdate();
  
  return true;
}

void SensorInterface::update() {
  unsigned long currentTime = millis();
  
  // Check if it's time to update readings
  if (currentTime - lastUpdateTime >= updateInterval) {
    readSensor();
    lastUpdateTime = currentTime;
  }
}

void SensorInterface::forceUpdate() {
  readSensor();
  lastUpdateTime = millis();
}

float SensorInterface::getTemperature() const {
  return temperature;
}

float SensorInterface::getHumidity() const {
  return humidity;
}

float SensorInterface::getPressure() const {
  return pressure;
}

void SensorInterface::setUpdateInterval(unsigned long interval) {
  updateInterval = interval;
}

bool SensorInterface::isAvailable() const {
  return sensorAvailable;
}

void SensorInterface::readSensor() {
  if (!sensorAvailable) {
    Serial.println("Sensor not available");
    return;
  }
  
  // Read sensor values
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F; // Convert to hPa
  
  Serial.println("Sensor readings:");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  
  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.println(" hPa");
  
  // Update thermostat state if available
  if (thermostatState) {
    thermostatState->setTemperature(temperature);
    thermostatState->setHumidity(humidity);
    thermostatState->setPressure(pressure);
  }
}