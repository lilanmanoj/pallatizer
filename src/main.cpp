#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pins of the Stepper Motors
#define MOTOR_X_STEP_PIN 43
#define MOTOR_X_DIR_PIN  44
#define MOTOR_Y_STEP_PIN 42
#define MOTOR_Y_DIR_PIN  41
#define MOTOR_Z_STEP_PIN 35
#define MOTOR_Z_DIR_PIN  47

// Sensor pin to detect the presence of a box
#define BOX_SENSOR 40

// Sensor to detect the presence of a pallet
#define PALLET_SENSOR 39

// Home position sensors
#define HOME_SENSOR_X 38
#define HOME_SENSOR_Y 37
#define HOME_SENSOR_Z 36

// ESTOP Switch
#define ESTOP_SW 21

// Home Switch
#define HOME_SW 20

// start Switch
#define START_SW 19

// Vacuum Pump Relay Pin
#define PUMP_RELAY 48

// Limit Switch Pins 
#define L1 4
#define L2 5
#define L3 6
#define L4 7
#define L5 15
#define L6 16

// Number of steps per revolution 
#define STEPS_PER_REV 200

// Delays
#define PUMP_DELAY 1000
#define STEP_DELAY 1000
#define PROG_LOOP_DELAY 500

// Initialize the LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD I2C address

volatile bool start = false;
volatile bool estopped = false;
volatile bool requestHome = false;
uint8_t boxCounter = 0;

// Rotation values for each box in an array:
// Format: rotations[box][motor] where motor = 0: motor1, 1: motor2, 2: motor3
int rotations[12][3] = {
    {5, 4, 7}, // 1st box
    {4, 4, 7}, // 2nd box
    {3, 4, 7}, // 3rd box
    {5, 3, 7}, // 4th box
    {4, 3, 7}, // 5th box
    {3, 3, 5}, // 6th box
    {5, 4, 5}, // 7th box
    {4, 4, 5}, // 8th box
    {3, 4, 5}, // 9th box
    {5, 3, 5}, // 10th box
    {4, 3, 5}, // 11th box
    {3, 3, 5}  // 12th box
};

// Function to rotate motor
void driveMotor(int stepPin, int dirPin, int rotations, bool reverse = false) {
  // Set the direction of rotation
  digitalWrite(dirPin, reverse);  // Adjust HIGH/LOW for direction

  // Perform the rotation
  for (int i = 0; i < rotations * STEPS_PER_REV; i++) {
    // If a homing request or estop was raised, abort the current motion
    if (requestHome || estopped) {
      return;
    }

    digitalWrite(stepPin, HIGH);
    delayMicroseconds(STEP_DELAY);  // Adjust speed here
    digitalWrite(stepPin, LOW);
    delayMicroseconds(STEP_DELAY);  // Adjust speed here
  }
}

void goToHomePosition() {
  while (digitalRead(HOME_SENSOR_X) == LOW) {
    driveMotor(MOTOR_X_STEP_PIN, MOTOR_X_DIR_PIN, 1);
  }

  while (digitalRead(HOME_SENSOR_Y) == LOW) {
    driveMotor(MOTOR_Y_STEP_PIN, MOTOR_Y_DIR_PIN, 1);
  }

  while (digitalRead(HOME_SENSOR_Z) == LOW) {
    driveMotor(MOTOR_Z_STEP_PIN, MOTOR_Z_DIR_PIN, 1);
  }
}

void pickupTheBox() {
  driveMotor(MOTOR_Y_STEP_PIN, MOTOR_Y_DIR_PIN, 10);
  driveMotor(MOTOR_Z_STEP_PIN, MOTOR_Z_DIR_PIN, 8);
  digitalWrite(PUMP_RELAY, HIGH);
  delay(PUMP_DELAY);
  driveMotor(MOTOR_Z_STEP_PIN, MOTOR_Z_DIR_PIN, 8, true);
  driveMotor(MOTOR_Y_STEP_PIN, MOTOR_Y_DIR_PIN, 10, true);
}

void IRAM_ATTR handleStartSw() {
  start = (estopped == false) ? true : false;
}

void IRAM_ATTR handleHomeSw() {
  requestHome = true;
}

void IRAM_ATTR handleEstop() {
  estopped = true;
}

void IRAM_ATTR releaseEstop() {
  estopped = false;
}

void setup() {
  // Set pin modes
  pinMode(MOTOR_X_STEP_PIN, OUTPUT);
  pinMode(MOTOR_X_DIR_PIN, OUTPUT);
  pinMode(MOTOR_Y_STEP_PIN, OUTPUT);
  pinMode(MOTOR_Y_DIR_PIN, OUTPUT);
  pinMode(MOTOR_Z_STEP_PIN, OUTPUT);
  pinMode(MOTOR_Z_DIR_PIN, OUTPUT);

  // Pump relay pin mode
  pinMode(PUMP_RELAY, OUTPUT);

  // IR sensors pin mode
  pinMode(BOX_SENSOR, INPUT);
  pinMode(PALLET_SENSOR, INPUT);
  pinMode(HOME_SENSOR_X, INPUT);
  pinMode(HOME_SENSOR_Y, INPUT);
  pinMode(HOME_SENSOR_Z, INPUT);

  // Switches pin modes
  pinMode(ESTOP_SW, INPUT_PULLUP);
  pinMode(HOME_SW, INPUT_PULLUP);
  pinMode(START_SW, INPUT_PULLUP);

  //Limit switches pin mode
  pinMode(L1, INPUT_PULLUP);
  pinMode(L2, INPUT_PULLUP);
  pinMode(L3, INPUT_PULLUP);
  pinMode(L4, INPUT_PULLUP);
  pinMode(L5, INPUT_PULLUP);
  pinMode(L6, INPUT_PULLUP);

  // Initialize serial for debugging
  Serial.begin(115200);

  // Initialize I2C with custom pins
  // SDA = GPIO1, SCL = GPIO2
  Wire.begin(1, 2);
  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrating...");

  // Initially go to home position
  goToHomePosition();

  // Interrupt routines
  attachInterrupt(
    digitalPinToInterrupt(START_SW),
    handleStartSw,
    FALLING
  );

  attachInterrupt(
    digitalPinToInterrupt(HOME_SW),
    handleHomeSw,
    FALLING
  );

  attachInterrupt(
    digitalPinToInterrupt(ESTOP_SW),
    handleEstop,
    RISING
  );

  attachInterrupt(
    digitalPinToInterrupt(ESTOP_SW),
    releaseEstop,
    FALLING
  );
}

void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);

  if (requestHome) {
    noInterrupts();
    requestHome = false;
    interrupts();
    goToHomePosition();
  }

  if (estopped) {
    lcd.print("E-stopped!");
    start = false;
  } else {
    lcd.print("Press start!");
  }

  while (start) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Waiting...");

    if (requestHome) {
      noInterrupts();
      requestHome = false;
      interrupts();
      goToHomePosition();

      continue;
    }

    if (estopped) {
      break;
    }

    boxCounter = 0;

    while (digitalRead(PALLET_SENSOR) == LOW) {
      if (requestHome) {
        noInterrupts();
        requestHome = false;
        interrupts();
        goToHomePosition();

        continue;
      }

      if (estopped) {
        break;
      }

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Working...");
      lcd.setCursor(0, 1);
      lcd.print("Boxes: ");
      lcd.print(boxCounter);

      if (digitalRead(BOX_SENSOR) == LOW) {
        goToHomePosition();

        if (boxCounter < 8) {
          pickupTheBox();

          // Get rotations for motor1, motor2, motor3 based on the current box count
          int motor1Rotations = rotations[boxCounter][0];
          int motor2Rotations = rotations[boxCounter][1];
          int motor3Rotations = rotations[boxCounter][2];

          // Rotate the motors based on the box count
          driveMotor(MOTOR_X_STEP_PIN, MOTOR_X_DIR_PIN, motor1Rotations);
          driveMotor(MOTOR_Y_STEP_PIN, MOTOR_Y_DIR_PIN, motor2Rotations);
          driveMotor(MOTOR_Z_STEP_PIN, MOTOR_Z_DIR_PIN, motor3Rotations);

          // Release the box
          digitalWrite(PUMP_RELAY, LOW);
          delay(PUMP_DELAY);

          // After the motors complete their rotations, return to initial positions
          driveMotor(MOTOR_Z_STEP_PIN, MOTOR_Z_DIR_PIN, motor3Rotations, true);
          driveMotor(MOTOR_Y_STEP_PIN, MOTOR_Y_DIR_PIN, motor2Rotations, true);
          driveMotor(MOTOR_X_STEP_PIN, MOTOR_X_DIR_PIN, motor1Rotations, true);

          // Increment the box counter
          boxCounter++;
        }
      }
    }
  }

  delay(PROG_LOOP_DELAY);
}
