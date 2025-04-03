#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h" // For DHT11 sensor
#include <EEPROM.h> // EEPROM for persistent threshold storage



/************************************************
 * ESP32 Smart Planter
 *
 * Features:
 * 1) Connects to Wi-Fi and an MQTT broker.
 * 2) Every 10 minutes, publishes soil moisture, temperature,
 *    humidity, light level, and water level to "planter01/sensors" (JSON).
 * 3) Monitors soil moisture and drives pump intermittently below threshold:
 *    - Publishes "pump activated"/"pump stopped" to "planter01/alarm".
 *    - Will not activate pump if water level is 0%.
 * 4) When water level is <= 25%, publishes "water level low" to "planter01/alarm".
 * 5) Lights up different LEDs based on water level:
 *    - Green LED (GPIO 5) when 100%
 *    - Blue LED (GPIO 15) when 75% to 25%
 *    - Red LED (GPIO 14) when 0%
 * 6) Listens on MQTT topic 'planter01/pump' for "pump on" to trigger manual watering.
 * 7) Listens on MQTT topic 'planter01/save' for a moisture threshold value (0–100):
 *    - Updates `thresholdPct` used to determine watering.
 *    - Saves the value to EEPROM
 *    - The value persists even after rebooting the ESP32.
 ************************************************/

/************************************************
 * 1. Wi-Fi & MQTT Credentials
 ************************************************/
#define WIFI_SSID       "MTR01-2G"
#define WIFI_PASSWORD   "ftfe1042"
#define MQTT_SERVER     "192.168.8.172"
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "ESP32SmartPlanter"

/************************************************
 * 2. GPIO Assignments (from your sketches + LED pins)
 ************************************************/
// Soil Moisture Sensor (analog input)
const int soilMoisturePin = 39;

// DHT11 Temperature/Humidity sensor
#define DHTPIN   4    // DHT11 on GPIO 4
#define DHTTYPE  DHT11
DHT dht(DHTPIN, DHTTYPE);

// Light Sensor (analog input)
const int lightSensorPin = 36;

// Water Level Sensor
const int waterLevelGroundPin    = 35;            // Ground reference
const int waterLevelPins[]       = {26, 25, 33, 32}; // 100%, 75%, 50%, 25%
const int waterLevelAssociated[] = {100, 75, 50, 25, 0};
const int waterLevelSensorThreshold = 100; // Adjust if needed

// Water Pump (MOSFET gate)
const int pumpPin = 17;

// Water Level LED Indicators
const int greenLED = 5;   // Lights up at 100%
const int blueLED  = 15;  // Lights up at 75% to 25%
const int redLED   = 14;  // Lights up at 0%

/************************************************
 * 3. Global Configuration & Variables
 ************************************************/
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Publish interval
const unsigned long sensorPublishInterval = 2000UL; // 3 Seconds (for testing)
unsigned long lastSensorPublish = 0;

// Soil moisture calibration (adjust to match your tests)
int soilMoistureDry  = 2650; // Raw analog reading for very dry
int soilMoistureWet  = 1115; // Raw analog reading for very wet
float thresholdPct = 50; // Start pumping if soil moisture level is below this value (percentage)

// Pump control
bool pumpActive = false;
unsigned long pumpIntervalStart = 0;
const unsigned long pumpOnTime  = 2000;   // 2 seconds ON
const unsigned long pumpOffTime = 10000;  // 10 seconds OFF for soaking

// Water level alert
int  waterLevelPercentThreshold = 25;  
bool waterLevelLowAlertSent     = false;

/************************************************
 * 4. Wi-Fi Setup
 ************************************************/
void setupWifi() {
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

/************************************************
 * 5. MQTT Setup & Functions
 ************************************************/
void mqttCallback(char* topic, byte* message, unsigned int length);

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected");
      mqttClient.subscribe("planter01/pump");
      mqttClient.subscribe("planter01/save");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println("; retry in 5 seconds");
      delay(5000);
    }
  }
}

void publishSensorData(float soilMoisturePct, float temperature, float humidity, float lightPct, int waterPct) {
  char payload[256];
  snprintf(payload, sizeof(payload),
    "{\"soil_moisture\":%.1f,\"temperature\":%.1f,\"humidity\":%.1f,\"light_level\":%.1f,\"water_level\":%d}",
    soilMoisturePct, temperature, humidity, lightPct, waterPct);
  mqttClient.publish("planter01/sensors", payload);
  // Serial.print("Published sensor data: ");
  // Serial.println(payload);
}

void publishAlarm(const char* msg) {
  mqttClient.publish("planter01/alarm", msg);
  // Serial.print("Published alarm: ");
  // Serial.println(msg);
}

/************************************************
 * 6. Sensor Reading Functions
 ************************************************/
float readSoilMoisturePercent() {
  int rawValue = analogRead(soilMoisturePin);
  // Serial.print("Raw soil moisture value: ");
  // Serial.println(rawValue);
  float percent = map(rawValue, soilMoistureDry, soilMoistureWet, 0, 100);
  return constrain(percent, 0, 100);
}

float readTemperature() {
  float temp = dht.readTemperature();
  // Serial.print("Temperature: ");
  // Serial.println(temp);
  return temp;
}

float readHumidity() {
  float hum = dht.readHumidity();
  // Serial.print("Humidity: ");
  // Serial.println(hum);
  return hum;
}

float readLightLevelPercent() {
  int rawValue = analogRead(lightSensorPin);
  // Serial.print("Raw light level: ");
  // Serial.println(rawValue);
  float percent = map(rawValue, 0, 4095, 0, 100);
  return constrain(percent, 0, 100);
}

int readWaterLevelPercent() {
  int detectedLevel = 0;
  for (int i = 0; i < 4; i++) {
    pinMode(waterLevelPins[i], OUTPUT);
    digitalWrite(waterLevelPins[i], HIGH);
    delay(200);
    int sensorReading = analogRead(waterLevelGroundPin);
    digitalWrite(waterLevelPins[i], LOW);
    pinMode(waterLevelPins[i], INPUT);
    // Serial.print("Water sensor "); Serial.print(i); Serial.print(" reading: "); Serial.println(sensorReading);
    if (sensorReading >= waterLevelSensorThreshold) {
      detectedLevel = waterLevelAssociated[i];
      break;
    }
  }
  // Serial.print("Detected water level: ");
  // Serial.println(detectedLevel);
  return detectedLevel;
}

/************************************************
 * 7. Pump Control Logic
 ************************************************/
void handlePump(float soilMoisturePct, int waterPct) {
  Serial.print("Soil moisture (%): "); Serial.println(soilMoisturePct);
  Serial.print("Threshold (%): "); Serial.println(thresholdPct);

  if (waterPct == 0) {
    if (pumpActive) {
      pumpActive = false;
      digitalWrite(pumpPin, LOW);
      publishAlarm("pump stopped (water level 0%)");
    }
    return;
  }

  if (soilMoisturePct < thresholdPct) {
    if (!pumpActive) {
      pumpActive = true;
      pumpIntervalStart = millis();
      digitalWrite(pumpPin, HIGH);
      publishAlarm("pump activated");
    } else {
      unsigned long elapsed = millis() - pumpIntervalStart;
      if (digitalRead(pumpPin) == HIGH && elapsed >= pumpOnTime) {
        digitalWrite(pumpPin, LOW);
        pumpIntervalStart = millis();
      } else if (digitalRead(pumpPin) == LOW && elapsed >= pumpOffTime) {
        digitalWrite(pumpPin, HIGH);
        pumpIntervalStart = millis();
      }
    }
  } else {
    if (pumpActive) {
      pumpActive = false;
      digitalWrite(pumpPin, LOW);
      publishAlarm("pump stopped");
    }
  }
}

/************************************************
 * 9. Water Level LED Indicator Logic
 ************************************************/
void updateWaterLevelLED(int waterPct) {
  digitalWrite(greenLED, LOW);
  digitalWrite(blueLED, LOW);
  digitalWrite(redLED, LOW);

  if (waterPct == 100) digitalWrite(greenLED, HIGH);
  else if (waterPct >= 25 && waterPct <= 75) digitalWrite(blueLED, HIGH);
  else if (waterPct == 0) digitalWrite(redLED, HIGH);
}

/************************************************
 * 10. Setup & Main Loop
 ************************************************/
void setup() {
  Serial.begin(115200);

  delay(1000);  // Give time for Serial to initialize

  Serial.println("Smart Planter Booting...");





  dht.begin();
  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(redLED, OUTPUT);


  EEPROM.begin(512); // Reserve space
  EEPROM.get(0, thresholdPct);
  if (isnan(thresholdPct) || thresholdPct < 0 || thresholdPct > 100) {
    thresholdPct = 50.0;
  }
  Serial.print("Loaded moisture threshold (EEPROM): ");
  Serial.println(thresholdPct);

  setupWifi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
}

void loop() {
  if (!mqttClient.connected()) reconnectMQTT();
  mqttClient.loop();

  if (millis() - lastSensorPublish >= sensorPublishInterval) {
    lastSensorPublish = millis();
    float sm = readSoilMoisturePercent();
    float temp = readTemperature();
    float hum = readHumidity();
    float light = readLightLevelPercent();
    int wl = readWaterLevelPercent();

    publishSensorData(sm, temp, hum, light, wl);

    if (wl <= waterLevelPercentThreshold && !waterLevelLowAlertSent) {
      publishAlarm("water level low");
      waterLevelLowAlertSent = true;
    } else if (wl > waterLevelPercentThreshold) {
      waterLevelLowAlertSent = false;
    }

    updateWaterLevelLED(wl);
  }

  float currentMoisture = readSoilMoisturePercent();
  int currentWater = readWaterLevelPercent();
  handlePump(currentMoisture, currentWater);
  updateWaterLevelLED(currentWater);
  delay(100);
}

/************************************************
 * 11. MQTT Callback
 ************************************************/
void mqttCallback(char* topic, byte* message, unsigned int length) {
  String incoming = "";
  for (int i = 0; i < length; i++) incoming += (char)message[i];
  incoming.trim();
  incoming.toLowerCase();

  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  Serial.println(incoming);

  if (String(topic) == "planter01/pump" && incoming == "pump on") {
    float currentSoilMoisture = readSoilMoisturePercent();
    int   currentWaterLevel   = readWaterLevelPercent();
    if (currentWaterLevel == 0) {
      publishAlarm("pump stopped (water level 0%)");
      return;
    }
    if (!pumpActive) {
      pumpActive = true;
      pumpIntervalStart = millis();
      digitalWrite(pumpPin, HIGH);
      delay(5000);
      publishAlarm("pump activated (manual)");
    }
  }

  if (String(topic) == "planter01/save") {
    float newThreshold = incoming.toFloat();
    if (newThreshold >= 0 && newThreshold <= 100) {
      thresholdPct = newThreshold;
      EEPROM.put(0, thresholdPct);
      if (EEPROM.commit()) {
        Serial.print("New moisture threshold saved (EEPROM): ");
        Serial.println(thresholdPct);
        mqttClient.publish("planter01/confirm", "threshold saved");
      } else {
        Serial.println("EEPROM commit failed.");
        mqttClient.publish("planter01/confirm", "threshold saving failed");
      }
    } else {
      Serial.println("Invalid threshold value received. Must be 0-100.");
      mqttClient.publish("planter01/confirm", "threshold saving failed");
    }
  }
}
