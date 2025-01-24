#include <DHT.h>
#include "VOneMqttClient.h"

#define DHTPIN 42
#define PIRPIN 4
#define RELAYPIN 39
#define DHTTYPE DHT11
#define TEMP_THRESHOLD 25.0 

// Device IDs for V-One
const char* DHT11Sensor = "a0cf26f8-43e3-4a45-9b99-8aab4298257d"; 
const char* PIR = "fa5466e0-13b5-4ee6-acbc-4d7ba35c201f";
const char* Fan = "a54a6bb2-b672-4af1-b30a-54f9810890d9";
const char* Relay = "92495ed6-7e97-4de0-8df6-0e7b67de9a85";

// Wi-Fi credentials
// const char* WIFI_SSID = "Moon";
// const char* WIFI_PASSWORD = "fatihaharis";

// Create an instance of VOneMqttClient
VOneMqttClient voneClient;

// last message time
unsigned long lastMsgTime = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  setup_wifi(),
  voneClient.setup();
  Serial.begin(115200);
  dht.begin();
  pinMode(PIRPIN, INPUT);
  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, LOW);
  Serial.println("Timestamp,Temperature,Humidity,Motion,FanStatus,BusStop");
}

void loop() {
  // Reconnect to the V-One using MQTT if failed
  if (!voneClient.connected()) {
    voneClient.reconnect();
    Serial.println("Sensor Fail");
    voneClient.publishDeviceStatusEvent(DHT11Sensor, true);
    voneClient.publishDeviceStatusEvent(PIR, true);
    voneClient.publishDeviceStatusEvent(Fan, true);
    voneClient.publishDeviceStatusEvent(Relay, true);
  }
  voneClient.loop();

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  JSONVar payloadObject;
  payloadObject["Humidity"] = humidity;
  payloadObject["Temperature"] = temperature;
  voneClient.publishTelemetryData(DHT11Sensor, payloadObject);

  int motionDetected = digitalRead(PIRPIN);
  voneClient.publishTelemetryData(PIR, "Motion", motionDetected);
  String fanStatus = "OFF";
  String busStop = "Bus Stop 1"; // Update manually for each location
  unsigned long timestamp = millis() / 1000;

  if (!isnan(temperature) && !isnan(humidity)) {
    if (temperature > TEMP_THRESHOLD && motionDetected == HIGH) {
      digitalWrite(RELAYPIN, HIGH); // Fan ON
      fanStatus = "ON";
    } else {
      digitalWrite(RELAYPIN, LOW); // Fan OFF
      fanStatus = "OFF";
    }

    // Publish Fan status to V-One
    voneClient.publishTelemetryData(Fan, "FanStatus", fanStatus.c_str());

    // Publish Relay status to V-One (0 for OFF, 1 for ON)
    int relayStatus = digitalRead(RELAYPIN);
    voneClient.publishTelemetryData(Relay, "RelayStatus", relayStatus);

    Serial.print(timestamp);
    Serial.print(",");
    Serial.print(temperature);
    Serial.print(",");
    Serial.print(humidity);
    Serial.print(",");
    Serial.print(motionDetected ? "Yes" : "No");
    Serial.print(",");
    Serial.print(fanStatus);
    Serial.print(",");
    Serial.println(busStop);
  } else {
    Serial.println("Failed to read from DHT sensor!");
  }

  delay(5000);
}
