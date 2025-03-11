#include "web/web_interface.h"
#include <LittleFS.h>
#include "esp_log.h"
#include <ESPmDNS.h>
#include "web/base64.h"
#include "interfaces/sensor_interface.h"
#include "web/elegant_ota_async.h"

static const char* TAG = "WebInterface";

void WebInterface::listFiles() {
    Serial.println("Listing files in LittleFS:");
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    
    while (file) {
        if (!file.isDirectory()) {
            Serial.printf("File: %s, Size: %d bytes\n", file.name(), file.size());
        }
        file = root.openNextFile();
    }
}