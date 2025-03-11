#include <Arduino.h>
#include "web/web_interface.h"
#include <ESPmDNS.h>
#include <LittleFS.h>
#include "esp_log.h"

static const char* TAG = "WebInterface";

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