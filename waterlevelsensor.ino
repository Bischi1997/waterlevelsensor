#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>  // Install via Library Manager if not present

// Pin Definitions
const int lowWaterLevelPin = 12;   // D6
const int highWaterLevelPin = 14;  // D5
const int relaisSwitch = 13;       // D7

bool manualOverride = false;
bool pumpState = false;

const char* ssid = "PumpController";
const char* password = "12345678";
ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);

  pinMode(lowWaterLevelPin, INPUT);
  pinMode(highWaterLevelPin, INPUT);
  pinMode(relaisSwitch, OUTPUT);
  digitalWrite(relaisSwitch, HIGH); // Pump OFF initially
  pumpState = false;

  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/toggle", handleTogglePump);
  server.on("/status", handleStatus);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();

  if (!manualOverride) {
    bool lowWater = digitalRead(lowWaterLevelPin);
    bool highWater = digitalRead(highWaterLevelPin);

    Serial.print("Low Water: ");
    Serial.println(lowWater ? "HIGH" : "LOW");
    Serial.print("High Water: ");
    Serial.println(highWater ? "HIGH" : "LOW");

    if (lowWater && highWater) {
      setPump(true);
    }

    if (!highWater) {
      setPump(false);
    }

    delay(500);
  }
}

void setPump(bool on) {
  pumpState = on;
  digitalWrite(relaisSwitch, on ? LOW : HIGH); // Active LOW
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='utf-8'>
  <title>Pump Controller</title>
  <style>
    body { font-family: sans-serif; }
    .status { font-size: 1.2em; }
    button { font-size: 1.1em; padding: 10px; }
  </style>
</head>
<body>
  <h1>Pump Controller</h1>
  <div class='status'>
    <p>Low Water Sensor: <span id='low'></span></p>
    <p>High Water Sensor: <span id='high'></span></p>
    <p>Pump State: <strong><span id='pump'></span></strong></p>
  </div>
  <p><a href='/toggle'><button>Toggle Pump</button></a></p>
  <p><small>Manual override disables automatic control until the pump is turned OFF again.</small></p>

  <script>
    function updateStatus() {
      fetch('/status')
        .then(response => response.json())
        .then(data => {
          document.getElementById('low').textContent = data.low ? 'HIGH' : 'LOW';
          document.getElementById('high').textContent = data.high ? 'HIGH' : 'LOW';
          document.getElementById('pump').textContent = data.pump ? 'ON' : 'OFF';
        });
    }
    setInterval(updateStatus, 1000); // every 1s
    updateStatus(); // initial call
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleTogglePump() {
  manualOverride = true;
  pumpState = !pumpState;
  digitalWrite(relaisSwitch, pumpState ? LOW : HIGH);

  if (!pumpState) {
    manualOverride = false;
  }

  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void handleStatus() {
  bool low = digitalRead(lowWaterLevelPin);
  bool high = digitalRead(highWaterLevelPin);

  StaticJsonDocument<200> doc;
  doc["low"] = low;
  doc["high"] = high;
  doc["pump"] = pumpState;
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}
