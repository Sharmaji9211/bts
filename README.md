"# Bright Track System" 
# Smart Classroom Monitoring System

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A real-time IoT solution for monitoring classroom occupancy, environmental conditions, and air quality with remote control capabilities.

## Features

- 👥 People counting with entry/exit detection
- 🌡️ Temperature & humidity monitoring (DHT22)
- ⚠️ Air quality detection (MQ2 gas sensor)
- 📊 OLED display for local monitoring
- 🌐 Web dashboard for remote monitoring
- 🔔 Automatic alerts for abnormal conditions
- 💡 Smart lighting and fan control
- ↔️ Bi-directional WebSocket communication

## Hardware Requirements

| Component               | Quantity |
|-------------------------|----------|
| ESP32 Development Board | 1        |
| PIR Motion Sensors      | 2        |
| DHT22 Temperature Sensor| 1        |
| MQ2 Gas Sensor          | 1        |
| SSD1306 OLED Display    | 1        |
| 5V Relay Module         | 2        |
| LED Strip               | 1        |
| Cooling Fan             | 1        |
| Breadboard & Jumper Wires | As needed |

## Connections

| ESP32 Pin | Component     |
|-----------|---------------|
| GPIO34    | PIR_IN        |
| GPIO35    | PIR_OUT       |
| GPIO4     | DHT22         |
| GPIO16    | MQ2           |
| GPIO27    | LED           |
| GPIO25    | Fan Relay     |
| GPIO26    | Buzzer        |
| SDA       | OLED SDA      |
| SCL       | OLED SCL      |

## Software Requirements

- Arduino IDE
- Node.js v16+
- PlatformIO (recommended)
- Modern Web Browser

## Installation

### ESP32 Firmware

1. Install required libraries:
   ```bash
   pio lib install "Adafruit SSD1306" "DHT sensor library" "WebSockets" "ArduinoJson"
   ```

```
Update WiFi credentials in config.h:

cpp
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
Upload code to ESP32

Server Setup
Install dependencies:

bash
npm install express ws
Start the server:

bash
npm start
Usage
Power on the ESP32 system

Access dashboard at http://localhost:81

Monitor real-time data:

Occupancy count

Temperature/Humidity

Air quality index

Control modes:

Auto: System-controlled based on thresholds

Manual: Direct control from dashboard

Dashboard Features
Real-time sensor data visualization

Historical data graphs

Manual override controls

Alert notifications

System status indicators

Mobile-responsive design

Configuration
Modify config.h for:

cpp
// Environmental thresholds
#define MAX_TEMP 28          // °C
#define MIN_TEMP 18
#define GAS_THRESHOLD 1000   // ppm

// Timing intervals
#define SEND_INTERVAL 2000   // ms
#define RECONNECT_TIME 3000  // ms
Troubleshooting
Port Conflict:

bash
# Find process using port 81
sudo lsof -i :81
# Kill process
kill -9 <PID>
Sensor Errors:

Check wiring connections

Verify sensor power supply

Calibrate MQ2 sensor

WebSocket Issues:

Verify ESP32 WiFi connection

Check server accessibility

Inspect browser console logs

Safety Precautions
Use proper insulation for high-voltage components

Maintain adequate ventilation for gas sensor

Keep system away from water sources

Regularly inspect electrical connections

License
This project is licensed under the MIT License - see the LICENSE file for details.

Contributing
Fork the repository

Create your feature branch (git checkout -b feature/AmazingFeature)

Commit your changes (git commit -m 'Add some AmazingFeature')

Push to the branch (git push origin feature/AmazingFeature)

Open a Pull Request


This README includes:
1. Project overview and features
2. Hardware/software requirements
3. Installation and setup instructions
4. Usage guidelines
5. Configuration options
6. Troubleshooting common issues
7. Safety information
8. Licensing and contribution guidelines

```"# bts" 
