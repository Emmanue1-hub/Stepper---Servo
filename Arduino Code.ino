#include <Stepper.h>

// Stepper motor configuration (28BYJ-48 with ULN2003 driver)
const int stepsPerRevolution = 2048;  // Half-step mode (2048 steps per motor rev)
Stepper myStepper(stepsPerRevolution, 8, 10, 9, 11);  // Pins: IN1, IN3, IN2, IN4

// IR sensor for homing (connected to pin 2)
const int sensorIRPin = 2;

// System state variables
volatile bool systemEnabled = false;  // Enable/disable flag
bool homingDone = false;             // Homing completion flag
long currentPositionSteps = 0;       // Tracks absolute position in steps

// Serial commands
const String CMD_START = "bg";       // Start system
const String CMD_STOP = "sp";        // Stop system
const String CMD_HOMING = "h";       // Homing command

// Movement settings
const int motorRPM = 12;             // Optimal speed for 28BYJ-48
const unsigned long debounceDelay = 50;  // Debounce time for IR sensor (ms)

void setup() {
  pinMode(sensorIRPin, INPUT_PULLUP);  // Enable internal pull-up for IR sensor
  Serial.begin(9600);
  while (!Serial);                     // Wait for serial connection (Leonardo/Micro)
  myStepper.setSpeed(motorRPM);
  Serial.println("SYSTEM_OFF");        // Initial state
}

void loop() {
  checkSerialCommands();  // Process incoming serial commands
}

// Check for incoming serial commands
void checkSerialCommands() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == CMD_START) {
      systemEnabled = true;
      homingDone = false;
      Serial.println("SYSTEM_ON");
    } 
    else if (input == CMD_STOP) {
      systemEnabled = false;
      Serial.println("SYSTEM_OFF");
    } 
    else if (input == CMD_HOMING && systemEnabled) {
      performHoming();
    } 
    else if (systemEnabled && homingDone && isNumeric(input)) {
      moveToDegrees(input.toFloat());  // Move to specified degrees
    }
  }
}

// Check if a string is numeric (for degree input)
bool isNumeric(String str) {
  for (char c : str) {
    if (!isdigit(c) && c != '.' && c != '-') {
      return false;
    }
  }
  return true;
}

// Homing routine (moves motor until IR sensor triggers)
void performHoming() {
  Serial.println("HOMING_START");
  unsigned long lastDebounceTime = 0;
  bool sensorState = digitalRead(sensorIRPin);

  while (systemEnabled) {
    checkSerialCommands();  // Allow abort via serial
    if (!systemEnabled) {
      Serial.println("HOMING_ABORTED");
      return;
    }

    bool currentState = digitalRead(sensorIRPin);
    if (currentState != sensorState) {
      lastDebounceTime = millis();
      sensorState = currentState;
    }

    // If sensor is triggered (LOW) and debounce time passed
    if (sensorState == LOW && (millis() - lastDebounceTime) > debounceDelay) {
      currentPositionSteps = 0;  // Reset position to 0
      homingDone = true;
      Serial.println("HOMING_DONE");
      return;
    }

    myStepper.step(1);  // Move one step clockwise
    currentPositionSteps++;
    delay(1);  // Small delay to reduce CPU load
  }
  Serial.println("HOMING_ABORTED");
}

// Move to a specific angle (in degrees)
void moveToDegrees(float degrees) {
  Serial.println("MOVEMENT_START");
  
  // CORRECTED FORMULA: 3600 steps = 360° output
  long targetSteps = round((degrees / 360.0) * 3600);
  long stepsRemaining = targetSteps - currentPositionSteps;
  int stepDirection = (stepsRemaining > 0) ? 1 : -1;

  while (stepsRemaining != 0 && systemEnabled) {
    myStepper.step(stepDirection);
    currentPositionSteps += stepDirection;
    stepsRemaining -= stepDirection;
    delay(1);  // Small delay to reduce CPU load
  }

  if (systemEnabled) {
    Serial.print("POSITION:");
    Serial.println(currentPositionSteps);  // Report final position
  } else {
    Serial.println("MOVEMENT_ABORTED");
  }
}
