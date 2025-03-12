# ESP32 KNX Gateway API Documentation

This document provides detailed information about the available functions and their usage in the ESP32 KNX Gateway project.

## Core Classes

### ESPKNXIP

The main class handling KNX communication and web server integration.

#### Initialization

```cpp
ESPKNXIP knx;
```

#### Methods

##### start
```cpp
void start(AsyncWebServer *srv)
```
Initializes the KNX interface with an AsyncWebServer instance.
- **Parameters:**
  - `srv`: Pointer to AsyncWebServer instance
- **Returns:** void
- **Usage:**
  ```cpp
  AsyncWebServer server(80);
  knx.start(&server);
  ```

##### physical_address_set
```cpp
void physical_address_set(address_t physical_address)
```
Sets the physical address for the KNX device.
- **Parameters:**
  - `physical_address`: KNX physical address structure
- **Returns:** void
- **Usage:**
  ```cpp
  knx.physical_address_set(knx.PA_to_address(1, 1, 160));
  ```

##### PA_to_address
```cpp
address_t PA_to_address(uint8_t area, uint8_t line, uint8_t member)
```
Converts physical address components to address structure.
- **Parameters:**
  - `area`: Area number (0-15)
  - `line`: Line number (0-15)
  - `member`: Member number (0-255)
- **Returns:** address_t structure
- **Usage:**
  ```cpp
  address_t addr = knx.PA_to_address(1, 1, 160);
  ```

##### GA_to_address
```cpp
address_t GA_to_address(uint8_t main, uint8_t middle, uint8_t sub)
```
Converts group address components to address structure.
- **Parameters:**
  - `main`: Main group (0-31)
  - `middle`: Middle group (0-7)
  - `sub`: Sub group (0-255)
- **Returns:** address_t structure
- **Usage:**
  ```cpp
  address_t addr = knx.GA_to_address(10, 6, 5);
  ```

##### write_2byte_float
```cpp
void write_2byte_float(address_t const &addr, float value)
```
Sends a 2-byte float value to a KNX group address.
- **Parameters:**
  - `addr`: Target KNX group address
  - `value`: Float value to send
- **Returns:** void
- **Usage:**
  ```cpp
  knx.write_2byte_float(tempAddress, 24.0);
  ```

##### loop
```cpp
void loop()
```
Handles KNX communication processing. Must be called in the main loop.
- **Parameters:** none
- **Returns:** void
- **Usage:**
  ```cpp
  void loop() {
    knx.loop();
  }
  ```

## Web Server Routes

### Root Handler
```cpp
void handleRoot(AsyncWebServerRequest *request)
```
Handles requests to the root path ('/').
- **Parameters:**
  - `request`: Pointer to AsyncWebServerRequest
- **Returns:** void
- **Response:** HTML dashboard

### Test Route
```cpp
HTTP GET /test
```
Returns a test message.
- **Response Type:** text/plain
- **Response:** "Hello from ESP32 AsyncWebServer!"

### Ping Route
```cpp
HTTP GET /ping
```
Server health check endpoint.
- **Response Type:** text/plain
- **Response:** "pong"

### Server Test Route
```cpp
HTTP GET /servertest
```
Verifies server functionality.
- **Response Type:** text/plain
- **Response:** "AsyncWebServer is working"

## Data Structures

### address_t
Structure representing KNX addresses.
```cpp
typedef union {
  struct {
    uint8_t area;
    uint8_t line;
    uint8_t member;
  } pa;  // Physical address
  struct {
    uint8_t main;
    uint8_t middle;
    uint8_t sub;
  } ga;  // Group address
  uint16_t value;  // Raw address value
} address_t;
```

## Configuration Constants

```cpp
const unsigned long sendInterval = 60000;  // Temperature update interval (ms)
```

## Debug Functions

### ESP Logging
```cpp
esp_log_level_set("ASYNCTCP", ESP_LOG_VERBOSE);
esp_log_level_set("WEBSERVER", ESP_LOG_VERBOSE);
esp_log_level_set("KNXIP", ESP_LOG_DEBUG);
```

## Integration Example

```cpp
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "esp-knx-ip.h"

AsyncWebServer server(80);
ESPKNXIP knx;

void setup() {
  // Initialize web server
  server.on("/", HTTP_GET, handleRoot);
  
  // Initialize KNX
  knx.start(&server);
  knx.physical_address_set(knx.PA_to_address(1, 1, 160));
  
  // Start server
  server.begin();
  
  // Configure KNX address
  address_t tempAddress = knx.GA_to_address(10, 6, 5);
  
  // Send data
  knx.write_2byte_float(tempAddress, 24.0);
}

void loop() {
  knx.loop();
}
```

## Best Practices

1. Always call `knx.loop()` in the main loop
2. Initialize the web server before starting KNX
3. Validate IP address before starting services
4. Use appropriate logging levels for debugging
5. Handle web requests asynchronously
6. Implement error handling for KNX communications

## Error Handling

The library provides several ways to handle errors:
- Return values from KNX functions
- ESP logging system
- Serial debugging output

## Performance Considerations

- Web server runs asynchronously for better performance
- KNX messages are processed in the main loop
- Temperature updates are time-controlled
- Memory usage is optimized for ESP32 