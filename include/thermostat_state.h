#ifndef THERMOSTAT_STATE_H
#define THERMOSTAT_STATE_H

#include <Arduino.h>
#include <functional>
#include "thermostat_types.h"

class ThermostatState {
public:
  // Constructor with default values
  ThermostatState();
  
  // Getters
  float getCurrentTemperature() const { return currentTemperature; }
  float getCurrentHumidity() const { return currentHumidity; }
  float getCurrentPressure() const { return currentPressure; }
  float getTargetTemperature() const { return targetTemperature; }
  float getValvePosition() const { return valvePosition; }
  ThermostatMode getMode() const { return operatingMode; }
  bool isHeating() const { return heatingActive; }
  ThermostatStatus getStatus() const { return status; }
  bool isEnabled() const { return enabled; }
  
  // Setters
  void setTemperature(float value);
  void setHumidity(float value);
  void setPressure(float value);
  void setTargetTemperature(float value);
  void setValvePosition(float value);
  void setMode(ThermostatMode mode);
  void setHeating(bool active) { heatingActive = active; }
  void setStatus(ThermostatStatus newStatus) { status = newStatus; }
  void setEnabled(bool state);
  
  // Alias methods for clarity
  void setCurrentTemperature(float value) { setTemperature(value); }
  void setCurrentHumidity(float value) { setHumidity(value); }
  void setCurrentPressure(float value) { setPressure(value); }
  
  // Callback function types
  using TemperatureCallback = std::function<void(float)>;
  using HumidityCallback = std::function<void(float)>;
  using PressureCallback = std::function<void(float)>;
  using TargetTemperatureCallback = std::function<void(float)>;
  using ValvePositionCallback = std::function<void(float)>;
  using ModeCallback = std::function<void(ThermostatMode)>;
  using HeatingCallback = std::function<void(bool)>;
  using StatusCallback = std::function<void(ThermostatStatus)>;
  
  // Register callbacks
  void onTemperatureChange(TemperatureCallback cb) { temperatureCallback = cb; }
  void onHumidityChange(HumidityCallback cb) { humidityCallback = cb; }
  void onPressureChange(PressureCallback cb) { pressureCallback = cb; }
  void onTargetTemperatureChange(TargetTemperatureCallback cb) { targetTemperatureCallback = cb; }
  void onValvePositionChange(ValvePositionCallback cb) { valvePositionCallback = cb; }
  void onModeChange(ModeCallback cb) { modeCallback = cb; }
  void onHeatingChange(HeatingCallback cb) { heatingCallback = cb; }
  void onStatusChange(StatusCallback cb) { statusCallback = cb; }

private:
  // Current state
  float currentTemperature;
  float currentHumidity;
  float currentPressure;
  float targetTemperature;
  float valvePosition;
  ThermostatMode operatingMode;
  bool heatingActive;
  ThermostatStatus status;
  bool enabled;  // New member for ON/OFF state
  
  // Validation helpers
  bool isValidTemperature(float value) const;
  bool isValidHumidity(float value) const;
  bool isValidPressure(float value) const;
  bool isValidValvePosition(float value) const;
  
  // Callbacks
  TemperatureCallback temperatureCallback;
  HumidityCallback humidityCallback;
  PressureCallback pressureCallback;
  TargetTemperatureCallback targetTemperatureCallback;
  ValvePositionCallback valvePositionCallback;
  ModeCallback modeCallback;
  HeatingCallback heatingCallback;
  StatusCallback statusCallback;
};

#endif // THERMOSTAT_STATE_H