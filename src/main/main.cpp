#include <Arduino.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#ifdef ESP32
  #include <WiFi.h>
  #include <WebServer.h>
  #include <ESPmDNS.h>
  #define WebServerClass WebServer
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
  #define WebServerClass ESP8266WebServer
#endif

// Global objects
Adafruit_BME280 bme;
WebServerClass server(80);
bool bmeAvailable = false;

// Function prototypes
bool setupSensors();
void setupWebServer();
void handleRoot();

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nStarting ESP32-KNX-Thermostat...");
  
  // Initialize sensors (including handling missing BME280)
  bmeAvailable = setupSensors();
  
  // Initialize WiFi
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180); // Auto-exit config portal after 3 minutes
  
  if (!wifiManager.autoConnect("KNX-Thermostat")) {
    Serial.println("Failed to connect to WiFi - restarting");
    delay(1000);
    ESP.restart();
  }
  
  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Setup web server
  setupWebServer();
  
  Serial.println("Setup completed");
}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Read BME280 every 10 seconds if available
  static unsigned long lastReadTime = 0;
  if (bmeAvailable && millis() - lastReadTime > 10000) {
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F;
    
    Serial.println("Sensor readings:");
    Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
    Serial.print("Pressure: "); Serial.print(pressure); Serial.println(" hPa");
    
    lastReadTime = millis();
  }
  
  // Small delay to prevent CPU hogging
  delay(10);
}

bool setupSensors() {
  Serial.println("Setting up sensors...");
  
  // Initialize I2C
  Wire.begin();
  
  // Try to initialize BME280 sensor
  if (!bme.begin(0x76)) {
    // Try alternate address
    if (!bme.begin(0x77)) {
      Serial.println("Could not find BME280 sensor, check wiring or try different address!");
      return false;
    }
  }
  
  Serial.println("BME280 sensor found and initialized!");
  
  // Configure BME280
  bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF);
  
  return true;
}

void setupWebServer() {
  // Configure web server routes
  server.on("/", HTTP_GET, handleRoot);
  
  // Start web server
  server.begin();
  
  // Set up mDNS responder
  if (MDNS.begin("knx-thermostat")) {
    Serial.println("mDNS responder started: http://knx-thermostat.local");
    MDNS.addService("http", "tcp", 80);
  } else {
    Serial.println("Error setting up mDNS responder");
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>KNX Thermostat</title>";
  html += "<style>body{font-family:Arial;margin:20px;} .container{max-width:600px;margin:0 auto;}</style>";
  html += "</head><body><div class='container'>";
  html += "<h1>KNX Thermostat</h1>";
  
  // Display sensor information
  html += "<div style='margin:20px 0;padding:15px;border:1px solid #ddd;border-radius:5px;'>";
  html += "<h2>Sensor Status</h2>";
  
  if (bmeAvailable) {
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F;
    
    html += "<p>Temperature: " + String(temperature) + " °C</p>";
    html += "<p>Humidity: " + String(humidity) + " %</p>";
    html += "<p>Pressure: " + String(pressure) + " hPa</p>";
  } else {
    html += "<p>BME280 sensor not found</p>";
  }
  
  html += "</div>";
  
  // Display system information
  html += "<div style='margin:20px 0;padding:15px;border:1px solid #ddd;border-radius:5px;'>";
  html += "<h2>System Information</h2>";
  html += "<p>IP Address: " + WiFi.localIP().toString() + "</p>";
  html += "<p>RSSI: " + String(WiFi.RSSI()) + " dBm</p>";
  html += "<p>Uptime: " + String(millis() / 1000 / 60) + " minutes</p>";
  html += "</div>";
  
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}