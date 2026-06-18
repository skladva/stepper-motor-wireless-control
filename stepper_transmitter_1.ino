#include <WiFi.h>

#define BUTTON1 2
#define BUTTON2 3

// === WiFi Client Settings ===
const char* hostSSID = "StepperNet";     // AP ESP32 SSID
const char* hostPassword = "12345678";   // AP password
const char* hostIP = "192.168.4.1";      // Fixed IP of Host ESP
const uint16_t hostPort = 5000;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);

  // Connect to Host ESP AP
  WiFi.begin(hostSSID, hostPassword);
  Serial.print("Connecting to host ESP...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to host ESP");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());
}

void sendCommand(const String &cmd) {
  if (!client.connected()) {
    if (!client.connect(hostIP, hostPort)) {
      Serial.println("❌ Failed to connect to host");
      return;
    }
  }
  client.println(cmd);
  Serial.print("📤 Sent: "); Serial.println(cmd);
}

void loop() {
  static bool button2Pressed = false;

  // --- Button 1: 90° step ---
  if (digitalRead(BUTTON1) == LOW) {
    sendCommand("STEP_90");
    delay(300); // debounce
  }

  // --- Button 2: Continuous run ---
  if (digitalRead(BUTTON2) == LOW && !button2Pressed) {
    sendCommand("RUN_CONT");
    button2Pressed = true;
  } else if (digitalRead(BUTTON2) == HIGH && button2Pressed) {
    sendCommand("STOP");
    button2Pressed = false;
  }

  // --- Serial input control ---
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == '1') sendCommand("STEP_90");
    else if (cmd == '2') sendCommand("RUN_CONT");
    else if (cmd == '3') sendCommand("STOP");
  }

  delay(10); // small delay to allow WiFi tasks
}
