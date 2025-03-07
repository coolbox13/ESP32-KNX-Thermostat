# ESP32-KNX-Thermostat

A modular thermostat solution for ESP32/ESP8266 with KNX integration, providing direct control of KNX heating valves. This project integrates BME280 temperature/humidity/pressure sensing with KNX communication and advanced PID control.

## Architecture

The project follows a modular architecture with clear separation of concerns:

- **Configuration Management**: Handles settings and persistent storage
- **Sensor Interface**: Manages BME280 sensor readings
- **KNX Interface**: Handles KNX communication
- **MQTT Interface**: Optional integration with Home Assistant
- **PID Controller**: Implements the control algorithm
- **Web Interface**: Provides configuration UI
- **Thermostat State**: Central state management with event callbacks

## Features

- **Direct KNX Communication**: Controls heating valves via KNX
- **Environmental Monitoring**: Temperature, humidity, pressure sensing
- **Web Configuration**: Easy setup through browser interface
- **WiFi Manager**: Simple network setup with captive portal
- **MQTT Support**: Integration with Home Assistant
- **PID Control**: Advanced temperature regulation
- **Persistent Settings**: All configuration stored in flash memory

## Hardware Requirements

- ESP32 or ESP8266 (NodeMCU ESP32-S recommended)
- BME280 temperature/humidity/pressure sensor
- I²C connection between ESP and BME280
- Power supply (USB or external)

## KNX Configuration

This thermostat uses standard KNX datapoints:
- **Temperature**: DPT 9.001 (2-byte float)
- **Setpoint**: DPT 9.001 (2-byte float)
- **Valve Position**: DPT 5.001 (1-byte percentage)
- **Operating Mode**: DPT 5.xxx (1-byte unsigned value)

Default group addresses:
- **Temperature**: 3/1/0
- **Setpoint**: 3/2/0
- **Valve Position**: 3/3/0
- **Mode**: 3/4/0

## Getting Started

### Development Environment

1. Clone this repository
2. Install PlatformIO (recommended) or configure Arduino IDE
3. Build and upload to your device

### Initial Setup

1. After uploading, connect to the "KNX-Thermostat" WiFi network
2. Configure WiFi settings in the captive portal
3. Access web interface at http://knx-thermostat.local or device IP
4. Configure KNX settings to match your installation

## Development Roadmap

1. **Core Integration**: Basic temperature reading and KNX communication
2. **PID Implementation**: Valve control based on temperature difference
3. **Web Interface Enhancement**: Complete configuration capabilities
4. **Operating Modes**: Support for multiple operating modes
5. **MQTT Integration**: Support for Home Assistant and other systems

## Directories Structure

```
ESP32-KNX-Thermostat/
├── include/                 # Header files
│   ├── config_manager.h     # Configuration handling
│   ├── knx_interface.h      # KNX communication
│   ├── mqtt_interface.h     # MQTT communication
│   ├── pid_controller.h     # PID control algorithm
│   ├── sensor_interface.h   # BME280 sensor handling
│   ├── thermostat_state.h   # Central state management
│   └── web_interface.h      # Web configuration UI
├── src/
│   ├── communication/       # Communication modules
│   │   ├── knx_interface.cpp
│   │   ├── mqtt_interface.cpp
│   │   └── web_interface.cpp
│   ├── config/              # Configuration handling
│   │   └── config_manager.cpp
│   ├── control/             # Control algorithms
│   │   └── pid_controller.cpp
│   ├── main.cpp             # Main application entry
│   └── sensors/             # Sensor handling
│       └── sensor_interface.cpp
└── platformio.ini           # PlatformIO configuration
```

## Contribution

Contributions welcome! Please follow the modular architecture and maintain separation of concerns when submitting pull requests.

## License

This project is released under the MIT License.