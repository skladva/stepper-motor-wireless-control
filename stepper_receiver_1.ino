#include <WiFi.h>

// === WiFi AP Settings ===
const char* apSSID = "StepperNet";
const char* apPassword = "12345678"; // min 8 chars

WiFiServer server(5000); // TCP server on port 5000

// === Stepper Motor Pins (SAFE GPIOs for ESP32) ===
#define PUL_PIN 13   // Safe to use
#define DIR_PIN 14   // Safe to use
#define ENA_PIN 15   // Safe to use

// Motor control state
volatile bool step90 = false;
volatile bool running = false;
volatile int stepCount = 0;

unsigned long lastStepMicros = 0;
const unsigned long stepIntervalMicros = 1600; // 800us HIGH + 800us LOW

void setup() {
  Serial.begin(115200);

  pinMode(PUL_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  digitalWrite(ENA_PIN, HIGH);

  // Start Access Point
  WiFi.softAP(apSSID, apPassword);
  Serial.println("\n✅ AP Started!");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  server.begin();
  Serial.println("TCP Server running on port 5000");
}

// Non-blocking rotate 90° step sequence (200 pulses for 90°)
void setRotate90() {
  digitalWrite(DIR_PIN, HIGH);
  stepCount = 200;
  step90 = true;
}

// Non-blocking step pulse generator
void doStepperPulse() {
  static bool pulseHigh = false;
  unsigned long now = micros();
  if (now - lastStepMicros >= 800) {  // 800us HIGH or LOW
    lastStepMicros = now;
    if (pulseHigh) {
      digitalWrite(PUL_PIN, LOW);
      pulseHigh = false;
      if (step90) {
        stepCount--;
        if (stepCount <= 0) step90 = false;
      }
    } else {
      digitalWrite(PUL_PIN, HIGH);
      pulseHigh = true;
    }
    yield();
  }
}

void loop() {
  // === Handle incoming client ===
  WiFiClient client = server.available();
  if (client && client.connected() && client.available()) {
    String cmd = client.readStringUntil('\n');
    cmd.trim();
    Serial.print("📩 Received: "); Serial.println(cmd);

    if (cmd == "STEP_90") setRotate90();
    else if (cmd == "RUN_CONT") running = true;
    else if (cmd == "STOP") running = false;
  }

  // === Motor control: use non-blocking pulse generator ===
  if (step90 || running) doStepperPulse();
}
