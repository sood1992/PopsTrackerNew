/**
 * PopsTracker - Accelerometer Module (ADXL345)
 *
 * Handles step counting, activity detection, and movement analysis
 */

#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include "config.h"

class AccelerometerModule {
public:
    AccelerometerModule();

    bool begin();
    void update();

    // Activity data
    AccelData getCurrentReading();
    ActivityData getActivityData();

    // Step counting
    uint32_t getStepCount();
    void resetStepCount();

    // Activity levels
    ActivityLevel getActivityLevel();
    bool isMoving();
    bool isResting();
    bool isScratching();    // Potential skin issue
    bool isCarried();       // Being carried (low variation)

    // Interrupt handling
    void enableWakeOnMovement();
    void disableWakeOnMovement();
    static void IRAM_ATTR handleInterrupt();

    // Calibration
    void calibrate();
    void setOffsets(float x, float y, float z);

private:
    Adafruit_ADXL345_Unified accel;

    // Current data
    AccelData currentReading;
    ActivityData activityData;

    // Step detection
    uint32_t stepCount;
    uint32_t lastStepTime;
    float lastMagnitude;
    bool stepPhase;         // True = expecting peak, False = expecting trough

    // Activity analysis
    float magnitudeHistory[50];     // ~1 second at 50Hz
    uint8_t historyIndex;
    float avgMagnitude;
    float magnitudeVariance;
    uint32_t lastActivityChange;

    // Scratch detection
    uint32_t scratchStartTime;
    uint8_t scratchCount;
    bool inScratchPattern;

    // Calibration offsets
    float offsetX, offsetY, offsetZ;

    // Interrupt flag
    static volatile bool interruptFlag;

    // Helper methods
    void detectSteps();
    void analyzeActivity();
    void detectScratching();
    float calculateMagnitude(float x, float y, float z);
    float calculateVariance();
};

extern AccelerometerModule accelerometer;

#endif // ACCELEROMETER_H
