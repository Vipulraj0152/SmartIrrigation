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
const int manualSwitchPin = D3; // GPIO4
const int moistureThreshold = 512; // 50% of 1023

const char* ssid = "SSID";
const char* password = "Password";

ESP8266WebServer server(80);
String ipAddress = "";

bool manualMode = false;
bool motorState = false;

void setup() {
  pinMode(relayPin, OUTPUT);
  pinMode(moisturePin,INPUT);
  pinMode(manualSwitchPin, INPUT_PULLUP);

  Serial.begin(115200);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
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

  ipAddress = WiFi.localIP().toString(); // Get IP address
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(ipAddress);

  // Serve the dashboard page
  server.on("/", HTTP_GET, []() {
    const char* html = R"(
      <!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Smart Irrigation Dashboard</title>
  <style>
    body {
      font-family: 'Arial', sans-serif;
      margin: 0;
      padding: 0;
      display: flex;
      justify-content: center;
      align-items: center;
      background: #2c3e50; /* Dark blue-gray background */
      color: #ecf0f1; /* Light text color */
      overflow: hidden;
    }

    .container {
      width: 90vw; /* Full width with some margin */
      max-width: 600px; /* Maximum width for larger screens */
      background: #34495e; /* Dark gray background for the container */
      border-radius: 20px; /* Rounded corners */
      box-shadow: 0 8px 16px rgba(0, 0, 0, 0.4); /* Softer shadow */
      padding: 30px;
      box-sizing: border-box;
      text-align: center;
      position: relative; /* For positioning the plant graphic */
      display: flex;
      flex-direction: column;
      min-height: 100vh; /* Ensure full height of the viewport */
    }

    h1 {
      color: #2ecc71; /* Vibrant green color */
      font-size: 2.5em; /* Larger heading */
      margin-bottom: 20px;
      font-weight: bold;
    }

    .data {
      display: flex;
      justify-content: space-around;
      margin-bottom: 30px;
    }

    .data > div {
      background: #1abc9c; /* Teal background for data sections */
      border-radius: 15px; /* Rounded corners */
      padding: 20px;
      flex: 1;
      margin: 0 10px;
    }

    .data p {
      margin: 0;
      font-size: 1.3em; /* Larger font size for data text */
      font-weight: bold;
    }

    .button {
      background-color: #e67e22; /* Vibrant orange for buttons */
      color: #fff;
      padding: 15px 30px; /* Increased padding */
      border: none;
      border-radius: 12px; /* Rounded corners */
      cursor: pointer;
      font-size: 1.2em; /* Larger font size */
      transition: background-color 0.3s ease, transform 0.2s ease; /* Smooth transitions */
      margin: 10px; /* Margin between buttons */
      display: inline-block; /* Display buttons inline */
    }

    .button:hover {
      background-color: #d35400; /* Darker orange on hover */
      transform: scale(1.05); /* Slightly enlarge on hover */
    }

    .button.off {
      background-color: #c0392b; /* Red for 'off' state */
    }

    .button.off:hover {
      background-color: #e74c3c; /* Darker red on hover */
    }

    .plant-graphic {
      position: absolute;
      bottom: -40px;
      right: -40px;
      width: 150px;
      height: auto;
      opacity: 0.4; /* Slightly transparent */
      pointer-events: none; /* Ensure it doesn’t interfere with interactions */
    }

    .footer {
      margin-top: auto; /* Push the footer to the bottom */
      padding: 10px;
      background: #2c3e50; /* Same as body background */
      color: #95a5a6; /* Light gray text color */
      font-size: 0.9em; /* Smaller font size */
    }

    .footer p {
      margin: 0;
    }

    @media (max-width: 600px) {
      .container {
        width: 95vw; /* Full width on small screens */
      }

      h1 {
        font-size: 2em; /* Adjust heading size on small screens */
      }

      .data {
        flex-direction: column; /* Stack data sections vertically on small screens */
      }

      .data > div {
        margin: 10px 0; /* Margin between stacked sections */
      }

      .button {
        font-size: 1em; /* Adjust font size on small screens */
        padding: 12px 24px; /* Adjust padding on small screens */
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Smart Irrigation Dashboard</h1>
    <div class="data">
      <div>
        <p id="moisture">Soil Moisture: Loading...</p>
      </div>
      <div>
        <p id="relay">Relay State: Loading...</p>
      </div>
    </div>
    <button id="manualButton" class="button">Manual Mode</button>
    <button id="motorButton" class="button">Turn Motor ON</button>
    <img src="https://png.pngtree.com/png-vector/20240207/ourmid/pngtree-indoor-plant-flowerpot-png-image_11669796.png" alt="Plant Graphic" class="plant-graphic"> <!-- Add your plant graphic here -->
    <footer class="footer">
      <p>Mini Project by Srishti and Shreya</p>
    </footer>
  </div>
  <script>
    const manualButton = document.getElementById('manualButton');
    const motorButton = document.getElementById('motorButton');
    const moistureElement = document.getElementById('moisture');
    const relayElement = document.getElementById('relay');

    function updateData() {
      fetch('/status')
        .then(response => response.json())
        .then(data => {
          moistureElement.textContent = `Soil Moisture: ${data.moisture}`;
          relayElement.textContent = `Relay State: ${data.relay ? 'ON' : 'OFF'}`;
          motorButton.textContent = data.relay ? 'Turn Motor OFF' : 'Turn Motor ON';
          manualButton.textContent = data.manual ? 'Manual Mode' : 'Automatic Mode';
          manualButton.classList.toggle('off', !data.manual);
        });
    }

    manualButton.addEventListener('click', () => {
      fetch('/toggleManual')
        .then(response => response.json())
        .then(data => {
          manualButton.textContent = data.manual ? 'Manual Mode' : 'Automatic Mode';
          manualButton.classList.toggle('off', !data.manual);
        });
    });

    motorButton.addEventListener('click', () => {
      fetch('/toggleMotor')
        .then(response => response.json())
        .then(data => {
          motorButton.textContent = data.motor ? 'Turn Motor OFF' : 'Turn Motor ON';
          motorButton.classList.toggle('off', data.motor);
        });
    });

    setInterval(updateData, 5000); // Update data every 5 seconds
  </script>
</body>
</html>
)";
    server.send(200, "text/html", html);
  });

  // Serve status data
  server.on("/status", HTTP_GET, []() {
    int soilMoisture = analogRead(moisturePin);
    String json = "{\"moisture\": " + String(soilMoisture) + ", \"relay\": " + (motorState ? "true" : "false") + ", \"manual\": " + (manualMode ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  // Toggle manual mode
  server.on("/toggleManual", HTTP_GET, []() {
    manualMode = !manualMode;
    if (!manualMode) {
      motorState = false;  // Ensure the motor is turned off when switching to automatic
    }
    digitalWrite(relayPin, motorState ? HIGH : LOW);
    String json = "{\"manual\": " + String(manualMode ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  // Toggle motor manually
  server.on("/toggleMotor", HTTP_GET, []() {
    motorState = !motorState;  // Toggle the motor state
    digitalWrite(relayPin, motorState ? HIGH : LOW);  // Update the relay
    String json = "{\"motor\": " + String(motorState ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  server.handleClient();

  int soilMoisture = analogRead(moisturePin);
  bool manualSwitchState = !digitalRead(manualSwitchPin);

  // If manual switch is pressed, enter manual mode
  if (manualSwitchState) {
    manualMode = true;
  }

  // Automatic mode control
  if (!manualMode) {
    if (soilMoisture < moistureThreshold) {
      motorState = true;
    } else {
      motorState = false;
    }
  }

  digitalWrite(relayPin, motorState ? LOW : HIGH);

  // Update the OLED display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("IP : ");
  display.println(ipAddress);
  display.println("---------------------");
  display.print("Soil Moisture : ");
  display.println(soilMoisture);
  display.println("Temp: 28c || Hum: 12%");
  display.print("    Motor is ");
  display.println(motorState ? "ON" : "OFF");
  display.println("---------------------");
  display.println("By : Srishti Kumari");
  display.println("   : Shreya M B");
  display.display();

  

  delay(1000);
}
