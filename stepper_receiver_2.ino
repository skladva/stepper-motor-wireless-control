#include <WiFi.h>
#include <esp_now.h>

// ----- Stepper Motor Pins (Safe GPIOs) -----
#define PUL_PIN 14   // Step pulse pin
#define DIR_PIN 13   // Direction pin
#define ENA_PIN 4    // Enable pin (active LOW)
#define LED_PIN 27   // LED pin

// ----- Motor settings -----
const int stepsPerRev = 400; // Steps per revolution for motor
const int microsteps = 1;    // Microstepping setting, adjust for your driver
const int stepsPer90 = stepsPerRev * microsteps / 4; // Steps per 90 degrees
const unsigned int pulseDelayUs = 8000; // Microseconds pulse width 

// ----- Control flags -----
volatile int command = 0;
volatile bool newCommand = false;
volatile bool dirAnticlockwise = false;   // NEW: Direction flag

bool running = false;  // Continuous run flag

// --- ESP-NOW receive callback ---
void OnDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  char message[20];
  if (len >= sizeof(message)) len = sizeof(message) - 1;
  memcpy(message, data, len);
  message[len] = '\0';

  // NEW: Add anticlockwise commands
  if (strcmp(message, "STEP_90") == 0) { command = 1; dirAnticlockwise = false; }
  else if (strcmp(message, "RUN_CONT") == 0) { command = 2; dirAnticlockwise = false; }
  else if (strcmp(message, "STOP") == 0) { command = 3; }
  else if (strcmp(message, "STEP_90_CCW") == 0) { command = 4; dirAnticlockwise = true; }
  else if (strcmp(message, "RUN_CONT_CCW") == 0) { command = 5; dirAnticlockwise = true; }

  // Control LED: ON for all commands except STOP, OFF for STOP
  if (command == 3) {
    digitalWrite(LED_PIN, LOW);
  } else {
    digitalWrite(LED_PIN, HIGH);
  }

  newCommand = true;
}

void setup() {
  Serial.begin(115200);

  pinMode(PUL_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(ENA_PIN, LOW);   // Enable driver (active LOW)
  digitalWrite(DIR_PIN, HIGH);  // Default direction
  digitalWrite(LED_PIN, LOW);   // LED off initially

  WiFi.mode(WIFI_STA);  // ESP-NOW requires station mode

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed");
    while(1);
  }
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("✅ ESP-NOW Receiver ready");
  Serial.print("MAC Address: "); Serial.println(WiFi.macAddress());
}

// Function to perform blocking 90 degree rotation
void rotate90Blocking(bool ccw = false) {
  digitalWrite(DIR_PIN, ccw ? LOW : HIGH);  // Set rotation direction
  Serial.print("Rotating 90 degrees ");
  Serial.println(ccw ? "anticlockwise..." : "clockwise...");
  for (int i = 0; i < stepsPer90; i++) {
    digitalWrite(PUL_PIN, HIGH);
    delayMicroseconds(pulseDelayUs);
    digitalWrite(PUL_PIN, LOW);
    delayMicroseconds(pulseDelayUs);
  }
  Serial.println("Rotation complete.");
}

// Function to generate continuous pulses for continuous run mode
void continuousRunFunction() {
  digitalWrite(PUL_PIN, HIGH);
  delayMicroseconds(pulseDelayUs);
  digitalWrite(PUL_PIN, LOW);
  delayMicroseconds(pulseDelayUs);
}

void loop() {
  if (newCommand) {
    newCommand = false;
    switch(command) {
      case 1: // STEP_90 (CW)
        running = false;
        rotate90Blocking(false);
        break;
      case 2: // RUN_CONT (CW)
        Serial.println("Starting continuous run CW");
        running = true;
        digitalWrite(DIR_PIN, HIGH);
        break;
      case 3: // STOP
        Serial.println("Stopping motor");
        running = false;
        break;
      case 4: // STEP_90_CCW
        running = false;
        rotate90Blocking(true);
        break;
      case 5: // RUN_CONT_CCW
        Serial.println("Starting continuous run CCW");
        running = true;
        digitalWrite(DIR_PIN, LOW); // CCW
        break;
    }
  }

  if (running) {
    continuousRunFunction();
  }

  yield();
}
