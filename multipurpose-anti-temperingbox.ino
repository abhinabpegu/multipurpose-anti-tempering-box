/*
  Smart Medicine / Vaccine Box
  Board: Arduino UNO R3
  Libraries: MFRC522 (Miguel Balboa), DHT sensor library (Adafruit),
             Adafruit Unified Sensor. Servo is built in.
  Serial Monitor: 9600 baud
*/

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <DHT.h>

// ---------------- PINS ----------------
#define DHT_PIN      2
#define HALL_PIN     3
#define SERVO_PIN    6
#define RST_PIN      9
#define SS_PIN       10   // MOSI 11, MISO 12, SCK 13 are fixed SPI pins

const byte TEMP_LEDS[4] = {A1, 7, 5, 8};  // blue, yellow, yellow, red
#define LED_HUMIDITY  A4                  // red
#define LED_ACCESS    4                   // green
#define LED_TAMPER    A2                  // red
#define LED_OK        A3                  // green

// ---------------- SETTINGS ----------------
// Scan a card, copy its UID from the Serial Monitor, paste it here.
const byte AUTH_CARDS[][4] = {
  {0xCA, 0x9F, 0x3F, 0x1D},   // <-- REPLACE with your card UID CA 9F 3F 1D
  // {0x12, 0x34, 0x56, 0x78}, // optional second card
};
const byte NUM_CARDS = sizeof(AUTH_CARDS) / sizeof(AUTH_CARDS[0]);

// Hall level when the magnet is near (lid closed). Change to HIGH if reversed.
const int LID_CLOSED_LEVEL = LOW;

const int LOCKED_ANGLE   = 120;
const int UNLOCKED_ANGLE = 0;

const float TEMP_LEVELS[4] = {26.0, 28.0, 30.0, 32.0};  // bar graph LEDs 1..4 (C)
const float OVER_TEMP      = 35.0;                      // all temp LEDs blink at/above this
const float OVER_HUMIDITY  = 80.0;     // % RH, humidity LED on at/above this
const float COOL_TEMP = 26.0;   // blue LED blinks below this temperature                 
const unsigned long UNLOCK_TIMEOUT   = 10000;  // re-lock if lid not opened
const unsigned long RELOCK_DELAY     = 2000;   // re-lock after lid closed again
const unsigned long LEFT_OPEN_WARN   = 30000;  // fast blink if lid open this long
const unsigned long LED_DELAY        = 1000;   // wait after servo moves before green LEDs
const unsigned long DHT_INTERVAL     = 2000;
const unsigned long HALL_DEBOUNCE_MS = 50;

// ---------------- OBJECTS & STATE ----------------
MFRC522 rfid(SS_PIN, RST_PIN);
Servo latch;
DHT dht(DHT_PIN, DHT11);

bool unlocked = false;
bool tamper = false;
bool lidClosed = true;
bool lidOpenedSinceUnlock = false;
unsigned long unlockTime = 0;
unsigned long unlockDoneTime = 0;
unsigned long lockDoneTime = 0;
unsigned long lidClosedSince = 0;
unsigned long lidOpenSince = 0;

int lastRawHall = HIGH;
unsigned long hallChangeTime = 0;

float temperature = 0, humidity = 0;
bool haveReading = false;
unsigned long lastDht = 0;

// ---------------- HELPERS ----------------
void logMsg(const char *msg) {
  Serial.print("[");
  Serial.print(millis() / 1000);
  Serial.print("s] ");
  Serial.println(msg);
}

void moveServo(int angle) {
  latch.attach(SERVO_PIN);
  latch.write(angle);
  delay(500);
  latch.detach();   // stops jitter and saves power
}

void lockBox() {
  moveServo(LOCKED_ANGLE);
  lockDoneTime = millis();
  unlocked = false;
  lidOpenedSinceUnlock = false;
  lidClosedSince = 0;
  logMsg("LOCKED");
}

void unlockBox() {
  moveServo(UNLOCKED_ANGLE);
  unlockDoneTime = millis();
  unlocked = true;
  unlockTime = millis();
  lidOpenedSinceUnlock = false;
  lidClosedSince = 0;
  logMsg("UNLOCKED");
}

bool isAuthorized() {
  if (rfid.uid.size != 4) return false;
  for (byte c = 0; c < NUM_CARDS; c++) {
    bool match = true;
    for (byte i = 0; i < 4; i++) {
      if (rfid.uid.uidByte[i] != AUTH_CARDS[c][i]) { match = false; break; }
    }
    if (match) return true;
  }
  return false;
}

void flashTamper3() {
  for (byte i = 0; i < 3; i++) {
    digitalWrite(LED_TAMPER, HIGH); delay(120);
    digitalWrite(LED_TAMPER, LOW);  delay(120);
  }
}

// ---------------- INPUTS ----------------
void updateLid() {
  int raw = digitalRead(HALL_PIN);
  if (raw != lastRawHall) {
    lastRawHall = raw;
    hallChangeTime = millis();
  }
  if (millis() - hallChangeTime >= HALL_DEBOUNCE_MS) {
    bool closedNow = (raw == LID_CLOSED_LEVEL);
    if (closedNow != lidClosed) {
      lidClosed = closedNow;
      if (lidClosed) { lidOpenSince = 0; logMsg("Lid CLOSED"); }
      else           { lidOpenSince = millis(); logMsg("Lid OPEN"); }
    }
  }
}

void handleCard() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  Serial.print("Card UID:");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();

  if (isAuthorized()) {
    logMsg("Access GRANTED");
    if (tamper) { tamper = false; logMsg("Tamper alarm cleared"); }
    if (!unlocked) unlockBox();
    else unlockTime = millis();
  } else {
    logMsg("Access DENIED (unknown card)");
    flashTamper3();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void readClimate() {
  if (millis() - lastDht < DHT_INTERVAL) return;
  lastDht = millis();
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    logMsg("DHT11 read failed (check wiring)");
    return;
  }
  temperature = t;
  humidity = h;
  haveReading = true;
  Serial.print("Temp: "); Serial.print(t, 1);
  Serial.print(" C  Humidity: "); Serial.print(h, 0); Serial.println(" %");
}

// ---------------- LOGIC ----------------
void updateLockLogic() {
  // Forced open while locked = tamper
  if (!unlocked && !lidClosed && !tamper) {
    tamper = true;
    logMsg("TAMPER! Lid opened without a valid card");
  }

  if (unlocked) {
    if (!lidClosed) {
      lidOpenedSinceUnlock = true;
      lidClosedSince = 0;
    } else if (lidOpenedSinceUnlock) {
      if (lidClosedSince == 0) lidClosedSince = millis();
      if (millis() - lidClosedSince >= RELOCK_DELAY) lockBox();
    } else if (millis() - unlockTime >= UNLOCK_TIMEOUT) {
      lockBox();
    }
  }
}

// ---------------- OUTPUTS ----------------
void updateLeds() {
  bool blink = (millis() / 400) % 2;
  bool overTemp = haveReading && temperature >= OVER_TEMP;

  // Temperature bar (all 4 blink when over temperature)
    // Temperature bar (all 4 blink when over temperature; blue blinks when cool)
  for (byte i = 0; i < 4; i++) {
    if (overTemp) {
      digitalWrite(TEMP_LEDS[i], blink);
    } else if (i == 0 && haveReading && temperature < COOL_TEMP) {
      digitalWrite(TEMP_LEDS[i], blink);   // blue: below 26 C, blink
    } else {
      digitalWrite(TEMP_LEDS[i], haveReading && temperature >= TEMP_LEVELS[i]);
    }
  }
  // Humidity and tamper
  digitalWrite(LED_HUMIDITY, haveReading && humidity >= OVER_HUMIDITY);
  digitalWrite(LED_TAMPER, tamper);

  // Access LED: off when locked; 1 s after unlocking it turns on.
  // Steady while lid is closed, blinks while lid is open.
  bool accessReady = unlocked && (millis() - unlockDoneTime >= LED_DELAY);
  if (accessReady) {
    if (!lidClosed) {
      bool leftOpen = lidOpenSince != 0 && (millis() - lidOpenSince > LEFT_OPEN_WARN);
      bool fastBlink = (millis() / 120) % 2;
      digitalWrite(LED_ACCESS, leftOpen ? fastBlink : blink);
    } else {
      digitalWrite(LED_ACCESS, HIGH);
    }
  } else {
    digitalWrite(LED_ACCESS, LOW);
  }

  // All OK LED: 1 s after locking, if lid closed and no alarms
  bool okReady = !unlocked && (millis() - lockDoneTime >= LED_DELAY);
  digitalWrite(LED_OK, okReady && lidClosed && !tamper && !overTemp);
}

// ---------------- SETUP / LOOP ----------------
void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  dht.begin();
  pinMode(HALL_PIN, INPUT_PULLUP);

  for (byte i = 0; i < 4; i++) pinMode(TEMP_LEDS[i], OUTPUT);
  pinMode(LED_HUMIDITY, OUTPUT);
  pinMode(LED_ACCESS, OUTPUT);
  pinMode(LED_TAMPER, OUTPUT);
  pinMode(LED_OK, OUTPUT);

  // Startup LED test
  for (byte i = 0; i < 4; i++) { digitalWrite(TEMP_LEDS[i], HIGH); delay(150); }
  digitalWrite(LED_HUMIDITY, HIGH); digitalWrite(LED_ACCESS, HIGH);
  digitalWrite(LED_TAMPER, HIGH);   digitalWrite(LED_OK, HIGH);
  delay(600);
  for (byte i = 0; i < 4; i++) digitalWrite(TEMP_LEDS[i], LOW);
  digitalWrite(LED_HUMIDITY, LOW); digitalWrite(LED_ACCESS, LOW);
  digitalWrite(LED_TAMPER, LOW);   digitalWrite(LED_OK, LOW);

  lastRawHall = digitalRead(HALL_PIN);
  lidClosed = (lastRawHall == LID_CLOSED_LEVEL);
  hallChangeTime = millis();

  if (lidClosed) {
    lockBox();
  } else {
    // Lid open at power-up: start unlocked so it doesn't instantly alarm.
    lidOpenSince = millis();
    moveServo(UNLOCKED_ANGLE);
    unlockDoneTime = millis();
    unlocked = true;
    lidOpenedSinceUnlock = true;
    unlockTime = millis();
    logMsg("Started with lid open: close it to lock");
  }

  logMsg("Smart medicine box ready");
}

void loop() {
  updateLid();
  handleCard();
  updateLockLogic();
  readClimate();
  updateLeds();
}