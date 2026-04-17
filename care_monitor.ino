#include <Wire.h>

#define ACCEL_ADDR 0x1E
#define BUZZER_PIN 12

const unsigned long WARNING_TIME = 30000;
const unsigned long ALERT_TIME   = 40000;

const int MOVEMENT_THRESHOLD = 25000;
const int REQUIRED_HITS = 1;

// Fall detection
const int IMPACT_THRESHOLD = 120000;
const int STILLNESS_THRESHOLD = 30000;
const unsigned long FALL_CONFIRM_TIME = 2000;

enum SystemState {
  ACTIVE,
  WARNING,
  ALERT,
  FALL_ALERT
};

SystemState state = ACTIVE;

unsigned long lastMovementTime = 0;
unsigned long lastBeepTime = 0;

int movementHits = 0;
int prevAx = 0;
int prevAy = 0;
int prevAz = 0;

bool beepState = false;

bool possibleFall = false;
unsigned long impactTime = 0;

int16_t read16(byte reg) {
  Wire.beginTransmission(ACCEL_ADDR);
  Wire.write(reg | 0x80);
  Wire.endTransmission(false);
  Wire.requestFrom(ACCEL_ADDR, 2);

  if (Wire.available() < 2) return 0;

  byte low = Wire.read();
  byte high = Wire.read();
  return (int16_t)(high << 8 | low);
}

void setupAccel() {
  Wire.beginTransmission(ACCEL_ADDR);
  Wire.write(0x20);
  Wire.write(0x57);
  Wire.endTransmission();

  Wire.beginTransmission(ACCEL_ADDR);
  Wire.write(0x21);
  Wire.write(0x00);
  Wire.endTransmission();
}

void setState(SystemState newState) {
  if (state == newState) return;

  state = newState;

  if (state == ACTIVE) {
    Serial.println("ACTIVE");
  } else if (state == WARNING) {
    Serial.println("WARNING: no movement detected...");
  } else if (state == ALERT) {
    Serial.println("ALERT: no movement");
    Serial.println("Caregiver reset required");
  } else if (state == FALL_ALERT) {
    Serial.println("FALL DETECTED");
    Serial.println("Caregiver reset required");
  }
}

void caregiverReset() {
  state = ACTIVE;
  movementHits = 0;
  possibleFall = false;
  impactTime = 0;
  lastMovementTime = millis();
  digitalWrite(BUZZER_PIN, LOW);
  beepState = false;
  Serial.println("Caregiver reset successful");
  Serial.println("ACTIVE");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 19);
  pinMode(BUZZER_PIN, OUTPUT);

  setupAccel();
  delay(500);

  prevAx = read16(0x28);
  prevAy = read16(0x2A);
  prevAz = read16(0x2C);

  lastMovementTime = millis();
  Serial.println("ACTIVE");
}

void loop() {
  if (Serial.available() > 0) {
    char input = Serial.read();
    if (input == 'r' || input == 'R') {
      caregiverReset();
    }
  }

  int ax = read16(0x28);
  int ay = read16(0x2A);
  int az = read16(0x2C);

  int diff = abs(ax - prevAx) + abs(ay - prevAy) + abs(az - prevAz);

  prevAx = ax;
  prevAy = ay;
  prevAz = az;

  if (state != ALERT && state != FALL_ALERT) {

    // normal movement
    if (diff > MOVEMENT_THRESHOLD) {
      movementHits++;
    } else {
      movementHits = 0;
    }

    if (movementHits >= REQUIRED_HITS) {
      lastMovementTime = millis();
      movementHits = 0;
      possibleFall = false;

      if (state == WARNING) {
        Serial.println("Movement detected");
        setState(ACTIVE);
      }
    }

  // fall only from ACTIVE
if (state == ACTIVE && diff > IMPACT_THRESHOLD && !possibleFall) {
  possibleFall = true;
  impactTime = millis();
  Serial.println("Possible fall detected");
}

// confirm fall only after impact + stillness
if (possibleFall) {
  unsigned long fallElapsed = millis() - impactTime;

  Serial.print("Fall timer: ");
  Serial.println(fallElapsed);

  // only check for stillness after the impact happened
  if (diff < STILLNESS_THRESHOLD) {
    if (fallElapsed >= FALL_CONFIRM_TIME) {
      setState(FALL_ALERT);
      possibleFall = false;
    }
  }

  // cancel fall if too much time passes and it never became still
  if (fallElapsed > 4000 && diff >= STILLNESS_THRESHOLD) {
    possibleFall = false;
    Serial.println("Fall cancelled");
  }
}

    unsigned long inactiveTime = millis() - lastMovementTime;

    if (state == ACTIVE && inactiveTime >= WARNING_TIME) {
      setState(WARNING);
    }

    if (state == WARNING && inactiveTime >= ALERT_TIME) {
      setState(ALERT);
    }
  }

  // buzzer
  if (state == ALERT || state == FALL_ALERT) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else if (state == WARNING) {
    if (millis() - lastBeepTime >= 500) {
      beepState = !beepState;
      digitalWrite(BUZZER_PIN, beepState);
      lastBeepTime = millis();
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    beepState = false;
  }

  delay(100);
}
