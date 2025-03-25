#include <HX711.h>
#include <Servo.h>
#include <DS1302.h>

// Pin Definitions
#define LOADCELL_DOUT_PIN 2
#define LOADCELL_SCK_PIN 3
#define SERVO_PIN 9

// DS1302 RTC Pins
#define RST_PIN 5
#define DAT_PIN 6
#define CLK_PIN 7

// Configuration
#define FOOD_CALIBRATION_FACTOR -1660.0
#define FOOD_TARGET_WEIGHT 50.0  // Target weight in grams
#define FEEDING_TIMES 1          // Number of feeding times

// Feeding Times (Updated: 4:00 PM)
const int feedingTimes[FEEDING_TIMES][2] = {
  {16, 0}   // 4:00 PM
};

// Global Objects
HX711 scale;
Servo foodServo;
DS1302 rtc(RST_PIN, DAT_PIN, CLK_PIN);

// State Variables
bool scaleReady = false;
bool foodDispensed[FEEDING_TIMES] = {false};

void setup() {
  Serial.begin(9600);
  Serial.println("==== Pet Feeder Initialization ====");

  initScale();
  initServo();
  initRTC();

  Serial.println("System Ready. Waiting for feeding time.");
}

void loop() {
  Time now = rtc.time();

  // Debugging Output Every 5 Seconds
  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime > 5000) {
    Serial.print("Current Time: ");
    printCurrentTime();
    Serial.print("Current Weight: ");
    Serial.print(getCurrentWeight());
    Serial.println(" g");
    lastPrintTime = millis();
  }

  // Check Feeding Time
  for (int i = 0; i < FEEDING_TIMES; i++) {
    if (!foodDispensed[i] && now.hr == feedingTimes[i][0] && now.min == feedingTimes[i][1]) {
      Serial.println("==== FEEDING TIME DETECTED ====");
      Serial.print("Feeding at: ");
      Serial.print(feedingTimes[i][0]);
      Serial.print(":");
      Serial.println(feedingTimes[i][1]);

      dispenseFoodWithSafetyChecks();
      foodDispensed[i] = true;
    }
  }

  // Reset Dispensed Status at Midnight
  if (now.hr == 0 && now.min == 0) {
    for (int i = 0; i < FEEDING_TIMES; i++) {
      foodDispensed[i] = false;
    }
  }

  delay(1000);
}

void initScale() {
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(FOOD_CALIBRATION_FACTOR);
  scale.tare();

  float testWeight = scale.get_units(5);
  if (testWeight >= 0) {
    scaleReady = true;
    Serial.println("Load Cell Initialized Successfully.");
  } else {
    scaleReady = false;
    Serial.println("Load Cell Initialization Failed!");
  }
}

void initServo() {
  foodServo.attach(SERVO_PIN);
  stopServo();  // Ensure the servo starts in a neutral position
  Serial.println("Servo Initialized Successfully.");
}

void initRTC() {
  rtc.writeProtect(false);
  rtc.halt(false);
  Serial.println("RTC Initialized Successfully.");
}

void dispenseFoodWithSafetyChecks() {
  Serial.println("Starting Food Dispensing Process");
  openFlapGate();

  unsigned long startTime = millis();
  unsigned long maxDispensingTime = 30000; // 30 seconds timeout

  while (getCurrentWeight() < FOOD_TARGET_WEIGHT) {
    Serial.print("Current Weight: ");
    Serial.print(getCurrentWeight());
    Serial.println(" g");

    if (millis() - startTime > maxDispensingTime) {
      Serial.println("DISPENSING TIMEOUT REACHED!");
      break;
    }

    delay(500);
  }

  closeFlapGate();
  Serial.println("Feeding Process Complete");
}

// Open Flap Gate (Rotate for a short time)
void openFlapGate() {
  foodServo.write(180); // Rotate forward to open
  delay(1000); // Adjust duration to match your feeder
  stopServo(); // Stop movement
  Serial.println("Flap Gate Opened.");
}

// Close Flap Gate (Rotate in reverse for a short time)
void closeFlapGate() {
  foodServo.write(0); // Rotate backward to close
  delay(1000); // Adjust duration to match your feeder
  stopServo(); // Stop movement
  Serial.println("Flap Gate Closed.");
}

// Stop servo movement
void stopServo() {
  foodServo.write(90); // Stop continuous rotation servo
}

float getCurrentWeight() {
  if (scaleReady) {
    return abs(scale.get_units(5));
  }
  return 0;
}

void printCurrentTime() {
  Time now = rtc.time();
  char timeStr[20];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", now.hr, now.min, now.sec);
  Serial.println(timeStr);
}