#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int moisturePin = A0;
const int relayPin = D5; // GPIO14
const int manualSwitchPin = D2; // GPIO4
const int moistureThreshold = 512; // 50% of 1023

const char* ssid = "SSID";
const char* password = "PASS";

ESP8266WebServer server(80);

bool manualMode = false;

void setup() {
  pinMode(relayPin, OUTPUT);
  pinMode(manualSwitchPin, INPUT_PULLUP);

  Serial.begin(115200);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Correct initialization
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.display();
  delay(2000);
  display.clearDisplay();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Serve the dashboard page
  server.on("/", HTTP_GET, []() {
    const char* html = "<!DOCTYPE html>"
                        "<html>"
                        "<head>"
                        "<meta charset=\"UTF-8\">"
                        "<title>Smart Irrigation Dashboard</title>"
                        "<style>"
                        "body {"
                        "  font-family: Arial, sans-serif;"
                        "  text-align: center;"
                        "  background-color: #f0f0f0;"
                        "  color: #333;"
                        "}"
                        ".container {"
                        "  width: 80%;"
                        "  margin: 0 auto;"
                        "  padding: 20px;"
                        "  background-color: #fff;"
                        "  border-radius: 8px;"
                        "  box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);"
                        "}"
                        "h1 {"
                        "  color: #4CAF50;"
                        "}"
                        ".data {"
                        "  font-size: 1.5em;"
                        "}"
                        ".button {"
                        "  background-color: #4CAF50;"
                        "  color: white;"
                        "  padding: 10px 20px;"
                        "  border: none;"
                        "  border-radius: 5px;"
                        "  cursor: pointer;"
                        "  font-size: 1em;"
                        "}"
                        ".button.off {"
                        "  background-color: #f44336;"
                        "}"
                        "</style>"
                        "</head>"
                        "<body>"
                        "<div class=\"container\">"
                        "<h1>Smart Irrigation Dashboard</h1>"
                        "<div class=\"data\">"
                        "<p id=\"moisture\">Soil Moisture: Loading...</p>"
                        "<p id=\"relay\">Relay State: Loading...</p>"
                        "</div>"
                        "<button id=\"manualButton\" class=\"button\">Manual Mode</button>"
                        "</div>"
                        "<script>"
                        "const manualButton = document.getElementById('manualButton');"
                        "const moistureElement = document.getElementById('moisture');"
                        "const relayElement = document.getElementById('relay');"
                        "function updateData() {"
                        "  fetch('/status')"
                        "    .then(response => response.json())"
                        "    .then(data => {"
                        "      moistureElement.textContent = `Soil Moisture: ${data.moisture}`;"
                        "      relayElement.textContent = `Relay State: ${data.relay ? 'ON' : 'OFF'}`;"
                        "    });"
                        "}"
                        "manualButton.addEventListener('click', () => {"
                        "  fetch('/toggleManual')"
                        "    .then(response => response.json())"
                        "    .then(data => {"
                        "      manualButton.textContent = data.manual ? 'Manual Mode' : 'Automatic Mode';"
                        "      manualButton.classList.toggle('off', !data.manual);"
                        "    });"
                        "});"
                        "setInterval(updateData, 1000);"
                        "</script>"
                        "</body>"
                        "</html>";
    server.send(200, "text/html", html);
  });

  // Serve status data
  server.on("/status", HTTP_GET, []() {
    int soilMoisture = analogRead(moisturePin);
    bool relayState = digitalRead(relayPin) == HIGH;
    String json = "{\"moisture\": " + String(soilMoisture) + ", \"relay\": " + (relayState ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  // Toggle manual mode
  server.on("/toggleManual", HTTP_GET, []() {
    manualMode = !manualMode;
    digitalWrite(relayPin, manualMode ? HIGH : LOW);
    String json = "{\"manual\": " + String(manualMode ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  server.handleClient();

  int soilMoisture = analogRead(moisturePin);
  bool manualSwitchState = !digitalRead(manualSwitchPin);

  if (manualSwitchState) {
    manualMode = true;
  }

  if (manualMode) {
    digitalWrite(relayPin, HIGH);
  } else {
    if (soilMoisture < moistureThreshold) {
      digitalWrite(relayPin, HIGH);
    } else {
      digitalWrite(relayPin, LOW);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Soil Moisture: ");
  display.println(soilMoisture);
  display.print("Relay State: ");
  display.println(digitalRead(relayPin) == HIGH ? "ON" : "OFF");
  display.display();

  delay(1000);
}
