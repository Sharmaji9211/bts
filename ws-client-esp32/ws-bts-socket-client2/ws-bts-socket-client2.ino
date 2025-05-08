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