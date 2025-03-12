#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "esp_log.h"

#include "thermostat_state.h"
#include "pid_controller.h"

static const char* TAG = "PIDController";

PIDController::PIDController(ThermostatState* state)
    : setpoint(21.0f)  // Default room temperature
    , input(0.0f)
    , output(0.0f)
    , integral(0.0f)
    , lastInput(0.0f)
    , lastTime(0)
    , active(false)
    , lastError(ThermostatStatus::OK)
    , thermostatState(state) {
    
    // Default PID configuration
    config.kp = 2.0f;
    config.ki = 0.5f;
    config.kd = 1.0f;
    config.minOutput = 0.0f;
    config.maxOutput = 100.0f;
    config.sampleTime = 30000.0f;  // 30 seconds default
    
    memset(lastErrorMessage, 0, sizeof(lastErrorMessage));
}

bool PIDController::begin() {
    ESP_LOGI(TAG, "Initializing PID controller");
    reset();
    return true;
}

void PIDController::loop() {
    if (!active || !thermostatState->isEnabled()) {
        output = 0;
        return;
    }
    
    unsigned long now = millis();
    if (now - lastTime >= static_cast<unsigned long>(config.sampleTime)) {
        output = computePID();
        lastTime = now;
    }
}

void PIDController::configure(const void* configData) {
    if (configData) {
        const PIDConfig* newConfig = static_cast<const PIDConfig*>(configData);
        config = *newConfig;
    }
}

void PIDController::setSetpoint(float sp) {
    setpoint = sp;
}

void PIDController::setInput(float in) {
    input = in;
}

float PIDController::getOutput() const {
    return output;
}

void PIDController::setActive(bool state) {
    if (active != state) {
        active = state;
        if (active) {
            // Reset integral when activating
            resetIntegral();
        }
    }
}

bool PIDController::isActive() const {
    return active;
}

ThermostatStatus PIDController::getLastError() const {
    return lastError;
}

const char* PIDController::getLastErrorMessage() const {
    return lastErrorMessage;
}

void PIDController::clearError() {
    lastError = ThermostatStatus::OK;
    memset(lastErrorMessage, 0, sizeof(lastErrorMessage));
}

void PIDController::setUpdateInterval(unsigned long interval) {
    config.sampleTime = static_cast<float>(interval);
}

void PIDController::reset() {
    resetIntegral();
}

bool PIDController::saveConfig() {
    // This is a stub implementation
    return true;
}

float PIDController::computePID() {
    float error = setpoint - input;
    float dInput = input - lastInput;
    
    integral += (config.ki * error);
    integral = clamp(integral, config.minOutput, config.maxOutput);
    
    float P = config.kp * error;
    float I = integral;
    float D = -config.kd * dInput;  // Negative because dInput is backwards
    
    float result = P + I + D;
    result = clamp(result, config.minOutput, config.maxOutput);
    
    lastInput = input;
    return result;
}

void PIDController::resetIntegral() {
    integral = 0.0f;
    lastInput = input;
}

float PIDController::clamp(float value, float min, float max) const {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void PIDController::update(float currentTemperature) {
    if (!thermostatState->isEnabled()) {
        output = 0;  // Turn off control when disabled
        return;
    }
    
    setInput(currentTemperature);
    loop();
}

