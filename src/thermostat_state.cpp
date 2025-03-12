#include "thermostat_state.h"

ThermostatState::ThermostatState() :
  currentTemperature(0.0f),
  currentHumidity(0.0f),
  currentPressure(0.0f),
  targetTemperature(ThermostatLimits::DEFAULT_TEMPERATURE),
  valvePosition(0.0f),
  operatingMode(ThermostatMode::OFF),
  heatingActive(false),
  status(ThermostatStatus::OK),
  temperatureCallback(nullptr),
  humidityCallback(nullptr),
  pressureCallback(nullptr),
  targetTemperatureCallback(nullptr),
  valvePositionCallback(nullptr),
  modeCallback(nullptr),
  heatingCallback(nullptr),
  statusCallback(nullptr),
  enabled(false) {
}

void ThermostatState::setTemperature(float value) {
  if (value != currentTemperature) {
    currentTemperature = value;
    if (temperatureCallback) {
      temperatureCallback(value);
    }
  }
}

void ThermostatState::setHumidity(float value) {
  if (value != currentHumidity) {
    currentHumidity = value;
    if (humidityCallback) {
      humidityCallback(value);
    }
  }
}

void ThermostatState::setPressure(float value) {
  if (value != currentPressure) {
    currentPressure = value;
    if (pressureCallback) {
      pressureCallback(value);
    }
  }
}

void ThermostatState::setTargetTemperature(float value) {
  if (!isValidTemperature(value)) {
    return;
  }
  
  if (value != targetTemperature) {
    targetTemperature = value;
    if (targetTemperatureCallback) {
      targetTemperatureCallback(value);
    }
  }
}

void ThermostatState::setValvePosition(float value) {
  if (!isValidValvePosition(value)) {
    return;
  }
  
  if (value != valvePosition) {
    valvePosition = value;
    if (valvePositionCallback) {
      valvePositionCallback(value);
    }
  }
}

void ThermostatState::setMode(ThermostatMode mode) {
  if (mode != operatingMode) {
    operatingMode = mode;
    if (modeCallback) {
      modeCallback(mode);
    }
  }
}

bool ThermostatState::isValidTemperature(float value) const {
  return value >= ThermostatLimits::MIN_TEMPERATURE && value <= ThermostatLimits::MAX_TEMPERATURE;
}

bool ThermostatState::isValidHumidity(float value) const {
  return value >= 0.0f && value <= 100.0f;
}

bool ThermostatState::isValidPressure(float value) const {
  return value >= 800.0f && value <= 1200.0f; // Standard atmospheric pressure range in hPa
}

bool ThermostatState::isValidValvePosition(float value) const {
  return value >= 0.0f && value <= 100.0f;
}

void ThermostatState::setEnabled(bool state) {
    if (enabled != state) {
        enabled = state;
        ESP_LOGI("ThermostatState", "Thermostat %s", state ? "enabled" : "disabled");
        // If you have state change callbacks, trigger them here
    }
} 