#include "web_interface.h"

WebInterface::WebInterface() : 
  server(80),
  thermostatState(nullptr),
  configManager(nullptr),
  sensorInterface(nullptr),
  knxInterface(nullptr),
  mqttInterface(nullptr),
  pidController(nullptr) {
}

bool WebInterface::begin(ThermostatState* state, 
                        ConfigManager* config, 
                        SensorInterface* sensor,
                        KNXInterface* knx,
                        MQTTInterface* mqtt,
                        PIDController* pid) {
  
  thermostatState = state;
  configManager = config;
  sensorInterface = sensor;
  knxInterface = knx;
  mqttInterface = mqtt;
  pidController = pid;
  
  // Set up web server routes
  server.on("/", HTTP_GET, [this](){ this->handleRoot(); });
  server.on("/save", HTTP_POST, [this](){ this->handleSave(); });
  server.on("/reboot", HTTP_GET, [this](){ this->handleReboot(); });
  server.on("/reset", HTTP_GET, [this](){ this->handleReset(); });
  server.on("/status", HTTP_GET, [this](){ this->handleGetStatus(); });
  server.onNotFound([this](){ this->handleNotFound(); });
  
  // Start the web server
  server.begin();
  
  // Set up mDNS
  setupMDNS();
  
  Serial.println("Web server started");
  return true;
}

void WebInterface::handle() {
  server.handleClient();
}

void WebInterface::handleRoot() {
  String html = generateHtml();
  server.send(200, "text/html", html);
}

void WebInterface::handleSave() {
  if (!configManager) {
    server.send(500, "text/plain", "Configuration manager not available");
    return;
  }
  
  // Device settings
  if (server.hasArg("deviceName")) {
    configManager->setDeviceName(server.arg("deviceName").c_str());
  }
  
  if (server.hasArg("sendInterval")) {
    configManager->setSendInterval(server.arg("sendInterval").toInt());
    if (sensorInterface) {
      sensorInterface->setUpdateInterval(server.arg("sendInterval").toInt());
    }
  }
  
  if (server.hasArg("pidInterval")) {
    configManager->setPidInterval(server.arg("pidInterval").toInt());
    if (pidController) {
      pidController->setUpdateInterval(server.arg("pidInterval").toInt());
    }
  }
  
  // KNX physical address
  if (server.hasArg("knxPhysicalArea") && server.hasArg("knxPhysicalLine") && server.hasArg("knxPhysicalMember")) {
    int area = server.arg("knxPhysicalArea").toInt();
    int line = server.arg("knxPhysicalLine").toInt();
    int member = server.arg("knxPhysicalMember").toInt();
    
    configManager->setKnxPhysicalAddress(area, line, member);
    // KNX interface would need to be reinitialized with new address
  }
  
  // KNX group addresses
  if (server.hasArg("knxTempArea") && server.hasArg("knxTempLine") && server.hasArg("knxTempMember")) {
    int area = server.arg("knxTempArea").toInt();
    int line = server.arg("knxTempLine").toInt();
    int member = server.arg("knxTempMember").toInt();
    
    configManager->setKnxTemperatureGA(area, line, member);
    if (knxInterface) {
      knxInterface->setTemperatureGA(area, line, member);
    }
  }
  
  if (server.hasArg("knxSetpointArea") && server.hasArg("knxSetpointLine") && server.hasArg("knxSetpointMember")) {
    int area = server.arg("knxSetpointArea").toInt();
    int line = server.arg("knxSetpointLine").toInt();
    int member = server.arg("knxSetpointMember").toInt();
    
    configManager->setKnxSetpointGA(area, line, member);
    if (knxInterface) {
      knxInterface->setSetpointGA(area, line, member);
    }
  }
  
  if (server.hasArg("knxValveArea") && server.hasArg("knxValveLine") && server.hasArg("knxValveMember")) {
    int area = server.arg("knxValveArea").toInt();
    int line = server.arg("knxValveLine").toInt();
    int member = server.arg("knxValveMember").toInt();
    
    configManager->setKnxValveGA(area, line, member);
    if (knxInterface) {
      knxInterface->setValvePositionGA(area, line, member);
    }
  }
  
  // MQTT settings
  if (server.hasArg("mqttServer")) {
    configManager->setMqttServer(server.arg("mqttServer").c_str());
  }
  
  if (server.hasArg("mqttPort")) {
    configManager->setMqttPort(server.arg("mqttPort").toInt());
  }
  
  if (server.hasArg("mqttUser")) {
    configManager->setMqttUser(server.arg("mqttUser").c_str());
  }
  
  if (server.hasArg("mqttPassword")) {
    configManager->setMqttPassword(server.arg("mqttPassword").c_str());
  }
  
  if (server.hasArg("mqttClientId")) {
    configManager->setMqttClientId(server.arg("mqttClientId").c_str());
  }
  
  // PID settings
  if (server.hasArg("kp")) {
    float kp = server.arg("kp").toFloat();
    configManager->setKp(kp);
    if (pidController) {
      pidController->setTunings(kp, pidController->getKi(), pidController->getKd());
    }
  }
  
  if (server.hasArg("ki")) {
    float ki = server.arg("ki").toFloat();
    configManager->setKi(ki);
    if (pidController) {
      pidController->setTunings(pidController->getKp(), ki, pidController->getKd());
    }
  }
  
  if (server.hasArg("kd")) {
    float kd = server.arg("kd").toFloat();
    configManager->setKd(kd);
    if (pidController) {
      pidController->setTunings(pidController->getKp(), pidController->getKi(), kd);
    }
  }
  
  if (server.hasArg("setpoint")) {
    float setpoint = server.arg("setpoint").toFloat();
    configManager->setSetpoint(setpoint);
    if (thermostatState) {
      thermostatState->setTargetTemperature(setpoint);
    }
  }
  
  // Save configuration
  configManager->saveConfig();
  
  // Redirect back to root
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "Saved. Redirecting...");
}

void WebInterface::handleReboot() {
  server.send(200, "text/html", "<html><body><h1>Rebooting...</h1><script>setTimeout(function(){ window.location.href = '/'; }, 5000);</script></body></html>");
  delay(500);
  ESP.restart();
}

void WebInterface::handleReset() {
  server.send(200, "text/html", "<html><body><h1>Factory Reset...</h1><script>setTimeout(function(){ window.location.href = '/'; }, 10000);</script></body></html>");
  
  // Factory reset
  if (configManager) {
    configManager->factoryReset();
  }
  
  delay(1000);
  ESP.restart();
}

void WebInterface::handleGetStatus() {
  if (!thermostatState) {
    server.send(500, "text/plain", "Thermostat state not available");
    return;
  }
  
  DynamicJsonDocument doc(512);
  
  // Device information
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();
  doc["uptime"] = millis() / 1000;
  
  // Sensor readings
  doc["temperature"] = thermostatState->currentTemperature;
  doc["humidity"] = thermostatState->currentHumidity;
  doc["pressure"] = thermostatState->currentPressure;
  
  // Control state
  doc["setpoint"] = thermostatState->targetTemperature;
  doc["valvePosition"] = thermostatState->valvePosition;
  doc["mode"] = static_cast<int>(thermostatState->operatingMode);
  doc["heatingActive"] = thermostatState->heatingActive;
  
  // PID information
  if (pidController) {
    JsonObject pid = doc.createNestedObject("pid");
    pid["kp"] = pidController->getKp();
    pid["ki"] = pidController->getKi();
    pid["kd"] = pidController->getKd();
    pid["error"] = pidController->getLastError();
    pid["pTerm"] = pidController->getProportionalTerm();
    pid["iTerm"] = pidController->getIntegralTerm();
    pid["dTerm"] = pidController->getDerivativeTerm();
  }
  
  // Connection state
  doc["knxConnected"] = true; // We don't have a direct way to check KNX connectivity
  doc["mqttConnected"] = mqttInterface ? mqttInterface->isConnected() : false;
  
  String response;
  serializeJson(doc, response);
  
  server.send(200, "application/json", response);
}

void WebInterface::handleNotFound() {
  server.send(404, "text/plain", "404: Not found");
}

String WebInterface::generateHtml() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>KNX Thermostat Configuration</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; }";
  html += ".container { max-width: 600px; margin: 0 auto; }";
  html += "label { display: inline-block; width: 120px; margin-bottom: 10px; }";
  html += "input[type='number'] { width: 60px; }";
  html += ".form-group { margin-bottom: 15px; border-bottom: 1px solid #ccc; padding-bottom: 15px; }";
  html += "h2 { color: #333; }";
  html += "button { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; cursor: pointer; }";
  html += "button:hover { background-color: #45a049; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>KNX Thermostat Configuration</h1>";
  
  html += "<form action='/save' method='post'>";
  
  // Device Settings
  html += "<div class='form-group'>";
  html += "<h2>Device Settings</h2>";
  html += "<label for='deviceName'>Device Name:</label>";
  html += "<input type='text' id='deviceName' name='deviceName' value='" + 
          String(configManager ? configManager->getDeviceName() : "KNX-Thermostat") + "'><br>";
  
  html += "<label for='sendInterval'>Send Interval (ms):</label>";
  html += "<input type='number' id='sendInterval' name='sendInterval' min='1000' step='1000' value='" + 
          String(configManager ? configManager->getSendInterval() : 10000) + "'><br>";
  
  html += "<label for='pidInterval'>PID Interval (ms):</label>";
  html += "<input type='number' id='pidInterval' name='pidInterval' min='1000' step='1000' value='" + 
          String(configManager ? configManager->getPidInterval() : 30000) + "'><br>";
  html += "</div>";
  
  // KNX Physical Address
  html += "<div class='form-group'>";
  html += "<h2>KNX Physical Address</h2>";
  html += "<label for='knxPhysicalArea'>Area:</label>";
  html += "<input type='number' id='knxPhysicalArea' name='knxPhysicalArea' min='0' max='15' value='" + 
          String(configManager ? configManager->getKnxPhysicalArea() : 1) + "'>";
  
  html += "<label for='knxPhysicalLine'>Line:</label>";
  html += "<input type='number' id='knxPhysicalLine' name='knxPhysicalLine' min='0' max='15' value='" + 
          String(configManager ? configManager->getKnxPhysicalLine() : 1) + "'>";
  
  html += "<label for='knxPhysicalMember'>Member:</label>";
  html += "<input type='number' id='knxPhysicalMember' name='knxPhysicalMember' min='0' max='255' value='" + 
          String(configManager ? configManager->getKnxPhysicalMember() : 201) + "'><br>";
  html += "</div>";
  
  // Temperature Group Address
  html += "<div class='form-group'>";
  html += "<h2>Temperature Group Address</h2>";
  html += "<label for='knxTempArea'>Area:</label>";
  html += "<input type='number' id='knxTempArea' name='knxTempArea' min='0' max='15' value='" + 
          String(configManager ? configManager->getKnxTempArea() : 3) + "'>";
  
  html += "<label for='knxTempLine'>Line:</label>";
  html += "<input type='number' id='knxTempLine' name='knxTempLine' min='0' max='15' value='" + 
          String(configManager ? configManager->getKnxTempLine() : 1) + "'>";
  
  html += "<label for='knxTempMember'>Member:</label>";
  html += "<input type='number' id='knxTempMember' name='knxTempMember' min='0' max='255' value='" + 
          String(configManager ? configManager->getKnxTempMember() : 0) + "'><br>";
  html += "</div>";
  
  // Setpoint Group Address
  html += "<div class='form-group'>";
  html += "<h2>Setpoint Group Address</h2>";
  html += "<label for='knxSetpointArea'>Area:</label>";
  html += "<input type='number' id='knxSetpointArea' name='knxSetpointArea' min='0' max='15' value='" + 
          String(configManager ? configManager->getKnxSetpointArea() : 3) + "'>";
  
  html += "<label for='knxSetpointLine'>Line:</label>";
  html += "<input type='number' id='knxSetpointLine' name='knxSetpointLine' min='0' max='15' value='" + 
          String(configManager ? configManager->getKnxSetpointLine() : 2) + "'>";
  
  html += "<label for='knxSetpointMember'>Member:</label>";
  html += "<input type='number' id='knxSetpointMember' name='knxSetpointMember' min='0' max='255' value='" + 
          String(configManager ? configManager->getKnxSetpointMember() : 0) + "'><br>";
  html += "</div>";
  
  // Valve Group Address
  html += "<div class='form-group'>";
  html += "<h2>Valve Group Address</h2>";
  html += "<label for='knxValveArea'>Area:</label>";
  html += "<input type='number' id='knxValveArea' name='knxValveArea' min='0' max='15' value='" + 
          String(configManager ? configManager->getKnxValveArea() : 3) + "'>";
  
  html += "<label for='knxValveLine'>Line:</label>";
  html += "<input type='number' id='knxValveLine' name='knxValveLine' min='0' max='15' value='" + 
          String(configManager ? configManager->getKnxValveLine() : 3) + "'>";
  
  html += "<label for='knxValveMember'>Member:</label>";
  html += "<input type='number' id='knxValveMember' name='knxValveMember' min='0' max='255' value='" + 
          String(configManager ? configManager->getKnxValveMember() : 0) + "'><br>";
  html += "</div>";
  
  // MQTT Settings
  html += "<div class='form-group'>";
  html += "<h2>MQTT Settings</h2>";
  html += "<label for='mqttServer'>MQTT Server:</label>";
  html += "<input type='text' id='mqttServer' name='mqttServer' value='" + 
          String(configManager ? configManager->getMqttServer() : "192.168.178.32") + "'><br>";
  
  html += "<label for='mqttPort'>MQTT Port:</label>";
  html += "<input type='number' id='mqttPort' name='mqttPort' value='" + 
          String(configManager ? configManager->getMqttPort() : 1883) + "'><br>";
  
  html += "<label for='mqttUser'>MQTT User:</label>";
  html += "<input type='text' id='mqttUser' name='mqttUser' value='" + 
          String(configManager ? configManager->getMqttUser() : "") + "'><br>";
  
  html += "<label for='mqttPassword'>MQTT Password:</label>";
  html += "<input type='password' id='mqttPassword' name='mqttPassword' value='" + 
          String(configManager ? configManager->getMqttPassword() : "") + "'><br>";
  
  html += "<label for='mqttClientId'>Client ID:</label>";
  html += "<input type='text' id='mqttClientId' name='mqttClientId' value='" + 
          String(configManager ? configManager->getMqttClientId() : "ESP32Thermostat") + "'><br>";
  html += "</div>";
  
  // PID Settings
  html += "<div class='form-group'>";
  html += "<h2>PID Settings</h2>";
  html += "<label for='kp'>Proportional (Kp):</label>";
  html += "<input type='number' id='kp' name='kp' step='0.1' value='" + 
          String(configManager ? configManager->getKp() : 1.0) + "'><br>";
  
  html += "<label for='ki'>Integral (Ki):</label>";
  html += "<input type='number' id='ki' name='ki' step='0.01' value='" + 
          String(configManager ? configManager->getKi() : 0.1) + "'><br>";
  
  html += "<label for='kd'>Derivative (Kd):</label>";
  html += "<input type='number' id='kd' name='kd' step='0.001' value='" + 
          String(configManager ? configManager->getKd() : 0.01) + "'><br>";
  
  html += "<label for='setpoint'>Setpoint (°C):</label>";
  html += "<input type='number' id='setpoint' name='setpoint' step='0.5' value='" + 
          String(configManager ? configManager->getSetpoint() : 21.0) + "'><br>";
  html += "</div>";
  
  // Current Status
  html += "<div class='form-group'>";
  html += "<h2>Current Status</h2>";
  html += "<p>Temperature: " + String(thermostatState ? thermostatState->currentTemperature : 0.0) + " °C</p>";
  html += "<p>Humidity: " + String(thermostatState ? thermostatState->currentHumidity : 0.0) + " %</p>";
  html += "<p>Pressure: " + String(thermostatState ? thermostatState->currentPressure : 0.0) + " hPa</p>";
  html += "<p>Valve Position: " + String(thermostatState ? thermostatState->valvePosition : 0.0) + " %</p>";
  html += "<p>Heating Active: " + String(thermostatState && thermostatState->heatingActive ? "Yes" : "No") + "</p>";
  html += "<p>IP Address: " + WiFi.localIP().toString() + "</p>";
  html += "<p>RSSI: " + String(WiFi.RSSI()) + " dBm</p>";
  html += "<p>MQTT Connection: " + String(mqttInterface && mqttInterface->isConnected() ? "Connected" : "Disconnected") + "</p>";
  html += "</div>";
  
  html += "<button type='submit'>Save Configuration</button>";
  html += "</form>";
  
  html += "<div style='margin-top: 20px;'>";
  html += "<a href='/reboot'><button type='button'>Reboot Device</button></a>";
  html += "<a href='/reset'><button type='button' style='background-color: #f44336; margin-left: 10px;'>Factory Reset</button></a>";
  html += "</div>";
  
  html += "</div></body></html>";
  
  return html;
}

void WebInterface::setupMDNS() {
  // Set up mDNS responder
  const char* hostname = configManager ? configManager->getDeviceName() : "knx-thermostat";
  
  if (MDNS.begin(hostname)) {
    Serial.print("mDNS responder started: http://");
    Serial.print(hostname);
    Serial.println(".local");
    
    // Add service to mDNS
    MDNS.addService("http", "tcp", 80);
  } else {
    Serial.println("Error setting up mDNS responder");
  }
}