#include <esp_now.h>
#include <WiFi.h>

// Use safer pins for ESP32 (avoid boot-pin conflicts)
#define BUTTON1 18
#define BUTTON2 15

uint8_t receiverAddress[] = {0xCC, 0x50, 0xE3, 0x85, 0x70, 0x7C}; // Replace with your receiver's MAC address

// --- Button1 (STEP_90 or RUN_CONT) ---
static unsigned long pressStart = 0;
static bool isRunning = false;
// --- Button2 (STEP_90_CCW or RUN_CONT_CCW) ---
static unsigned long pressStart2 = 0;
static bool isRunning2 = false;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer!");
    return;
  }

  Serial.println("\n--- ESP-NOW Remote ---");
  Serial.println("Send commands:");
  Serial.println("1 -> 90° step");
  Serial.println("2 -> Continuous run");
  Serial.println("3 -> Stop\n");
}

void sendCommand(const char *msg) {
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)msg, strlen(msg));
  if (result == ESP_OK)
    Serial.println(String("Sent: ") + msg);
  else
    Serial.println("Error sending command!");
}

void loop() {
  static bool button2Pressed = false;
  static unsigned long lastPressTime = 0;
  const unsigned long debounceDelay = 200;

  int buttonState = digitalRead(BUTTON1);
  int button2State = digitalRead(BUTTON2);

  // Button 1 pressed (falling edge detection)
  if (buttonState == HIGH && pressStart == 0) {
    pressStart = millis();  // start timing
  }

  // Button 1 held down
  if (buttonState == HIGH && pressStart > 0 && !isRunning) {
    if (millis() - pressStart >= 2000) {  // pressed > 2 seconds
      sendCommand("RUN_CONT");
      isRunning = true;
    }
  }

  // Button 1 released
  if (buttonState == LOW && pressStart > 0) {
    if (isRunning) {
      sendCommand("STOP");
      isRunning = false;
    } else if (millis() - pressStart < 4000) {
      sendCommand("STEP_90");
    }
    pressStart = 0;
  }

  // ----------- Button 2 logic for ANTICLOCKWISE ---------------
  if (button2State == HIGH && pressStart2 == 0) {
    pressStart2 = millis(); // start timing
  }

  if (button2State == HIGH && pressStart2 > 0 && !isRunning2) {
    if (millis() - pressStart2 >= 2000) { // held > 2 seconds
      sendCommand("RUN_CONT_CCW"); // <--- New command for CCW
      isRunning2 = true;
    }
  }

  if (button2State == LOW && pressStart2 > 0) {
    if (isRunning2) {
      sendCommand("STOP");
      isRunning2 = false;
    } else if (millis() - pressStart2 < 4000) {
      sendCommand("STEP_90_CCW"); // <--- New command for CCW
    }
    pressStart2 = 0;
  }

  // --- Serial commands ---
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == '1') sendCommand("STEP_90");
    else if (cmd == '2') sendCommand("RUN_CONT");
    else if (cmd == '3') sendCommand("STOP");
    else if (cmd == '4') sendCommand("STEP_90_CCW");
    else if (cmd == '5') sendCommand("RUN_CONT_CCW");
  }
}
