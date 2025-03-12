#include "thermostat_controller.h"
#include "thermostat_state.h"
#include "sensor_interface.h"
#include "control_interface.h"
#include <esp_log.h>

static const char* TAG = "ThermostatController";

ThermostatController::ThermostatController(PIDController* pid, SensorInterface* sensor)
    : pidController(pid)
    , sensorInterface(sensor)
    , lastError(ThermostatStatus::OK) {
    memset(lastErrorMessage, 0, sizeof(lastErrorMessage));
}

void ThermostatController::begin() {
    if (!sensorInterface->begin()) {
        snprintf(lastErrorMessage, sizeof(lastErrorMessage), "Failed to initialize sensor");
        lastError = ThermostatStatus::ERROR_SENSOR;
        return;
    }
    
    if (!pidController->begin()) {
        snprintf(lastErrorMessage, sizeof(lastErrorMessage), "Failed to initialize PID controller");
        lastError = ThermostatStatus::ERROR_CONTROL;
        return;
    }
    
    lastError = ThermostatStatus::OK;
}

void ThermostatController::update() {
    // Update sensor readings
    sensorInterface->updateReadings();
    
    // Get current temperature
    float currentTemp = sensorInterface->getTemperature();
    
    // Update PID controller
    pidController->update(currentTemp);
    
    // Update valve position based on PID output
    float valvePosition = pidController->getOutput();
    // Ensure valve position is within bounds
    valvePosition = constrain(valvePosition, 0.0f, 100.0f);
    
    // Update status
    if (sensorInterface->getLastError() != ThermostatStatus::OK) {
        lastError = sensorInterface->getLastError();
        strncpy(lastErrorMessage, sensorInterface->getLastErrorMessage(), sizeof(lastErrorMessage) - 1);
        lastErrorMessage[sizeof(lastErrorMessage) - 1] = '\0';
    } else if (pidController->getLastError() != ThermostatStatus::OK) {
        lastError = pidController->getLastError();
        strncpy(lastErrorMessage, pidController->getLastErrorMessage(), sizeof(lastErrorMessage) - 1);
        lastErrorMessage[sizeof(lastErrorMessage) - 1] = '\0';
    } else {
        lastError = ThermostatStatus::OK;
        memset(lastErrorMessage, 0, sizeof(lastErrorMessage));
    }
}

// Error handling methods are defined in the header file 