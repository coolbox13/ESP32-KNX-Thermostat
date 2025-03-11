#include "web_interface.h"
#include <LittleFS.h>
#include "esp_log.h"
#include <ESPmDNS.h>
#include "web/base64.h"
#include "interfaces/sensor_interface.h"
#include "web/elegant_ota_async.h"

static const char* TAG = "WebInterface";

WebInterface::WebInterface(ConfigManager* configManager, SensorInterface* sensorInterface, 
                          PIDController* pidController, ThermostatState* thermostatState,
                          ProtocolManager* protocolManager)
    : server(80)
    , configManager(configManager)
    , sensorInterface(sensorInterface)
    , pidController(pidController)
    , thermostatState(thermostatState)
    , protocolManager(protocolManager)
    , otaInitialized(false) {
    ESP_LOGI(TAG, "Web interface initialized");
}

WebInterface::~WebInterface() {
    end();
}

bool WebInterface::begin() {
    ESP_LOGI(TAG, "Starting web interface...");
    
    // Note: LittleFS is now initialized in main.cpp
    
    try {
        // Set up request handlers
        server.on("/", HTTP_GET, std::bind(&WebInterface::handleRoot, this, std::placeholders::_1));
        server.on("/save", HTTP_POST, std::bind(&WebInterface::handleSave, this, std::placeholders::_1));
        server.on("/status", HTTP_GET, std::bind(&WebInterface::handleGetStatus, this, std::placeholders::_1));
        server.on("/setpoint", HTTP_POST, std::bind(&WebInterface::handleSetpoint, this, std::placeholders::_1));
        server.on("/mode", HTTP_POST, std::bind(&WebInterface::handleMode, this, std::placeholders::_1));
        server.on("/config", HTTP_POST, std::bind(&WebInterface::handleSaveConfig, this, std::placeholders::_1));
        server.on("/reboot", HTTP_POST, std::bind(&WebInterface::handleReboot, this, std::placeholders::_1));
        server.on("/factory_reset", HTTP_POST, std::bind(&WebInterface::handleFactoryReset, this, std::placeholders::_1));
        
        // Set up MDNS for easy access
        setupMDNS();
        
        // Set up 404 handler
        server.onNotFound(std::bind(&WebInterface::handleNotFound, this, std::placeholders::_1));
        
        // Initialize web server
        server.begin();
        ESP_LOGI(TAG, "Web interface started successfully");
        return true;
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Failed to start web interface: %s", e.what());
        return false;
    } catch (...) {
        ESP_LOGE(TAG, "Failed to start web interface: Unknown error");
        return false;
    }
}

void WebInterface::end() {
    server.end();
    ESP_LOGI(TAG, "Web interface stopped");
}

void WebInterface::loop() {
    // AsyncWebServer doesn't need explicit loop handling
}

void WebInterface::listFiles() {
    ESP_LOGI(TAG, "Listing files in LittleFS:");
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while(file) {
        ESP_LOGI(TAG, "File: %s, Size: %d bytes", file.name(), file.size());
        file = root.openNextFile();
    }
    root.close();
}

String WebInterface::generateHtml() {
    File file = LittleFS.open("/index.html", "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open index.html");
        return "Error: Failed to load web interface";
    }
    String html = file.readString();
    
    // Add CSRF token to the HTML
    String csrfToken = generateCSRFToken(nullptr);
    html.replace("{{CSRF_TOKEN}}", csrfToken);
    
    file.close();
    return html;
}

void WebInterface::setupMDNS() {
    // Set up MDNS responder
    if (MDNS.begin("thermostat")) {
        ESP_LOGI(TAG, "MDNS responder started");
        MDNS.addService("http", "tcp", 80);
    } else {
        ESP_LOGW(TAG, "Error setting up MDNS responder!");
    }
}

bool WebInterface::isAuthenticated(AsyncWebServerRequest* request) {
    if (!configManager->getWebUsername()[0]) {
        return true;  // No authentication required if username is empty
    }
    
    if (!request->authenticate(configManager->getWebUsername(), configManager->getWebPassword())) {
        return false;
    }
    return true;
}

void WebInterface::requestAuthentication(AsyncWebServerRequest* request) {
    request->requestAuthentication();
}

void WebInterface::addSecurityHeaders(AsyncWebServerResponse* response) {
    response->addHeader("X-Content-Type-Options", "nosniff");
    response->addHeader("X-XSS-Protection", "1; mode=block");
    response->addHeader("X-Frame-Options", "DENY");
    response->addHeader("Referrer-Policy", "no-referrer");
    response->addHeader("Content-Security-Policy", "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline';");
}

bool WebInterface::handleFileRead(AsyncWebServerRequest* request, String path) {
    ESP_LOGD(TAG, "handleFileRead: %s", path.c_str());
    
    if (path.endsWith("/")) {
        path += "index.html";
    }
    
    String contentType = getContentType(path);
    
    if (LittleFS.exists(path)) {
        File file = LittleFS.open(path, "r");
        if (file) {
            AsyncWebServerResponse *response = request->beginResponse(LittleFS, path, contentType);
            addSecurityHeaders(response);
            request->send(response);
            file.close();
            return true;
        }
    }
    return false;
}

String WebInterface::getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".json")) return "application/json";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";
}

bool WebInterface::validateCSRFToken(AsyncWebServerRequest* request) {
    if (!request->hasHeader("X-CSRF-Token")) {
        return false;
    }
    
    String token = request->header("X-CSRF-Token");
    // In a real implementation, you would validate the token against a stored value
    // For now, we'll accept any non-empty token
    return token.length() > 0;
}

String WebInterface::generateCSRFToken(AsyncWebServerRequest* request) {
    // In a real implementation, you would generate a secure random token
    // For now, we'll return a simple timestamp-based token
    return String(millis());
}