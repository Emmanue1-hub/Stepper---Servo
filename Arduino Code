#include <Stepper.h>

// Configuración del motor
const int stepsPerRevolution = 2048;
const float gearRatio = 5.692;
Stepper myStepper(stepsPerRevolution, 8, 10, 9, 11);

// Sensor IR
const int sensorIRPin = 2;
bool systemEnabled = false;
bool homingDone = false;
long currentPositionSteps = 0;

// Comandos (simples strings como especificaste)
const String CMD_START = "bg";
const String CMD_STOP = "sp";
const String CMD_HOMING = "h";

// Velocidad
const int motorRPM = 8;
const unsigned long debounceDelay = 50;

void setup() {
  pinMode(sensorIRPin, INPUT_PULLUP);
  Serial.begin(9600);
  myStepper.setSpeed(motorRPM);
  Serial.println("SYSTEM_OFF");  // Estado inicial
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    // Comando START
    if (input == CMD_START) {
      systemEnabled = true;
      homingDone = false;  // Reset homing al iniciar
      Serial.println("SYSTEM_ON");
    }
    // Comando STOP
    else if (input == CMD_STOP) {
      systemEnabled = false;
      Serial.println("SYSTEM_OFF");
    }
    // Comando HOMING (solo si sistema activado)
    else if (input == CMD_HOMING && systemEnabled) {
      performHoming();
    }
    // Comando de GRADOS (solo número)
    else if (systemEnabled && homingDone && isNumeric(input)) {
      float degrees = input.toFloat();
      moveToDegrees(degrees);
      Serial.print("POSITION:");
      Serial.println(currentPositionSteps);
    }
  }
}

// Función auxiliar para verificar si es número
bool isNumeric(String str) {
  for (char c : str) {
    if (!isdigit(c) && c != '.' && c != '-') {
      return false;
    }
  }
  return true;
}

void performHoming() {
  unsigned long lastDebounceTime = 0;
  bool sensorState = digitalRead(sensorIRPin);
  
  Serial.println("HOMING_START");
  
  while (systemEnabled) {
    bool currentState = digitalRead(sensorIRPin);
    
    if (currentState != sensorState) {
      lastDebounceTime = millis();
      sensorState = currentState;
    }
    
    if (sensorState == LOW && (millis() - lastDebounceTime) > debounceDelay) {
      break;
    }
    
    myStepper.step(1);
    currentPositionSteps++;
    delay(1);
  }
  
  if (systemEnabled) {
    currentPositionSteps = 0;
    homingDone = true;
    Serial.println("HOMING_DONE");
  }
}

void moveToDegrees(float degrees) {
  long targetSteps = round((degrees / 360.0) * stepsPerRevolution * gearRatio);
  long stepsToMove = targetSteps - currentPositionSteps;
  
  // Prioridad horaria excepto en retrocesos explícitos
  myStepper.step(stepsToMove);
  currentPositionSteps = targetSteps;
}
