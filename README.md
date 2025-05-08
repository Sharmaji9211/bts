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
```
WebSocket Issues:
  //const jsonDatadoc= json.stringify(event.data);done by websocket server not here
  
    //javascript object notation for above arduino json object which handle by client and server
  
    //  jsonDatadoc={
    // "occupancy"= counter.getPeopleCount();
    // "temperature" = counter.getTemperature();
    // "humidity"= counter.getHumidity();
    // "airQuality" = counter.getAirQuality();
    // "lightState"= counter.getLightState();
    // "fanState" = counter.getFanState();
    // "lightAuto" = counter.lightAutoMode;
    // "fanAuto" = counter.fanAutoMode;
    // "timestamp" = millis();
    // }
// renderToUI(jsonDatadoc);
//sendToDataBase(jsonDatadoc);
```
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

```
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// Constants
#define PIR_PIN_IN 34
#define PIR_PIN_OUT 35
#define LED_PIN 27
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define Fan_PIN 25
#define DHTPIN 4       // Changed from Temp_PIN to DHTPIN
#define MQ2_PIN 16
#define BUZZER_PIN 26
#define DHTTYPE DHT22

// WiFi and WebSocket setup
const char* ssid = "TCA@Admin";
const char* password = "Shivam@9211";
const char* websocket_server = "192.168.1.13";
const uint16_t websocket_port = 81;

WebSocketsClient webSocket;
unsigned long lastDataSend = 0;
const long dataInterval = 2000;
unsigned long lastReconnectAttempt = 0;
const long reconnectInterval = 3000;

// OLED Display Object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// DHT Sensor
DHT dht(DHTPIN, DHTTYPE);

// People Counter Class
class PeopleCounter {
private:
    int peopleCount;
    bool motionDetected_IN;
    bool motionDetected_OUT;
    unsigned long lastEventTime;
    String lastEventType;
    int lastPersonID;
    bool eventChanged;
    
    // Environmental sensors
    float temperature;
    float humidity;
    int airQuality;

public:
    bool lightAutoMode = true;
    bool fanAutoMode = true;
    int lightBrightness = 50;
    int fanSpeed = 0;

    PeopleCounter() {
        peopleCount = 0;
        motionDetected_IN = false;
        motionDetected_OUT = false;
        lastEventTime = 0;
        lastEventType = "";
        lastPersonID = 0;
        eventChanged = false;
        temperature = 0;
        humidity = 0;
        airQuality = 0;
    }

    void testHardwarePin() {
        pinMode(Fan_PIN, OUTPUT);
        pinMode(MQ2_PIN, INPUT_PULLUP);
        pinMode(BUZZER_PIN, OUTPUT);
        digitalWrite(Fan_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        dht.begin();
    }

    void initializeSystem() {
        pinMode(PIR_PIN_IN, INPUT_PULLUP);
        pinMode(PIR_PIN_OUT, INPUT_PULLUP);
        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN, LOW);
        Serial.begin(115200);

        if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
            Serial.println(F("SSD1306 allocation failed"));
            while (true);
        }

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(5, 5);
        display.println("Initializing...");
        display.display();
        delay(1000);
        display.clearDisplay();
    }

    void detectMotion() {
        int val_IN = digitalRead(PIR_PIN_IN);
        int val_OUT = digitalRead(PIR_PIN_OUT);

        if (val_IN == LOW && !motionDetected_IN) {
            motionDetected_IN = true;
            peopleCount++;
            lastPersonID++;
            lastEventTime = millis();
            lastEventType = "IN";
            eventChanged = true;
            Serial.println("Person Entered");
        } else if (val_IN == HIGH && motionDetected_IN) {
            motionDetected_IN = false;
        }

        if (val_OUT == LOW && !motionDetected_OUT) {
            motionDetected_OUT = true;
            if (peopleCount > 0) {
                peopleCount--;
                lastEventTime = millis();
                lastEventType = "OUT";
                eventChanged = true;
                Serial.println("Person Exited");
            }
        } else if (val_OUT == HIGH && motionDetected_OUT) {
            motionDetected_OUT = false;
        }
    }

    void controlLED() {
        if (lightAutoMode) {
            digitalWrite(LED_PIN, peopleCount > 0 ? HIGH : LOW);
        }
    }

    void updateOLED() {
        if (eventChanged) {
            displayTimeAndCount();
            eventChanged = false;
        }
    }

    void readEnvironment() {
        // Read DHT22
        float newTemp = dht.readTemperature();
        float newHum = dht.readHumidity();
        
        if (!isnan(newTemp)) temperature = newTemp;
        if (!isnan(newHum)) humidity = newHum;
        
        // Read MQ2
        airQuality = analogRead(MQ2_PIN);
    }

    void controlSystems() {
        if (fanAutoMode) {
            if (temperature > 24 && peopleCount > 0) {
                digitalWrite(Fan_PIN, HIGH);
                digitalWrite(BUZZER_PIN, (temperature > 28) ? HIGH : LOW);
            } else {
                digitalWrite(Fan_PIN, LOW);
                digitalWrite(BUZZER_PIN, LOW);
            }
        }
        
        if (analogRead(MQ2_PIN) > 1000) { // Gas detection threshold
            displayGasWarning();
        }
    }

    // Getters
    int getPeopleCount() { return peopleCount; }
    float getTemperature() { return temperature; }
    float getHumidity() { return humidity; }
    int getAirQuality() { return airQuality; }
    bool getFanState() { return digitalRead(Fan_PIN); }
    bool getLightState() { return digitalRead(LED_PIN); }

private:
    void displayTimeAndCount() {
        unsigned long seconds = (lastEventTime / 1000) % 60;
        unsigned long minutes = (lastEventTime / 60000) % 60;
        unsigned long hours = (lastEventTime / 3600000) % 24;

        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(1, 1);
        display.print("Event: ");
        display.print(lastEventType);
        display.setCursor(0, 15);
        display.print("Time: ");
        display.printf("%02d:%02d:%02d", hours, minutes, seconds);
        display.setCursor(0, 35);
        display.printf("Count: %d\nTemp: %.1fC\nHum: %.1f%%", 
                      peopleCount, temperature, humidity);
        display.display();
    }

    void displayGasWarning() {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("! WARNING !");
        display.println("Dangerous Gas");
        display.println("Detected!");
        display.display();
        digitalWrite(BUZZER_PIN, HIGH);
    }
} counter;

void setupWebSocket() {
    webSocket.begin(websocket_server, websocket_port, "/");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(2000);
    webSocket.enableHeartbeat(15000, 3000, 2);
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[WSc] Disconnected!");
            break;
        case WStype_CONNECTED:
            Serial.println("[WSc] Connected to server");
            webSocket.sendTXT("{\"type\":\"handshake\",\"device\":\"ESP32\"}");
            break;
        case WStype_TEXT:
            handleIncomingCommand(payload, length);
            break;
        case WStype_ERROR:
            Serial.printf("[WSc] Error: %s\n", payload);
            break;
    }
}
//cammands come from  web browser -client
void handleIncomingCommand(const uint8_t* payload, size_t length) {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

    if (doc.containsKey("command")) {
        const char* command = doc["command"];
        
        if (strcmp(command, "light") == 0) {
            counter.lightAutoMode = false;
            digitalWrite(LED_PIN, doc["value"].as<bool>());
        }
        else if (strcmp(command, "fan") == 0) {
            counter.fanAutoMode = false;
            digitalWrite(Fan_PIN, doc["value"].as<bool>());
        }
        else if (strcmp(command, "mode") == 0) {
            counter.lightAutoMode = doc["lightAuto"];
            counter.fanAutoMode = doc["fanAuto"];
        }
    }
}
//data send to the websocket server and web browser -client
void sendSensorData() {
    DynamicJsonDocument doc(512);
    
    doc["occupancy"] = counter.getPeopleCount();
    doc["temperature"] = counter.getTemperature();
    doc["humidity"] = counter.getHumidity();
    doc["airQuality"] = counter.getAirQuality();
    doc["lightState"] = counter.getLightState();
    doc["fanState"] = counter.getFanState();
    doc["lightAuto"] = counter.lightAutoMode;
    doc["fanAuto"] = counter.fanAutoMode;
    doc["timestamp"] = millis();

    
    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.sendTXT(jsonString);
   
  
}

void networkTask() {
    webSocket.loop();
    
    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - lastReconnectAttempt > reconnectInterval) {
            lastReconnectAttempt = millis();
            WiFi.reconnect();
            Serial.println("Reconnecting to WiFi...");
        }
    }
    else if (!webSocket.isConnected()) {
        if (millis() - lastReconnectAttempt > reconnectInterval) {
            lastReconnectAttempt = millis();
            setupWebSocket();
            Serial.println("Reconnecting to WebSocket...");
        }
    }
    
    if (webSocket.isConnected() && millis() - lastDataSend > dataInterval) {
        lastDataSend = millis();
        sendSensorData();
    }
}

void setup() {
    counter.initializeSystem();
    counter.testHardwarePin();
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    setupWebSocket();
}

void loop() {
    counter.detectMotion();
    counter.controlLED();
    counter.readEnvironment();
    counter.controlSystems();
    counter.updateOLED();
    
    networkTask();
    delay(100); // Maintain system responsiveness
}
```

```
const WebSocket = require('ws');
const express = require('express');
const path = require('path');

// Create HTTP server
const app = express();
const PORT = 81;
const server = app.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
});

// Serve static files for dashboard
app.use(express.static(path.join(__dirname, 'public')));

// Create WebSocket server
const wss = new WebSocket.Server({ server });

// Store connected clients and latest sensor data
const clients = new Set();
let classroomState = {
  occupancy: 0,
  temperature: 0,
  humidity: 0,
  airQuality: 0,
  lightState: false,
  fanState: false,
  lightAuto: true,
  fanAuto: true,
  lastUpdate: null
};

wss.on('connection', (ws) => {
  console.log('New client connected');
  clients.add(ws);

  // Send initial state to new client
  ws.send(JSON.stringify({
    type: 'init',
    data: classroomState
  }));

  // Heartbeat system
  const heartbeatInterval = setInterval(() => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.ping();
    }
  }, 15000);

  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message);
      console.log(data);
      // Handle different message types
      switch(data.type) {
        case 'sensorUpdate': // From ESP32
          handleSensorUpdate(data);
          break;
          
        case 'controlCommand': // From Dashboard
          forwardControlCommand(data);
          break;
          
        case 'pong':
          // Update last heartbeat
          ws.isAlive = true;
          break;
      }
    } catch (e) {
      console.error('Error processing message:', e);
    }
  });

  ws.on('close', () => {
    console.log('Client disconnected');
    clients.delete(ws);
    clearInterval(heartbeatInterval);
  });

  ws.on('error', (error) => {
    console.error('WebSocket error:', error);
  });

  ws.on('pong', () => {
    ws.isAlive = true;
  });
});

// Handle sensor updates from ESP32
function handleSensorUpdate(data) {
  // Update classroom state
  classroomState = {
    ...classroomState,
    ...data,
    lastUpdate: new Date().toISOString()
  };

  // Broadcast to all dashboard clients
  broadcastToDashboards({
    type: 'stateUpdate',
    data: classroomState
  });
}

// Forward control commands to ESP32
function forwardControlCommand(data) {
  clients.forEach(client => {
    if (client.deviceType === 'esp32' && client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify({
        type: 'control',
        command: data.command,
        value: data.value
      }));
    }
  });
}

// Broadcast to all dashboard clients
function broadcastToDashboards(message) {
  clients.forEach(client => {
    if (client.deviceType === 'dashboard' && client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify(message));
    }
  });
}

// Cleanup dead connections
setInterval(() => {
  clients.forEach(client => {
    if (!client.isAlive) {
      client.terminate();
      clients.delete(client);
      return;
    }
    client.isAlive = false;
  });
}, 30000);

// Identify device type (ESP32/Dashboard)
wss.on('headers', (headers, req) => {
  const deviceType = req.url === '/esp32' ? 'esp32' : 'dashboard';
  headers.push(`X-Device-Type: ${deviceType}`);
});
```

```

<!DOCTYPE html>
<html>
<head>
    <title>Classroom Monitor</title>
    <style>
        .dashboard {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 20px;
            padding: 20px;
        }
        .card {
            border: 1px solid #ccc;
            padding: 20px;
            border-radius: 8px;
        }
    </style>
</head>
<body>
    <div class="dashboard">
        <div class="card">
            <h2>Occupancy: <span id="occupancy">0</span></h2>
        </div>
        <div class="card">
            <h2>Temperature: <span id="temperature">0</span>°C</h2>
        </div>
        <div class="card">
            <h2>Humidity: <span id="humidity">0</span>%</h2>
        </div>
        <div class="card">
            <h2>Air Quality: <span id="airQuality">0</span></h2>
        </div>
        <div class="card">
            <h3>Light Control</h3>
            <button onclick="sendCommand('light', true)">ON</button>
            <button onclick="sendCommand('light', false)">OFF</button>
            <label>
                <input type="checkbox" id="lightAuto" onchange="toggleAutoMode('light')">
                Auto Mode
            </label>
        </div>
        <div class="card">
            <h3>Fan Control</h3>
            <button onclick="sendCommand('fan', true)">ON</button>
            <button onclick="sendCommand('fan', false)">OFF</button>
            <label>
                <input type="checkbox" id="fanAuto" onchange="toggleAutoMode('fan')">
                Auto Mode
            </label>
        </div>
    </div>

    <script>
        //create new  websocket client to connect with ws server
        const ws = new WebSocket('ws://localhost:81');

        ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            if (data.type === 'stateUpdate') {
                updateDashboard(data.data);
            }
        };

        function updateDashboard(state) {
            document.getElementById('occupancy').textContent = state.occupancy;
            document.getElementById('temperature').textContent = state.temperature.toFixed(1);
            document.getElementById('humidity').textContent = state.humidity.toFixed(1);
            document.getElementById('airQuality').textContent = state.airQuality;
            document.getElementById('lightAuto').checked = state.lightAuto;
            document.getElementById('fanAuto').checked = state.fanAuto;
        }

        function sendCommand(command, value) {
            ws.send(JSON.stringify({
                type: 'controlCommand',
                command: command,
                value: value
            }));
        }

        function toggleAutoMode(device) {
            const autoMode = document.getElementById(`${device}Auto`).checked;
            ws.send(JSON.stringify({
                type: 'controlCommand',
                command: 'mode',
                device: device,
                value: autoMode
            }));
        }
    </script>
</body>
</html>
```