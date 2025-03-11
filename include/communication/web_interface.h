#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "thermostat_state.h"
#include "config_manager.h"

// Forward declarations
class SensorInterface;
class PIDController;
class ProtocolManager;

class WebInterface {
public:
    WebInterface(ConfigManager* configManager, SensorInterface* sensorInterface, 
                PIDController* pidController, ThermostatState* thermostatState,
                ProtocolManager* protocolManager);
    virtual ~WebInterface();

    bool begin();
    void end();
    void loop();
    void listFiles();
    
    // Request handlers
    void handleRoot(AsyncWebServerRequest* request);
    void handleSave(AsyncWebServerRequest* request);
    void handleGetStatus(AsyncWebServerRequest* request);
    void handleSetpoint(AsyncWebServerRequest* request);
    void handleSaveConfig(AsyncWebServerRequest* request);
    void handleReboot(AsyncWebServerRequest* request);
    void handleFactoryReset(AsyncWebServerRequest* request);
    void handleNotFound(AsyncWebServerRequest* request);
    void handleMode(AsyncWebServerRequest* request);
    
    // Utility methods
    bool handleFileRead(AsyncWebServerRequest* request, String path);
    void addSecurityHeaders(AsyncWebServerResponse* response);
    bool isAuthenticated(AsyncWebServerRequest* request);
    void requestAuthentication(AsyncWebServerRequest* request);
    bool validateCSRFToken(AsyncWebServerRequest* request);
    String generateCSRFToken(AsyncWebServerRequest* request);
    String getContentType(String filename);
    String generateHtml();
    void setupMDNS();

private:
    AsyncWebServer server;
    ConfigManager* configManager;
    SensorInterface* sensorInterface;
    PIDController* pidController;
    ThermostatState* thermostatState;
    ProtocolManager* protocolManager;
    bool otaInitialized;
};