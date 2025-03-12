# ESP32-KNX-Thermostat (Minimal Version)

This is a minimized version of the ESP32-KNX-Thermostat project, focusing on core functionality without the web interface and OTA update capabilities.

## Features

- **PID Control**: Precise temperature control using PID algorithm
- **BME280 Sensor**: Temperature, humidity, and pressure sensing
- **KNX Interface**: Integration with KNX building automation systems
- **MQTT Interface**: Integration with MQTT for IoT connectivity
- **Configuration via JSON**: Simple configuration using a JSON file stored in LittleFS
- **WiFi Manager**: Fallback to AP mode for WiFi configuration if connection fails

## Removed Features

- Web interface for configuration
- OTA (Over-The-Air) updates

## Hardware Requirements

- ESP32 development board
- BME280 sensor (I2C interface)
- KNX interface hardware (if using KNX)

## Configuration

Configuration is done by editing the `config.json` file and uploading it to the ESP32's LittleFS filesystem. The file should have the following structure:

```json
{
  "web": {
    "username": "admin",
    "password": "admin"
  },
  "wifi": {
    "ssid": "your_wifi_ssid",
    "password": "your_wifi_password"
  },
  "knx": {
    "enabled": true,
    "physical": {
      "area": 1,
      "line": 1,
      "member": 160
    }
  },
  "mqtt": {
    "enabled": true,
    "server": "192.168.178.32",
    "port": 1883,
    "username": "",
    "password": "",
    "clientId": "esp32_thermostat",
    "topicPrefix": "esp32/thermostat/"
  },
  "pid": {
    "kp": 2.0,
    "ki": 0.5,
    "kd": 1.0,
    "minOutput": 0.0,
    "maxOutput": 100.0,
    "sampleTime": 30000.0
  },
  "device": {
    "name": "ESP32 Thermostat",
    "sendInterval": 60000
  }
}
```

## WiFi Configuration

WiFi credentials can be configured in two ways:

1. **Via config.json**: Edit the `wifi` section in the `config.json` file with your SSID and password.

2. **Via WiFi Manager**: If the device cannot connect to WiFi using the stored credentials, it will create an access point named "ESP32-Thermostat". Connect to this AP with your phone or computer, and a configuration portal will open where you can enter your WiFi credentials.

## Building and Flashing

1. Install PlatformIO
2. Clone this repository
3. Build the project: `pio run`
4. Upload to ESP32: `pio run --target upload`
5. Upload filesystem: `pio run --target uploadfs`

## Monitoring

You can monitor the device using the serial port:

```
pio device monitor
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.