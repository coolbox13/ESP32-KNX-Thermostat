#pragma once

#include "control_interface.h"
#include "thermostat_types.h"
#include "thermostat_state.h"

// PID configuration structure
struct PIDConfig {
    float kp;           // Proportional gain
    float ki;           // Integral gain
    float kd;           // Derivative gain
    float minOutput;    // Minimum output value
    float maxOutput;    // Maximum output value
    float sampleTime;   // Sample time in milliseconds
};

class PIDController : public ControlInterface {
public:
    PIDController(ThermostatState* state);
    
    // ControlInterface methods
    bool begin() override;
    void loop() override;
    void configure(const void* config) override;
    void setSetpoint(float sp) override;
    void setInput(float in) override;
    float getOutput() const override;
    void setActive(bool state) override;
    bool isActive() const override;
    ThermostatStatus getLastError() const override;
    const char* getLastErrorMessage() const override;
    void clearError() override;
    void setUpdateInterval(unsigned long interval) override;
    void reset() override;
    bool saveConfig() override;

    // PID specific methods
    void setOutputLimits(float min, float max);
    void setDirection(bool reverse);
    float getKp() const { return config.kp; }
    float getKi() const { return config.ki; }
    float getKd() const { return config.kd; }
    float getMinOutput() const { return config.minOutput; }
    float getMaxOutput() const { return config.maxOutput; }
    float getSampleTime() const { return config.sampleTime; }
    
    // Update method that combines setInput and loop
    void update(float newInput);

protected:
    // Helper methods
    float computePID();
    void resetIntegral();
    float clamp(float value, float min, float max) const;

private:
    PIDConfig config;
    float setpoint;
    float input;
    float output;
    float integral;
    float lastInput;
    unsigned long lastTime;
    bool active;
    ThermostatStatus lastError;
    char lastErrorMessage[128];
    ThermostatState* thermostatState;  // Added thermostat state pointer
};