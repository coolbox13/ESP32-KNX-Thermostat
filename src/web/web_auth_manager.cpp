#include "web/web_auth_manager.h"

#ifdef HTTP_DELETE
#undef HTTP_DELETE
#endif
#ifdef HTTP_GET
#undef HTTP_GET
#endif
#ifdef HTTP_HEAD
#undef HTTP_HEAD
#endif
#ifdef HTTP_POST
#undef HTTP_POST
#endif
#ifdef HTTP_PUT
#undef HTTP_PUT
#endif
#ifdef HTTP_OPTIONS
#undef HTTP_OPTIONS
#endif
#ifdef HTTP_PATCH
#undef HTTP_PATCH
#endif

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Arduino.h>
#include <esp_log.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config_manager.h"

static const char* TAG = "WebAuthManager";

WebAuthManager::WebAuthManager(AsyncWebServer& server, ConfigManager& configManager)
    : server(server)
    , configManager(configManager) {
    // Initialize with empty credentials
    memset(username, 0, sizeof(username));
    memset(password, 0, sizeof(password));
}

void WebAuthManager::setCredentials(const char* user, const char* pass) {
    // Safely copy username with null termination
    strncpy(username, user, sizeof(username) - 1);
    username[sizeof(username) - 1] = '\0';
    
    // Safely copy password with null termination
    strncpy(password, pass, sizeof(password) - 1);
    password[sizeof(password) - 1] = '\0';
    
    ESP_LOGI("WebAuthManager", "Credentials updated for user: %s", username);
}

bool WebAuthManager::authenticate(AsyncWebServerRequest* request) {
    // First check rate limiting
    String ip = request->client()->remoteIP().toString();
    if (!checkRateLimit(ip)) {
        return false;
    }
    
    // Check if session cookie exists
    String sessionId = request->header("Cookie");
    if (sessionId.length() > 0) {
        // Extract session ID from cookie
        int start = sessionId.indexOf("session=");
        if (start >= 0) {
            start += 8; // Length of "session="
            int end = sessionId.indexOf(";", start);
            if (end < 0) end = sessionId.length();
            sessionId = sessionId.substring(start, end);
            
            // Validate session
            if (validateSession(sessionId)) {
                return true;
            }
        }
    }
    
    // If no valid session, check basic auth
    if (configManager.getWebUsername()[0] != '\0') {
        if (request->authenticate(configManager.getWebUsername(), configManager.getWebPassword())) {
            // Create new session
            String newSession = createSession();
            return true;
        }
    } else {
        return true; // No authentication required if no credentials set
    }
    
    // If we get here, authentication failed
    requestAuthentication(request);
    return false;
}

void WebAuthManager::requestAuthentication(AsyncWebServerRequest* request) {
    AsyncWebServerResponse *response = request->beginResponse(401);
    response->addHeader("WWW-Authenticate", "Basic realm=\"Login Required\"");
    request->send(response);
}

bool WebAuthManager::validateSession(const String& sessionId) {
    auto it = sessions.find(sessionId);
    if (it != sessions.end()) {
        Session& session = it->second;
        unsigned long now = millis();
        
        // Check if session has expired
        if (now - session.created > SESSION_TIMEOUT) {
            sessions.erase(it);
            return false;
        }
        
        // Update last access time
        session.lastAccess = now;
        return true;
    }
    return false;
}

String WebAuthManager::createSession() {
    cleanupSessions();
    
    String sessionId = generateRandomString(32);
    Session session;
    session.created = millis();
    session.lastAccess = session.created;
    
    sessions[sessionId] = session;
    return sessionId;
}

void WebAuthManager::removeSession(const String& sessionId) {
    sessions.erase(sessionId);
}

bool WebAuthManager::validateCSRFToken(AsyncWebServerRequest *request, const String& token) {
    // Get the CSRF token from the session cookie
    String sessionId = request->header("Cookie");
    if (sessionId.length() > 0) {
        int start = sessionId.indexOf("session=");
        if (start >= 0) {
            start += 8; // Length of "session="
            int end = sessionId.indexOf(";", start);
            if (end < 0) end = sessionId.length();
            sessionId = sessionId.substring(start, end);
            
            // Check if session exists and is valid
            auto it = sessions.find(sessionId);
            if (it != sessions.end()) {
                // Compare the provided token with the session's CSRF token
                return token == it->second.csrfToken;
            }
        }
    }
    return false;
}

String WebAuthManager::generateCSRFToken(AsyncWebServerRequest *request) {
    // Get the current session ID
    String sessionId = request->header("Cookie");
    if (sessionId.length() > 0) {
        int start = sessionId.indexOf("session=");
        if (start >= 0) {
            start += 8; // Length of "session="
            int end = sessionId.indexOf(";", start);
            if (end < 0) end = sessionId.length();
            sessionId = sessionId.substring(start, end);
            
            // Check if session exists and is valid
            auto it = sessions.find(sessionId);
            if (it != sessions.end()) {
                // Generate a new CSRF token for this session
                String token = generateRandomString(32);
                it->second.csrfToken = token;
                return token;
            }
        }
    }
    
    // If no valid session, generate a temporary token
    return generateRandomString(32);
}

bool WebAuthManager::checkRateLimit(const String& ip) {
    cleanupRateLimits();
    
    auto it = rateLimits.find(ip);
    if (it != rateLimits.end()) {
        RateLimit& limit = it->second;
        unsigned long now = millis();
        
        // Reset if window has expired
        if (now - limit.firstAttempt > RATE_LIMIT_WINDOW) {
            limit.firstAttempt = now;
            limit.attempts = 1;
            return true;
        }
        
        // Check if too many attempts
        if (limit.attempts >= MAX_ATTEMPTS) {
            return false;
        }
        
        limit.attempts++;
        return true;
    }
    
    // First attempt for this IP
    RateLimit limit;
    limit.firstAttempt = millis();
    limit.attempts = 1;
    rateLimits[ip] = limit;
    return true;
}

void WebAuthManager::addSecurityHeaders(AsyncWebServerResponse *response) {
    response->addHeader("X-Content-Type-Options", "nosniff");
    response->addHeader("X-Frame-Options", "DENY");
    response->addHeader("X-XSS-Protection", "1; mode=block");
    response->addHeader("Strict-Transport-Security", "max-age=31536000; includeSubDomains");
    response->addHeader("Content-Security-Policy", "default-src 'self'");
    response->addHeader("Referrer-Policy", "same-origin");
}

void WebAuthManager::cleanupSessions() {
    unsigned long now = millis();
    auto it = sessions.begin();
    
    // Use safe iterator-based removal
    while (it != sessions.end()) {
        if (now - it->second.created > SESSION_TIMEOUT) {
            ESP_LOGD(TAG, "Removing expired session: %s", it->first.c_str());
            it = sessions.erase(it); // erase() returns the next iterator
        } else {
            ++it;
        }
    }
}

void WebAuthManager::cleanupRateLimits() {
    unsigned long now = millis();
    for (auto it = rateLimits.begin(); it != rateLimits.end();) {
        if (now - it->second.firstAttempt > RATE_LIMIT_WINDOW) {
            it = rateLimits.erase(it);
        } else {
            ++it;
        }
    }
}

String WebAuthManager::generateRandomString(size_t length) {
    const char charset[] = "0123456789"
                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                          "abcdefghijklmnopqrstuvwxyz";
    String result;
    result.reserve(length);
    
    for (size_t i = 0; i < length; i++) {
        int index = random(0, sizeof(charset) - 1);
        result += charset[index];
    }
    
    return result;
} 