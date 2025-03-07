#include "config_manager.h"

ConfigManager::ConfigManager() {
  // Initialize with default values
  setDefaults();
}

bool ConfigManager::begin() {
  // Initialize file system
  if (!FileFS.begin()) {
    Serial.println("Failed to mount file system");
    return false;
  }
  
  // Try to load configuration
  if (!loadConfig()) {
    Serial.println("Using default configuration");
    saveConfig(); // Save default configuration
  }
  
  return true;
}

bool ConfigManager::setupWiFi() {
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180); // Auto-exit config portal after 3 minutes
  
  // Try to connect to WiFi
  if (!wifiManager.autoConnect(deviceName)) {
    Serial.println("Failed to connect to WiFi - restarting");
    delay(1000);
    ESP.restart();
    return false;
  }
  
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  return true;
}

bool ConfigManager::saveConfig() {
  Serial.println("Saving configuration...");
  DynamicJsonDocument doc(1024);
  
  // Device settings
  doc["deviceName"] = deviceName;
  doc["sendInterval"] = sendInterval;
  doc["pidInterval"] = pidInterval;
  
  // KNX settings
  doc["knxPhysicalArea"] = knxPhysicalArea;
  doc["knxPhysicalLine"] = knxPhysicalLine;
  doc["knxPhysicalMember"] = knxPhysicalMember;
  
  doc["knxTempArea"] = knxTempArea;
  doc["knxTempLine"] = knxTempLine;
  doc["knxTempMember"] = knxTempMember;
  
  doc["knxSetpointArea"] = knxSetpointArea;
  doc["knxSetpointLine"] = knxSetpointLine;
  doc["knxSetpointMember"] = knxSetpointMember;
  
  doc["knxValveArea"] = knxValveArea;
  doc["knxValveLine"] = knxValveLine;
  doc["knxValveMember"] = knxValveMember;
  
  // MQTT settings
  doc["mqttServer"] = mqttServer;
  doc["mqttPort"] = mqttPort;
  doc["mqttUser"] = mqttUser;
  doc["mqttPassword"] = mqttPassword;
  doc["mqttClientId"] = mqttClientId;
  
  // PID settings
  doc["kp"] = kp;
  doc["ki"] = ki;
  doc["kd"] = kd;
  doc["setpoint"] = setpoint;
  
  File configFile = FileFS.open(CONFIG_FILE, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }
  
  serializeJson(doc, configFile);
  configFile.close();
  Serial.println("Configuration saved");
  return true;
}

bool ConfigManager::loadConfig() {
  Serial.println("Loading configuration...");
  if (!FileFS.exists(CONFIG_FILE)) {
    Serial.println("Config file not found");
    return false;
  }
  
  File configFile = FileFS.open(CONFIG_FILE, "r");
  if (!configFile) {
    Serial.println("Failed to open config file for reading");
    return false;
  }
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    Serial.println("Failed to parse config file");
    return false;
  }
  
  // Device settings
  strlcpy(deviceName, doc["deviceName"] | DEFAULT_DEVICE_NAME, sizeof(deviceName));
  sendInterval = doc["sendInterval"] | 10000;
  pidInterval = doc["pidInterval"] | 30000;
  
  // KNX settings
  knxPhysicalArea = doc["knxPhysicalArea"] | 1;
  knxPhysicalLine = doc["knxPhysicalLine"] | 1;
  knxPhysicalMember = doc["knxPhysicalMember"] | 201;
  
  knxTempArea = doc["knxTempArea"] | 3;
  knxTempLine = doc["knxTempLine"] | 1;
  knxTempMember = doc["knxTempMember"] | 0;
  
  knxSetpointArea = doc["knxSetpointArea"] | 3;
  knxSetpointLine = doc["knxSetpointLine"] | 2;
  knxSetpointMember = doc["knxSetpointMember"] | 0;
  
  knxValveArea = doc["knxValveArea"] | 3;
  knxValveLine = doc["knxValveLine"] | 3;
  knxValveMember = doc["knxValveMember"] | 0;
  
  // MQTT settings
  strlcpy(mqttServer, doc["mqttServer"] | "192.168.178.32", sizeof(mqttServer));
  mqttPort = doc["mqttPort"] | 1883;
  strlcpy(mqttUser, doc["mqttUser"] | "", sizeof(mqttUser));
  strlcpy(mqttPassword, doc["mqttPassword"] | "", sizeof(mqttPassword));
  strlcpy(mqttClientId, doc["mqttClientId"] | "ESP32Thermostat", sizeof(mqttClientId));
  
  // PID settings
  kp = doc["kp"] | 1.0;
  ki = doc["ki"] | 0.1;
  kd = doc["kd"] | 0.01;
  setpoint = doc["setpoint"] | 21.0;
  
  Serial.println("Configuration loaded");
  return true;
}

void ConfigManager::setDefaults() {
  strlcpy(deviceName, DEFAULT_DEVICE_NAME, sizeof(deviceName));
  sendInterval = 10000;
  pidInterval = 30000;
  
  // KNX settings
  knxPhysicalArea = 1;
  knxPhysicalLine = 1;
  knxPhysicalMember = 201;
  
  knxTempArea = 3;
  knxTempLine = 1;
  knxTempMember = 0;
  
  knxSetpointArea = 3;
  knxSetpointLine = 2;
  knxSetpointMember = 0;
  
  knxValveArea = 3;
  knxValveLine = 3;
  knxValveMember = 0;
  
  // MQTT settings
  strlcpy(mqttServer, "192.168.178.32", sizeof(mqttServer));
  mqttPort = 1883;
  strlcpy(mqttUser, "", sizeof(mqttUser));
  strlcpy(mqttPassword, "", sizeof(mqttPassword));
  strlcpy(mqttClientId, "ESP32Thermostat", sizeof(mqttClientId));
  
  // PID settings
  kp = 1.0;
  ki = 0.1;
  kd = 0.01;
  setpoint = 21.0;
}

void ConfigManager::factoryReset() {
  // Remove config file
  FileFS.remove(CONFIG_FILE);
  
  // Reset to defaults
  setDefaults();
  
  // Save default configuration
  saveConfig();
}

// Setters
void ConfigManager::setDeviceName(const char* name) {
  strlcpy(deviceName, name, sizeof(deviceName));
}

void ConfigManager::setSendInterval(int interval) {
  sendInterval = interval;
}

void ConfigManager::setPidInterval(int interval) {
  pidInterval = interval;
}

void ConfigManager::setKnxPhysicalAddress(int area, int line, int member) {
  knxPhysicalArea = area;
  knxPhysicalLine = line;
  knxPhysicalMember = member;
}

void ConfigManager::setKnxTemperatureGA(int area, int line, int member) {
  knxTempArea = area;
  knxTempLine = line;
  knxTempMember = member;
}

void ConfigManager::setKnxSetpointGA(int area, int line, int member) {
  knxSetpointArea = area;
  knxSetpointLine = line;
  knxSetpointMember = member;
}

void ConfigManager::setKnxValveGA(int area, int line, int member) {
  knxValveArea = area;
  knxValveLine = line;
  knxValveMember = member;
}

void ConfigManager::setMqttServer(const char* server) {
  strlcpy(mqttServer, server, sizeof(mqttServer));
}

void ConfigManager::setMqttPort(int port) {
  mqttPort = port;
}

void ConfigManager::setMqttUser(const char* user) {
  strlcpy(mqttUser, user, sizeof(mqttUser));
}

void ConfigManager::setMqttPassword(const char* password) {
  strlcpy(mqttPassword, password, sizeof(mqttPassword));
}

void ConfigManager::setMqttClientId(const char* clientId) {
  strlcpy(mqttClientId, clientId, sizeof(mqttClientId));
}

void ConfigManager::setKp(float value) {
  kp = value;
}

void ConfigManager::setKi(float value) {
  ki = value;
}

void ConfigManager::setKd(float value) {
  kd = value;
}

void ConfigManager::setSetpoint(float value) {
  setpoint = value;
}

// Getters
const char* ConfigManager::getDeviceName() const {
  return deviceName;
}

int ConfigManager::getSendInterval() const {
  return sendInterval;
}

int ConfigManager::getPidInterval() const {
  return pidInterval;
}

int ConfigManager::getKnxPhysicalArea() const {
  return knxPhysicalArea;
}

int ConfigManager::getKnxPhysicalLine() const {
  return knxPhysicalLine;
}

int ConfigManager::getKnxPhysicalMember() const {
  return knxPhysicalMember;
}

int ConfigManager::getKnxTempArea() const {
  return knxTempArea;
}

int ConfigManager::getKnxTempLine() const {
  return knxTempLine;
}

int ConfigManager::getKnxTempMember() const {
  return knxTempMember;
}

int ConfigManager::getKnxSetpointArea() const {
  return knxSetpointArea;
}

int ConfigManager::getKnxSetpointLine() const {
  return knxSetpointLine;
}

int ConfigManager::getKnxSetpointMember() const {
  return knxSetpointMember;
}

int ConfigManager::getKnxValveArea() const {
  return knxValveArea;
}

int ConfigManager::getKnxValveLine() const {
  return knxValveLine;
}

int ConfigManager::getKnxValveMember() const {
  return knxValveMember;
}

const char* ConfigManager::getMqttServer() const {
  return mqttServer;
}

int ConfigManager::getMqttPort() const {
  return mqttPort;
}

const char* ConfigManager::getMqttUser() const {
  return mqttUser;
}

const char* ConfigManager::getMqttPassword() const {
  return mqttPassword;
}

const char* ConfigManager::getMqttClientId() const {
  return mqttClientId;
}

float ConfigManager::getKp() const {
  return kp;
}

float ConfigManager::getKi() const {
  return ki;
}

float ConfigManager::getKd() const {
  return kd;
}

float ConfigManager::getSetpoint() const {
  return setpoint;
}