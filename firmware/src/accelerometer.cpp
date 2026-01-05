/**
 * PopsTracker - Accelerometer Module Implementation
 */

#include "accelerometer.h"

// Global instance
AccelerometerModule accelerometer;

// Static interrupt flag
volatile bool AccelerometerModule::interruptFlag = false;

AccelerometerModule::AccelerometerModule() :
    accel(12345),   // Unique sensor ID
    stepCount(0),
    lastStepTime(0),
    lastMagnitude(1.0),
    stepPhase(true),
    historyIndex(0),
    avgMagnitude(1.0),
    magnitudeVariance(0),
    lastActivityChange(0),
    scratchStartTime(0),
    scratchCount(0),
    inScratchPattern(false),
    offsetX(0), offsetY(0), offsetZ(0)
{
    memset(magnitudeHistory, 0, sizeof(magnitudeHistory));
    memset(&currentReading, 0, sizeof(AccelData));
    memset(&activityData, 0, sizeof(ActivityData));
}

bool AccelerometerModule::begin() {
    // Note: I2C is already initialized in main.cpp initializeSensors()
    DEBUG_PRINTLN("Initializing ADXL345...");

    if (!accel.begin(ADXL345_ADDR)) {
        DEBUG_PRINTLN("ERROR: ADXL345 not found!");
        return false;
    }

    // Configure sensor
    accel.setRange(ADXL345_RANGE_4_G);      // +/- 4g range for dog activity
    accel.setDataRate(ADXL345_DATARATE_50_HZ);  // 50 Hz sample rate

    // Setup interrupt pin
    pinMode(ADXL345_INT1, INPUT);
    attachInterrupt(digitalPinToInterrupt(ADXL345_INT1), handleInterrupt, RISING);

    DEBUG_PRINTLN("ADXL345 initialized successfully");
    DEBUG_PRINTF("Range: +/- %d g\n", 4);

    return true;
}

void IRAM_ATTR AccelerometerModule::handleInterrupt() {
    interruptFlag = true;
}

void AccelerometerModule::update() {
    // Read accelerometer
    sensors_event_t event;
    accel.getEvent(&event);

    // Apply calibration offsets
    float x = event.acceleration.x - offsetX;
    float y = event.acceleration.y - offsetY;
    float z = event.acceleration.z - offsetZ;

    // Convert to G (accelerometer already returns m/s^2, divide by 9.81)
    x /= 9.81;
    y /= 9.81;
    z /= 9.81;

    // Update current reading
    currentReading.x = x;
    currentReading.y = y;
    currentReading.z = z;
    currentReading.magnitude = calculateMagnitude(x, y, z);
    currentReading.timestamp = millis();

    // Update magnitude history for variance calculation
    magnitudeHistory[historyIndex] = currentReading.magnitude;
    historyIndex = (historyIndex + 1) % 50;

    // Detect steps
    detectSteps();

    // Analyze activity level
    analyzeActivity();

    // Detect scratching behavior
    detectScratching();

    // Check interrupt flag
    if (interruptFlag) {
        interruptFlag = false;
        DEBUG_PRINTLN("Wake interrupt triggered");
    }
}

float AccelerometerModule::calculateMagnitude(float x, float y, float z) {
    return sqrt(x * x + y * y + z * z);
}

void AccelerometerModule::detectSteps() {
    float magnitude = currentReading.magnitude;
    uint32_t currentTime = millis();

    // Debounce check
    if (currentTime - lastStepTime < STEP_DEBOUNCE_MS) {
        lastMagnitude = magnitude;
        return;
    }

    // Step detection using peak detection algorithm
    // Looking for a pattern: low -> high -> low (each step causes acceleration spike)
    if (stepPhase) {
        // Looking for peak
        if (magnitude > STEP_THRESHOLD && lastMagnitude <= magnitude) {
            // Still rising, wait for peak
        } else if (magnitude < lastMagnitude && lastMagnitude > STEP_THRESHOLD) {
            // Found peak, now looking for trough
            stepPhase = false;
        }
    } else {
        // Looking for trough (step completion)
        if (magnitude < 1.0 && lastMagnitude >= magnitude) {
            // Still falling
        } else if (magnitude > lastMagnitude && lastMagnitude < 1.0) {
            // Found trough - step complete!
            stepCount++;
            activityData.stepCount = stepCount;
            lastStepTime = currentTime;
            stepPhase = true;

            DEBUG_PRINTF("Step detected! Count: %u\n", stepCount);
        }
    }

    lastMagnitude = magnitude;
}

float AccelerometerModule::calculateVariance() {
    // Calculate variance of magnitude history
    float sum = 0;
    for (int i = 0; i < 50; i++) {
        sum += magnitudeHistory[i];
    }
    avgMagnitude = sum / 50.0;

    float varianceSum = 0;
    for (int i = 0; i < 50; i++) {
        float diff = magnitudeHistory[i] - avgMagnitude;
        varianceSum += diff * diff;
    }

    return varianceSum / 50.0;
}

void AccelerometerModule::analyzeActivity() {
    magnitudeVariance = calculateVariance();
    uint32_t currentTime = millis();

    // Determine activity level based on variance and magnitude
    ActivityLevel newLevel;

    if (magnitudeVariance < 0.01) {
        // Very little movement
        newLevel = ACTIVITY_RESTING;
        activityData.isMoving = false;
        activityData.isSleeping = (currentTime - activityData.lastActivityMs > 60000); // 1 min no activity
    } else if (magnitudeVariance < 0.1 || avgMagnitude < LIGHT_THRESH + 0.5) {
        newLevel = ACTIVITY_LIGHT;
        activityData.isMoving = true;
        activityData.isSleeping = false;
        activityData.lastActivityMs = currentTime;
    } else if (magnitudeVariance < 0.5 || avgMagnitude < ACTIVE_THRESH + 0.5) {
        newLevel = ACTIVITY_ACTIVE;
        activityData.isMoving = true;
        activityData.isSleeping = false;
        activityData.lastActivityMs = currentTime;
    } else {
        newLevel = ACTIVITY_VERY_ACTIVE;
        activityData.isMoving = true;
        activityData.isSleeping = false;
        activityData.lastActivityMs = currentTime;
    }

    // Track active minutes
    if (newLevel >= ACTIVITY_LIGHT &&
        currentTime - lastActivityChange >= 60000) {
        activityData.activeMinutes++;
        lastActivityChange = currentTime;
    }

    activityData.level = newLevel;
}

void AccelerometerModule::detectScratching() {
    // Scratching produces rapid, repetitive motion patterns
    // Typically higher frequency than walking, with consistent pattern

    float magnitude = currentReading.magnitude;
    uint32_t currentTime = millis();

    // Look for rapid oscillations above scratch threshold
    if (magnitude > SCRATCH_THRESHOLD) {
        if (!inScratchPattern) {
            scratchStartTime = currentTime;
            inScratchPattern = true;
            scratchCount = 1;
        } else {
            scratchCount++;
        }
    } else if (inScratchPattern && magnitude < 1.5) {
        // Check if pattern matches scratching (rapid bursts)
        uint32_t duration = currentTime - scratchStartTime;

        // Scratching: >5 spikes in <2 seconds
        if (duration > 500 && duration < 2000 && scratchCount > 5) {
            activityData.isScratching = true;
            DEBUG_PRINTF("Scratching detected! Count: %d in %u ms\n", scratchCount, duration);
        } else {
            // Reset pattern if it doesn't match
            inScratchPattern = false;
            scratchCount = 0;

            // Clear scratching flag after 5 seconds
            if (currentTime - scratchStartTime > 5000) {
                activityData.isScratching = false;
            }
        }
    }
}

bool AccelerometerModule::isCarried() {
    // When dog is being carried, accelerometer shows:
    // - Low variance (not walking)
    // - But GPS shows movement
    // This function returns true if accelerometer shows "no dog movement"

    return magnitudeVariance < CARRIED_ACCEL_THRESH;
}

AccelData AccelerometerModule::getCurrentReading() {
    return currentReading;
}

ActivityData AccelerometerModule::getActivityData() {
    return activityData;
}

uint32_t AccelerometerModule::getStepCount() {
    return stepCount;
}

void AccelerometerModule::resetStepCount() {
    stepCount = 0;
    activityData.stepCount = 0;
}

ActivityLevel AccelerometerModule::getActivityLevel() {
    return activityData.level;
}

bool AccelerometerModule::isMoving() {
    return activityData.isMoving;
}

bool AccelerometerModule::isResting() {
    return activityData.level == ACTIVITY_RESTING;
}

bool AccelerometerModule::isScratching() {
    return activityData.isScratching;
}

void AccelerometerModule::enableWakeOnMovement() {
    // Configure ADXL345 for activity interrupt
    // This allows ESP32 to wake from sleep on movement

    // Write to THRESH_ACT register (threshold for activity)
    Wire.beginTransmission(ADXL345_ADDR);
    Wire.write(0x24);   // THRESH_ACT register
    Wire.write(0x20);   // ~2g threshold (32 * 62.5mg)
    Wire.endTransmission();

    // Write to ACT_INACT_CTL register
    Wire.beginTransmission(ADXL345_ADDR);
    Wire.write(0x27);   // ACT_INACT_CTL register
    Wire.write(0x70);   // AC-coupled, enable X, Y, Z for activity
    Wire.endTransmission();

    // Map activity interrupt to INT1
    Wire.beginTransmission(ADXL345_ADDR);
    Wire.write(0x2F);   // INT_MAP register
    Wire.write(0x00);   // All interrupts to INT1
    Wire.endTransmission();

    // Enable activity interrupt
    Wire.beginTransmission(ADXL345_ADDR);
    Wire.write(0x2E);   // INT_ENABLE register
    Wire.write(0x10);   // Enable Activity interrupt
    Wire.endTransmission();

    DEBUG_PRINTLN("Wake on movement enabled");
}

void AccelerometerModule::disableWakeOnMovement() {
    // Disable activity interrupt
    Wire.beginTransmission(ADXL345_ADDR);
    Wire.write(0x2E);   // INT_ENABLE register
    Wire.write(0x00);   // Disable all interrupts
    Wire.endTransmission();

    DEBUG_PRINTLN("Wake on movement disabled");
}

void AccelerometerModule::calibrate() {
    DEBUG_PRINTLN("Calibrating accelerometer...");
    DEBUG_PRINTLN("Keep device still on flat surface!");

    float sumX = 0, sumY = 0, sumZ = 0;
    const int samples = 100;

    delay(1000);    // Wait for device to settle

    for (int i = 0; i < samples; i++) {
        sensors_event_t event;
        accel.getEvent(&event);

        sumX += event.acceleration.x;
        sumY += event.acceleration.y;
        sumZ += event.acceleration.z;

        delay(20);
    }

    // Calculate offsets (Z should be ~9.81 when flat)
    offsetX = sumX / samples;
    offsetY = sumY / samples;
    offsetZ = (sumZ / samples) - 9.81;  // Subtract gravity

    DEBUG_PRINTF("Calibration complete. Offsets: X=%.3f, Y=%.3f, Z=%.3f\n",
                 offsetX, offsetY, offsetZ);
}

void AccelerometerModule::setOffsets(float x, float y, float z) {
    offsetX = x;
    offsetY = y;
    offsetZ = z;
}
