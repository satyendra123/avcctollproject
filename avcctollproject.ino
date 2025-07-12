#define AXLE_SENSOR_PIN 2
#define HEIGHT_SENSOR_COUNT 10
#define AXLE_TIMEOUT_MS 3000  // 3 seconds after last axle

const int heightSensorPins[HEIGHT_SENSOR_COUNT] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
const int heightValues[HEIGHT_SENSOR_COUNT]     = {1400, 1520, 1650, 1780, 1910, 2030, 2400, 2600, 2800, 3000};

volatile bool newAxleDetected = false;
volatile int axleCount = 0;

unsigned long lastAxleTime = 0;
bool detectionInProgress = false;
bool cooldownActive = false;

void countAxle() {
  static unsigned long lastTrigger = 0;
  unsigned long now = millis();

  if (now - lastTrigger > 50) {
    if (digitalRead(AXLE_SENSOR_PIN) == HIGH) {
      axleCount++;
      newAxleDetected = true;
      lastAxleTime = now;
    }
  }
  lastTrigger = now;
}

void setup() {
  Serial.begin(9600);
  pinMode(AXLE_SENSOR_PIN, INPUT);  // Use INPUT_PULLUP if sensor is open-collector
  attachInterrupt(digitalPinToInterrupt(AXLE_SENSOR_PIN), countAxle, RISING);

  for (int i = 0; i < HEIGHT_SENSOR_COUNT; i++) {
    pinMode(heightSensorPins[i], INPUT);  // Use INPUT_PULLUP if sensor requires it
  }

  Serial.println("System Ready");
}

void loop() {
  if (!detectionInProgress && !cooldownActive && newAxleDetected) {
    detectionInProgress = true;
    newAxleDetected = false;
    lastAxleTime = millis();
  }

  if (detectionInProgress) {
    bool sensorTriggered[HEIGHT_SENSOR_COUNT] = {false};

    while (millis() - lastAxleTime < AXLE_TIMEOUT_MS) {
      for (int i = 0; i < HEIGHT_SENSOR_COUNT; i++) {
        // Use LOW here if sensor is active-low
        if (digitalRead(heightSensorPins[i]) == HIGH) {
          sensorTriggered[i] = true;
        }
      }

      if (newAxleDetected) {
        newAxleDetected = false;
        lastAxleTime = millis();  // Reset timeout
      }
    }

    // Determine the highest triggered sensor
    int highestSensorIndex = -1;
    for (int i = HEIGHT_SENSOR_COUNT - 1; i >= 0; i--) {
      if (sensorTriggered[i]) {
        highestSensorIndex = i;
        break;
      }
    }

    int vehicleHeight = (highestSensorIndex >= 0) ? heightValues[highestSensorIndex] : 0;
    String vehicleType = classifyVehicle(axleCount);

    if (vehicleHeight > 0 || axleCount > 0) {
      Serial.print("|AA|");
      Serial.print(axleCount);
      Serial.print("|");
      Serial.print(vehicleType);
      Serial.print("|");
      Serial.print(vehicleHeight);
      Serial.println("|FF|");
    } else {
      Serial.println("⚠️ Ignored false trigger (1 axle + no height)");
    }

    // Reset system
    axleCount = 0;
    detectionInProgress = false;
    cooldownActive = true;
    delay(3000);  // Cooldown delay
    cooldownActive = false;
    Serial.println("System Ready");
  }
}

String classifyVehicle(int axles) {
  switch (axles) {
    case 1: return "Bike";
    case 2: return "Car";
    case 3: return "MiniBus";
    case 4: return "Bus";
    case 5: return "Truck";
    default: return (axles > 5) ? "MultiAxle" : "Unknown";
  }
}
