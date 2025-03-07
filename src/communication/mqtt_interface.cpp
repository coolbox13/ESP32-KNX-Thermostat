#include "mqtt_interface.h"

// Static callback function
void MQTTInterface::mqttCallback(char* topic, byte* payload, unsigned int length) {
  // This static method can't access instance variables directly
  // The PubSubClient library doesn't provide a way to pass user data to callbacks
  // This would need to be handled with a global instance or another mechanism
  
  // For now, just print the received message
  Serial.print("MQTT message received [");
  Serial.print(topic);
  Serial.print("]: ");
  
  // Convert payload to string
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  
  // In a real implementation, we'd need to pass this to the appropriate instance
  // This is a limitation of the PubSubClient library
}

MQTTInterface::MQTTInterface() : 
  mqttClient(wifiClient),
  port(1883),
  thermostatState(nullptr),
  lastConnectAttempt(0) {
  
  // Initialize strings
  server[0] = '\0';
  user[0] = '\0';
  password[0] = '\0';
  strlcpy(clientId, "KNXThermostat", sizeof(clientId));
}

bool MQTTInterface::begin(ThermostatState* state, 
                         const char* server, 
                         int port, 
                         const char* user, 
                         const char* password,
                         const char* clientId) {
  thermostatState = state;
  
  // Set server and credentials
  setServer(server, port);
  setCredentials(user, password);
  setClientId(clientId);
  
  // Set callback
  mqttClient.setCallback(mqttCallback);
  
  // Register callbacks if thermostat state is available
  if (thermostatState) {
    registerCallbacks();
  }
  
  // Try to connect
  return connect();
}

void MQTTInterface::setServer(const char* server, int port) {
  strlcpy(this->server, server, sizeof(this->server));
  this->port = port;
  mqttClient.setServer(server, port);
}

void MQTTInterface::setCredentials(const char* user, const char* password) {
  strlcpy(this->user, user, sizeof(this->user));
  strlcpy(this->password, password, sizeof(this->password));
}

void MQTTInterface::setClientId(const char* clientId) {
  strlcpy(this->clientId, clientId, sizeof(this->clientId));
}

void MQTTInterface::loop() {
  // Check if MQTT is connected
  if (!mqttClient.connected()) {
    unsigned long currentTime = millis();
    
    // Try to reconnect periodically
    if (currentTime - lastConnectAttempt >= CONNECT_RETRY_INTERVAL) {
      lastConnectAttempt = currentTime;
      connect();
    }
  }
  
  // Process MQTT messages
  mqttClient.loop();
}

bool MQTTInterface::connect() {
  if (strlen(server) == 0) {
    Serial.println("MQTT server not configured");
    return false;
  }
  
  Serial.print("Connecting to MQTT server ");
  Serial.print(server);
  Serial.print(":");
  Serial.print(port);
  Serial.print(" as ");
  Serial.print(clientId);
  Serial.println("...");
  
  bool connected = false;
  
  // Connect with or without credentials
  if (strlen(user) > 0) {
    connected = mqttClient.connect(clientId, user, password);
  } else {
    connected = mqttClient.connect(clientId);
  }
  
  if (connected) {
    Serial.println("Connected to MQTT server");
    
    // Subscribe to topics
    mqttClient.subscribe("esp_thermostat/setpoint");
    mqttClient.subscribe("esp_thermostat/settings/#");
    
    // Publish initial values
    if (thermostatState) {
      publishTemperature(thermostatState->currentTemperature);
      publishHumidity(thermostatState->currentHumidity);
      publishPressure(thermostatState->currentPressure);
      publishSetpoint(thermostatState->targetTemperature);
      publishValvePosition(thermostatState->valvePosition);
      publishMode(thermostatState->operatingMode);
    }
    
    return true;
  } else {
    Serial.print("Failed to connect to MQTT server, rc=");
    Serial.println(mqttClient.state());
    return false;
  }
}

void MQTTInterface::disconnect() {
  mqttClient.disconnect();
}

bool MQTTInterface::isConnected() const {
  return mqttClient.connected();
}

void MQTTInterface::publishTemperature(float temperature) {
  if (!isConnected()) return;
  
  char tempStr[8];
  dtostrf(temperature, 1, 2, tempStr);
  mqttClient.publish(TOPIC_TEMPERATURE, tempStr);
}

void MQTTInterface::publishHumidity(float humidity) {
  if (!isConnected()) return;
  
  char humStr[8];
  dtostrf(humidity, 1, 2, humStr);
  mqttClient.publish(TOPIC_HUMIDITY, humStr);
}

void MQTTInterface::publishPressure(float pressure) {
  if (!isConnected()) return;
  
  char pressStr[8];
  dtostrf(pressure, 1, 2, pressStr);
  mqttClient.publish(TOPIC_PRESSURE, pressStr);
}

void MQTTInterface::publishSetpoint(float setpoint) {
  if (!isConnected()) return;
  
  char setpointStr[8];
  dtostrf(setpoint, 1, 2, setpointStr);
  mqttClient.publish(TOPIC_SETPOINT, setpointStr);
}

void MQTTInterface::publishValvePosition(float position) {
  if (!isConnected()) return;
  
  char valveStr[8];
  dtostrf(position, 1, 2, valveStr);
  mqttClient.publish(TOPIC_VALVE_POSITION, valveStr);
}

void MQTTInterface::publishMode(ThermostatMode mode) {
  if (!isConnected()) return;
  
  char modeStr[2];
  itoa(static_cast<int>(mode), modeStr, 10);
  mqttClient.publish(TOPIC_MODE, modeStr);
}

void MQTTInterface::handleCallback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to string
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  String topicStr = String(topic);
  Serial.println("MQTT message received [" + topicStr + "]: " + message);
  
  if (topicStr == TOPIC_SETPOINT) {
    float setpoint = message.toFloat();
    if (setpoint > 0 && thermostatState) {
      thermostatState->setTargetTemperature(setpoint);
    }
  } 
  else if (topicStr == String(TOPIC_SETTINGS_BASE) + "proportional") {
    float kp = message.toFloat();
    // This would need to update the PID controller through a callback
  }
  else if (topicStr == String(TOPIC_SETTINGS_BASE) + "integral") {
    float ki = message.toFloat();
    // This would need to update the PID controller through a callback
  }
  else if (topicStr == String(TOPIC_SETTINGS_BASE) + "derivative") {
    float kd = message.toFloat();
    // This would need to update the PID controller through a callback
  }
  else if (topicStr == String(TOPIC_SETTINGS_BASE) + "mode") {
    int mode = message.toInt();
    if (thermostatState) {
      thermostatState->setMode(static_cast<ThermostatMode>(mode));
    }
  }
}

void MQTTInterface::registerCallbacks() {
  if (!thermostatState) return;
  
  // Register callbacks to publish MQTT messages when thermostat state changes
  thermostatState->onTemperatureChanged = [this](float temp) {
    this->publishTemperature(temp);
  };
  
  thermostatState->onHumidityChanged = [this](float humidity) {
    this->publishHumidity(humidity);
  };
  
  thermostatState->onSetpointChanged = [this](float setpoint) {
    this->publishSetpoint(setpoint);
  };
  
  thermostatState->onValvePositionChanged = [this](float position) {
    this->publishValvePosition(position);
  };
  
  thermostatState->onModeChanged = [this](ThermostatMode mode) {
    this->publishMode(mode);
  };
}