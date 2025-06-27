#include <Stepper.h>

// Configuración del motor 28BYJ-48
const int stepsPerRevolution = 2048;  // Medio paso para mayor precisión
const float gearRatio = 3600 / 2048;        // Relación de engranajes real del motor
Stepper myStepper(stepsPerRevolution, 8, 10, 9, 11);  // Pines ULN2003

// Sensor IR para homing
const int sensorIRPin = 2;

// Estados del sistema
volatile bool systemEnabled = false;  // 'volatile' para acceso seguro en interrupciones
bool homingDone = false;
long currentPositionSteps = 0;        // Posición absoluta en pasos

// Comandos seriales
const String CMD_START = "bg";
const String CMD_STOP = "sp";
const String CMD_HOMING = "h";

// Configuración de movimiento
const int motorRPM = 12;               // Velocidad óptima para 28BYJ-48 con carga
const unsigned long debounceDelay = 50;  // Tiempo antirrebote para sensor IR

void setup() {
  // Configuración inicial
  pinMode(sensorIRPin, INPUT_PULLUP);
  Serial.begin(9600);
  while (!Serial);  // Esperar conexión serial (solo para algunas placas)
  myStepper.setSpeed(motorRPM);
  Serial.println("SYSTEM_OFF");  // Estado inicial
}

void loop() {
  // Verificar comandos seriales continuamente
  checkSerialCommands();
}

void checkSerialCommands() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    // Comando START
    if (input == CMD_START) {
      systemEnabled = true;
      homingDone = false;
      Serial.println("SYSTEM_ON");
    }
    // Comando STOP
    else if (input == CMD_STOP) {
      systemEnabled = false;
      Serial.println("SYSTEM_OFF");
    }
    // Comando HOMING
    else if (input == CMD_HOMING && systemEnabled) {
      performHoming();
    }
    // Comando de movimiento (solo número)
    else if (systemEnabled && homingDone && isNumeric(input)) {
      moveToDegrees(input.toFloat());
    }
  }
}

// Función para verificar si un string es numérico
bool isNumeric(String str) {
  for (char c : str) {
    if (!isdigit(c) && c != '.' && c != '-') {
      return false;
    }
  }
  return true;
}

void performHoming() {
  Serial.println("HOMING_START");
  unsigned long startTime = millis();
  unsigned long lastDebounceTime = 0;
  bool sensorState = digitalRead(sensorIRPin);
  
  while (systemEnabled) {
    
    // 2. Verificar comandos seriales (para stop inmediato)
    checkSerialCommands();
    if (!systemEnabled) {
      Serial.println("HOMING_ABORTED");
      return;
    }
    
    // 3. Lógica de detección del sensor con debounce
    bool currentState = digitalRead(sensorIRPin);
    if (currentState != sensorState) {
      lastDebounceTime = millis();
      sensorState = currentState;
    }
    
    // 4. Si el sensor está activado y pasó el tiempo de debounce
    if (sensorState == LOW && (millis() - lastDebounceTime) > debounceDelay) {
      currentPositionSteps = 0;
      homingDone = true;
      Serial.println("HOMING_DONE");
      return;
    }
    
    // 5. Movimiento continuo del motor
    myStepper.step(1);
    currentPositionSteps++;
    
    // Pequeña pausa para no saturar el procesador
    delay(1);
  }
  
  Serial.println("HOMING_ABORTED");
}

void moveToDegrees(float degrees) {
  Serial.println("MOVEMENT_START");
  long targetSteps = round((degrees / 360.0) * stepsPerRevolution / gearRatio);
  long stepsRemaining = targetSteps - currentPositionSteps;
  int stepDirection = (stepsRemaining > 0) ? 1 : -1;
  
  while (stepsRemaining != 0 && systemEnabled) {
    // Verificar comandos seriales periódicamente
    if (millis() % 100 == 0) {  // Cada 100ms
      checkSerialCommands();
    }
    
    myStepper.step(stepDirection);
    currentPositionSteps += stepDirection;
    stepsRemaining -= stepDirection;
    
    // Pequeña pausa para no saturar el procesador
    delay(1);
  }
  
  if (systemEnabled) {
    Serial.print("POSITION:");
    Serial.println(currentPositionSteps);
  } else {
    Serial.println("MOVEMENT_ABORTED");
  }
}
