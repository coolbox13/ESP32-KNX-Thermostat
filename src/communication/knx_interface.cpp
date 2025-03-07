#include "knx_interface.h"

KNXInterface::KNXInterface() : thermostatState(nullptr) {
  // Initialize group addresses to default values
  temperatureGA = calculateGA(3, 1, 0);
  setpointGA = calculateGA(3, 2, 0);
  valvePositionGA = calculateGA(3, 3, 0);
  modeGA = calculateGA(3, 4, 0);
}

bool KNXInterface::begin(int area, int line, int member) {
  // Start KNX communication
  if (!knx.start(nullptr)) {
    Serial.println("Failed to start KNX communication");
    return false;
  }
  
  // Set physical address
  address_t physicalAddr;
  physicalAddr.pa.area = area;
  physicalAddr.pa.line = line;
  physicalAddr.pa.member = member;
  
  if (!knx.physical_address_set(physicalAddr)) {
    Serial.println("Failed to set KNX physical address");
    return false;
  }
  
  Serial.printf("KNX physical address set to %d.%d.%d\n", area, line, member);
  
  // Register callbacks
  registerKNXCallbacks();
  
  return true;
}

void KNXInterface::registerCallbacks(ThermostatState* state) {
  thermostatState = state;
  
  // Register callbacks only if thermostat state is available
  if (thermostatState) {
    // When temperature changes, send to KNX
    thermostatState->onTemperatureChanged = [this](float temp) {
      this->sendTemperature(temp);
    };
    
    // When setpoint changes, send to KNX
    thermostatState->onSetpointChanged = [this](float setpoint) {
      this->sendSetpoint(setpoint);
    };
    
    // When valve position changes, send to KNX
    thermostatState->onValvePositionChanged = [this](float position) {
      this->sendValvePosition(position);
    };
    
    // When operating mode changes, send to KNX
    thermostatState->onModeChanged = [this](ThermostatMode mode) {
      this->sendMode(mode);
    };
  }
}

void KNXInterface::loop() {
  // Process KNX communications
  knx.loop();
}

void KNXInterface::setTemperatureGA(int area, int line, int member) {
  temperatureGA = calculateGA(area, line, member);
  Serial.printf("KNX Temperature GA set to %d/%d/%d\n", area, line, member);
}

void KNXInterface::setSetpointGA(int area, int line, int member) {
  setpointGA = calculateGA(area, line, member);
  Serial.printf("KNX Setpoint GA set to %d/%d/%d\n", area, line, member);
  registerKNXCallbacks(); // Re-register callbacks for new addresses
}

void KNXInterface::setValvePositionGA(int area, int line, int member) {
  valvePositionGA = calculateGA(area, line, member);
  Serial.printf("KNX Valve Position GA set to %d/%d/%d\n", area, line, member);
}

void KNXInterface::setModeGA(int area, int line, int member) {
  modeGA = calculateGA(area, line, member);
  Serial.printf("KNX Mode GA set to %d/%d/%d\n", area, line, member);
  registerKNXCallbacks(); // Re-register callbacks for new addresses
}

bool KNXInterface::sendTemperature(float temperature) {
  // DPT 9.001 - Temperature (2-byte float)
  if (knx.write_2byte_float(temperatureGA, temperature)) {
    Serial.printf("Temperature sent to KNX: %.2f°C\n", temperature);
    return true;
  } else {
    Serial.println("Failed to send temperature to KNX");
    return false;
  }
}

bool KNXInterface::sendSetpoint(float setpoint) {
  // DPT 9.001 - Temperature (2-byte float)
  if (knx.write_2byte_float(setpointGA, setpoint)) {
    Serial.printf("Setpoint sent to KNX: %.2f°C\n", setpoint);
    return true;
  } else {
    Serial.println("Failed to send setpoint to KNX");
    return false;
  }
}

bool KNXInterface::sendValvePosition(float position) {
  // Ensure position is within 0-100 range
  uint8_t scaledPosition = constrain(position, 0, 100);
  
  // DPT 5.001 - Percentage (1-byte unsigned)
  if (knx.write_1byte_uint(valvePositionGA, scaledPosition)) {
    Serial.printf("Valve position sent to KNX: %.1f%%\n", position);
    return true;
  } else {
    Serial.println("Failed to send valve position to KNX");
    return false;
  }
}

bool KNXInterface::sendMode(ThermostatMode mode) {
  // DPT 5.xxx - Unsigned Value (1-byte unsigned)
  uint8_t modeValue = static_cast<uint8_t>(mode);
  
  if (knx.write_1byte_uint(modeGA, modeValue)) {
    Serial.printf("Mode sent to KNX: %d\n", modeValue);
    return true;
  } else {
    Serial.println("Failed to send mode to KNX");
    return false;
  }
}

address_t KNXInterface::calculateGA(int area, int line, int member) {
  address_t tmp;
  tmp.ga.area = area;
  tmp.ga.line = line;
  tmp.ga.member = member;
  return tmp;
}

void KNXInterface::handleSetpointCallback(message_t const &msg, void *arg) {
  KNXInterface* instance = static_cast<KNXInterface*>(arg);
  
  if (instance && instance->thermostatState && msg.data_len >= 2) {
    // KNX DPT 9.001 is 2-byte float
    float newSetpoint = knx.dpt2_to_float(msg.data[0], msg.data[1]);
    Serial.printf("Received setpoint from KNX: %.2f°C\n", newSetpoint);
    
    instance->thermostatState->setTargetTemperature(newSetpoint);
  }
}

void KNXInterface::handleModeCallback(message_t const &msg, void *arg) {
  KNXInterface* instance = static_cast<KNXInterface*>(arg);
  
  if (instance && instance->thermostatState && msg.data_len >= 1) {
    uint8_t modeValue = msg.data[0];
    ThermostatMode newMode = static_cast<ThermostatMode>(modeValue);
    
    Serial.printf("Received mode from KNX: %d\n", modeValue);
    
    instance->thermostatState->setMode(newMode);
  }
}

void KNXInterface::registerKNXCallbacks() {
  // Note: The esp-knx-ip library doesn't have a direct method to unregister callbacks
  // When registering with the same name and group address, it should overwrite previous callbacks
  
  // Register for setpoint changes
  knx.callback_register("SetpointReceived", setpointGA, handleSetpointCallback, this);
  
  // Register for mode changes
  knx.callback_register("ModeReceived", modeGA, handleModeCallback, this);
}