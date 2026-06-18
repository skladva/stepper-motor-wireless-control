#include <esp_now.h>
#include <WiFi.h>

// Use safer pins for ESP32 (avoid boot-pin conflicts)
#define BUTTON1 18
#define BUTTON2 15
#define BUTTON3 19 // Latching button 

uint8_t receiverAddress[] = {0xCC, 0x50, 0xE3, 0x85, 0x70, 0x7C}; // Replace with your receiver's MAC address

// --- Button1 (STEP_90 or RUN_CONT) ---
static unsigned long pressStart = 0;
static bool isRunning = false;
// --- Button2 (STEP_90_CCW or RUN_CONT_CCW) ---
static unsigned long pressStart2 = 0;
static bool isRunning2 = false;

// For debouncing Button3 interrupt
volatile unsigned long lastInterruptTime = 0;

bool powerStatus = true;

void sendCommand(const char *msg) {
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)msg, strlen(msg));
  if (result == ESP_OK)
    Serial.println(String("Sent: ") + msg);
    //int a = 1;
  else
    Serial.println("Error sending command!");
}

volatile bool button3Event = false;
volatile int button3StateFlag = HIGH; // store last read state

void IRAM_ATTR button3ISR() {
  unsigned long interruptTime = millis();
  static unsigned long lastInterruptTime = 0;
  if (interruptTime - lastInterruptTime > 200) {
    // only record event; do NOT call esp_now_send or Serial here
    button3StateFlag = digitalRead(BUTTON3); // safe quick read
    button3Event = true;
    lastInterruptTime = interruptTime;
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(BUTTON3, INPUT_PULLUP);

  // Attach interrupt to button3 on both rising and falling edge
  attachInterrupt(digitalPinToInterrupt(BUTTON3), button3ISR, CHANGE);

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



void loop() {
  static unsigned long lastSendLedTime = 0;
  const unsigned long ledSendInterval = 200;

  int buttonState = digitalRead(BUTTON1);
  int button2State = digitalRead(BUTTON2);
  int button3State = digitalRead(BUTTON3);
  
  if (powerStatus){
  // Button 1 pressed (falling edge detection)
  if (buttonState == HIGH && pressStart == 0) {
    pressStart = millis();  // start timing
  }

  // Button 1 held down
  if (buttonState == HIGH && pressStart > 0 && !isRunning) {
    if (millis() - pressStart >= 400) {  // pressed > 2 seconds
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

  // Button 2 logic for ANTICLOCKWISE
  if (button2State == HIGH && pressStart2 == 0) {
    pressStart2 = millis(); // start timing
  }

  if (button2State == HIGH && pressStart2 > 0 && !isRunning2) {
    if (millis() - pressStart2 >= 400) { // held > 2 seconds
      sendCommand("RUN_CONT_CCW");
      isRunning2 = true;
    }
  }

  if (button2State == LOW && pressStart2 > 0) {
    if (isRunning2) {
      sendCommand("STOP");
      isRunning2 = false;
    } else if (millis() - pressStart2 < 4000) {
      sendCommand("STEP_90_CCW");
    }
    pressStart2 = 0;
  }

  // Button press feedback LED command sent every 200 ms
  if (millis() - lastSendLedTime > ledSendInterval) {
    lastSendLedTime = millis();
    if (button2State == HIGH || (buttonState == HIGH && pressStart > 0) || pressStart2 > 0) {
      sendCommand("LED_ON");
    } else {
      sendCommand("LED_OFF");
    }
  }
}
    // if (button3State == HIGH){
  //   sendCommand("POWER_ON");
  // } else {
  //   sendCommand("POWER_OFF");
  // }

  if (button3Event) {
  button3Event = false;
  if (button3StateFlag == LOW) {  // INPUT_PULLUP: LOW = pressed
    sendCommand("POWER_ON");
    Serial.println("Power ON (from loop)");
    powerStatus = true;
  } else {
    sendCommand("POWER_OFF");
    Serial.println("Power OFF (from loop)");
    powerStatus = false;
  }
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
