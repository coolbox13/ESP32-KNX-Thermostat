#include "pid_controller.h"

PIDController::PIDController() :
  thermostatState(nullptr),
  kp(1.0),
  ki(0.1),
  kd(0.01),
  lastUpdateTime(0),
  updateInterval(30000),  // Default 30 seconds
  lastError(0.0),
  integral(0.0),
  lastOutput(0.0),
  proportionalTerm(0.0),
  integralTerm(0.0),
  derivativeTerm(0.0) {
}

void PIDController::begin(ThermostatState* state, float kp, float ki, float kd) {
  thermostatState = state;
  setTunings(kp, ki, kd);
  reset();
}

void PIDController::setTunings(float kp, float ki, float kd) {
  // Guard against invalid values
  if (kp < 0.0) kp = 0.0;
  if (ki < 0.0) ki = 0.0;
  if (kd < 0.0) kd = 0.0;
  
  this->kp = kp;
  this->ki = ki;
  this->kd = kd;
}

void PIDController::setUpdateInterval(unsigned long interval) {
  // Prevent zero interval
  if (interval < 1000) interval = 1000;
  updateInterval = interval;
}

void PIDController::update() {
  if (!thermostatState) {
    return;
  }
  
  unsigned long currentTime = millis();
  
  // Check if it's time to update
  if (currentTime - lastUpdateTime >= updateInterval) {
    float output = calculate();
    thermostatState->setValvePosition(output);
    lastOutput = output;
    lastUpdateTime = currentTime;
  }
}

void PIDController::reset() {
  integral = 0.0;
  lastError = 0.0;
  lastOutput = 0.0;
  lastUpdateTime = 0; // Force immediate update
}

float PIDController::getKp() const {
  return kp;
}

float PIDController::getKi() const {
  return ki;
}

float PIDController::getKd() const {
  return kd;
}

float PIDController::getLastError() const {
  return lastError;
}

float PIDController::getProportionalTerm() const {
  return proportionalTerm;
}

float PIDController::getIntegralTerm() const {
  return integralTerm;
}

float PIDController::getDerivativeTerm() const {
  return derivativeTerm;
}

float PIDController::getLastOutput() const {
  return lastOutput;
}

float PIDController::calculate() {
  if (!thermostatState) {
    return 0.0;
  }
  
  // Get current temperature and setpoint
  float currentTemp = thermostatState->currentTemperature;
  float targetTemp = thermostatState->targetTemperature;
  
  // Calculate error
  float error = targetTemp - currentTemp;
  
  // Calculate proportional term
  proportionalTerm = kp * error;
  
  // Calculate integral term
  integral += ki * error;
  limitIntegral();
  integralTerm = integral;
  
  // Calculate derivative term
  derivativeTerm = kd * (error - lastError);
  
  // Calculate output
  float output = proportionalTerm + integralTerm + derivativeTerm;
  
  // Limit output to 0-100%
  if (output > 100.0) output = 100.0;
  if (output < 0.0) output = 0.0;
  
  // Debug output
  Serial.println("PID Calculation:");
  Serial.print("Setpoint: ");
  Serial.print(targetTemp);
  Serial.println(" °C");
  
  Serial.print("Current Temperature: ");
  Serial.print(currentTemp);
  Serial.println(" °C");
  
  Serial.print("Error: ");
  Serial.print(error);
  Serial.println(" °C");
  
  Serial.print("P-Term: ");
  Serial.println(proportionalTerm);
  
  Serial.print("I-Term: ");
  Serial.println(integralTerm);
  
  Serial.print("D-Term: ");
  Serial.println(derivativeTerm);
  
  Serial.print("Output: ");
  Serial.print(output);
  Serial.println(" %");
  
  // Save last error for next iteration
  lastError = error;
  
  return output;
}

void PIDController::limitIntegral() {
  // Anti-windup protection
  if (integral > 100.0) integral = 100.0;
  if (integral < 0.0) integral = 0.0;
}