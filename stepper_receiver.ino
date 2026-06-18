#include <WiFi.h>
#include <esp_now.h>

// Motor pins
#define PUL_PIN 14
#define DIR_PIN 13
#define ENA_PIN 4
#define RELAY_PIN 26

// LED for button state indication
#define LED_PIN 27

// Motor parameters
const int stepsPerRev = 400;
const int microsteps = 1;
const int stepsPer90 = stepsPerRev * microsteps / 4;
const unsigned int pulseDelayUs = 8000;

volatile int command = 0;
volatile bool newCommand = false;
volatile bool dirAnticlockwise = false;

volatile bool power = true;

bool running = false;

// ESP-NOW receive callback
void OnDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  String message = String((char*)data).substring(0, len);

  // Control LED based on button press messages
  if (message == "LED_ON" && power == true) {
    digitalWrite(LED_PIN, HIGH);
  } else if (message == "LED_OFF") {
    digitalWrite(LED_PIN, LOW);
  } else if (message == "POWER_ON") {
    digitalWrite(RELAY_PIN, LOW);
    power = true;
    Serial.println("POWER_ON");
  } else if (message == "POWER_OFF") {
    digitalWrite(RELAY_PIN, HIGH);
    power = false;
    Serial.println("POWER_OFF");
  }

  // Handle motor commands as before
  if (message == "STEP_90") { command = 1; dirAnticlockwise = false; }
  else if (message == "RUN_CONT") { command = 2; dirAnticlockwise = false; }
  else if (message == "STOP") { command = 3; }
  else if (message == "STEP_90_CCW") { command = 4; dirAnticlockwise = true; }
  else if (message == "RUN_CONT_CCW") { command = 5; dirAnticlockwise = true; }

  if (message == "STEP_90" || message == "RUN_CONT" || message == "STEP_90_CCW" || message == "RUN_CONT_CCW" || message == "STOP") {
    newCommand = true;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PUL_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(ENA_PIN, LOW);  // Enable motor driver (active LOW)
  digitalWrite(LED_PIN, LOW);  // LED off initially
  digitalWrite(DIR_PIN, HIGH); // Default direction

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while(true);
  }
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Receiver ready");
  Serial.print("MAC Address: "); Serial.println(WiFi.macAddress());
}

// Perform blocking 90 degree rotation
void rotate90Blocking(bool ccw = false) {
  digitalWrite(DIR_PIN, ccw ? LOW : HIGH);
  Serial.print("Rotating 90 degrees ");
  Serial.println(ccw ? "anticlockwise" : "clockwise");
  for (int i = 0; i < stepsPer90; i++) {
    digitalWrite(PUL_PIN, HIGH);
    delayMicroseconds(pulseDelayUs);
    digitalWrite(PUL_PIN, LOW);
    delayMicroseconds(pulseDelayUs);
  }
  Serial.println("Rotation complete.");
}

// Continuous motor run pulse generation
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
        digitalWrite(DIR_PIN, LOW);
        break;
    }
  }

  if (running) {
    continuousRunFunction();
  }

  yield();
}
