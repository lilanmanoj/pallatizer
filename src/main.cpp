#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pins of the Stepper Motors
#define MOTOR1_STEP_PIN 43
#define MOTOR1_DIR_PIN  44
#define MOTOR2_STEP_PIN 42
#define MOTOR2_DIR_PIN  41
#define MOTOR3_STEP_PIN 35
#define MOTOR3_DIR_PIN  47

// Sensor pin to detect the presence of a box
#define BOX_SENSOR 40

// Sensor to detect the presence of a pallet
#define PALLET_SENSOR 39

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

// Initialize the LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD I2C address

// Counter for the number of boxes processed
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
void rotateMotor(int stepPin, int dirPin, int rotations) {
  // Set the direction of rotation
  digitalWrite(dirPin, HIGH);  // Adjust HIGH/LOW for direction

  // Perform the rotation
  for (int i = 0; i < rotations * STEPS_PER_REV; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(1000);  // Adjust speed here
    digitalWrite(stepPin, LOW);
    delayMicroseconds(1000);  // Adjust speed here
  }
}

// Function to return the motor to its initial position
void returnToInitialPosition(int stepPin, int dirPin, int rotations) {
  digitalWrite(dirPin, LOW);  // Reverse the direction for return

  // Perform the return movement with the same number of rotations
  for (int i = 0; i < rotations * STEPS_PER_REV; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(1000);  // Adjust speed here
    digitalWrite(stepPin, LOW);
    delayMicroseconds(1000);  // Adjust speed here
  }
}

void setup() {
    // Set pin modes
    pinMode(MOTOR1_STEP_PIN, OUTPUT);
    pinMode(MOTOR1_DIR_PIN, OUTPUT);
    pinMode(MOTOR2_STEP_PIN, OUTPUT);
    pinMode(MOTOR2_DIR_PIN, OUTPUT);
    pinMode(MOTOR3_STEP_PIN, OUTPUT);
    pinMode(MOTOR3_DIR_PIN, OUTPUT);

    // Pump relay pin mode
    pinMode(PUMP_RELAY, OUTPUT);

    // IR sensors pin mode
    pinMode(BOX_SENSOR, INPUT);
    pinMode(PALLET_SENSOR, INPUT);

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
}

void loop() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Working...");
    lcd.setCursor(0, 1);
    lcd.print("Boxes: ");
    lcd.print(boxCounter);

    // Check if the box is detected
    if (digitalRead(BOX_SENSOR) == LOW) {  // If sensor detects the box
        Serial.println("Box detected, starting the palletizer operation...");

        // Perform operations based on the number of boxes detected
        if (boxCounter < 12) {
            // Box picking up process
            rotateMotor(MOTOR2_STEP_PIN, MOTOR2_DIR_PIN, 10);
            rotateMotor(MOTOR3_STEP_PIN, MOTOR3_DIR_PIN, 8);
            digitalWrite(PUMP_RELAY, HIGH);
            delay(500);
            returnToInitialPosition(MOTOR3_STEP_PIN, MOTOR3_DIR_PIN, 8);
            returnToInitialPosition(MOTOR2_STEP_PIN, MOTOR2_DIR_PIN, 10);
            
            // Get rotations for motor1, motor2, motor3 based on the current box count
            int motor1Rotations = rotations[boxCounter][0];
            int motor2Rotations = rotations[boxCounter][1];
            int motor3Rotations = rotations[boxCounter][2];

            // Rotate the motors based on the box count
            rotateMotor(MOTOR1_STEP_PIN, MOTOR1_DIR_PIN, motor1Rotations);
            rotateMotor(MOTOR2_STEP_PIN, MOTOR2_DIR_PIN, motor2Rotations);
            rotateMotor(MOTOR3_STEP_PIN, MOTOR3_DIR_PIN, motor3Rotations);

            // Release the box
            digitalWrite(PUMP_RELAY, LOW);
            delay(500);

            // After the motors complete their rotations, return to initial positions
            returnToInitialPosition(MOTOR3_STEP_PIN, MOTOR3_DIR_PIN, motor3Rotations);
            returnToInitialPosition(MOTOR2_STEP_PIN, MOTOR2_DIR_PIN, motor2Rotations);
            returnToInitialPosition(MOTOR1_STEP_PIN, MOTOR1_DIR_PIN, motor1Rotations);

            // Increment the box counter
            boxCounter++;
            Serial.print("Box count: ");
            Serial.println(boxCounter);
        } else {
            Serial.println("Maximum number of boxes detected.");
        }

        // Delay before checking for the next box
        delay(500);
    }
}
