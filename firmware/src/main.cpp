/**
 * PopsTracker - Dog Collar GPS & Activity Tracker
 *
 * Main firmware for LILYGO T-A7670G R2
 *
 * Features:
 * - Real-time GPS tracking
 * - Activity monitoring (steps, activity level)
 * - Walk session tracking with grading
 * - Walker accountability
 * - Cheating detection (carried, vehicle)
 * - LTE connectivity for live updates
 *
 * Hardware:
 * - LILYGO T-A7670G R2 (ESP32 + A7670G LTE + L76K GPS)
 * - ADXL345 accelerometer
 * - Buzzer & vibration motor for alerts
 *
 * Author: Built for Popcorn
 */

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "accelerometer.h"
#include "gps.h"
#include "cellular.h"
#include "walk_tracker.h"

// ============================================================================
// GLOBAL STATE
// ============================================================================
DeviceStatus deviceStatus;
GPSData gpsData;
ActivityData activityData;

// Timing
uint32_t lastGPSUpdate = 0;
uint32_t lastAccelUpdate = 0;
uint32_t lastServerSync = 0;
uint32_t lastHeartbeat = 0;
uint32_t lastDisplayUpdate = 0;
uint32_t bootTime = 0;

// Flags
bool gpsInitialized = false;
bool modemInitialized = false;
bool accelInitialized = false;

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================
void initializeHardware();
void initializeSensors();
void initializeConnectivity();
void updateSensors();
void updateWalkTracking();
void syncToServer();
void sendHeartbeat();
void handleAlerts();
void updateBattery();
void enterSleepMode();
void handleSerialCommands();
void buzzerBeep(int onMs, int offMs, int count);
void motorVibrate(int onMs, int offMs, int count);
void printStatus();

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    bootTime = millis();

    // Initialize serial for debugging
    Serial.begin(SERIAL_BAUD);
    delay(1000);

    Serial.println("\n\n");
    Serial.println("╔═══════════════════════════════════════════╗");
    Serial.println("║     PopsTracker - Dog Collar Tracker      ║");
    Serial.println("║         Made with ❤ for Popcorn           ║");
    Serial.println("╠═══════════════════════════════════════════╣");
    Serial.printf("║  Firmware: v%s                        ║\n", FIRMWARE_VERSION);
    Serial.printf("║  Device: %-30s  ║\n", DEVICE_ID);
    Serial.println("╚═══════════════════════════════════════════╝");
    Serial.println();

    // Initialize hardware
    initializeHardware();

    // Initialize sensors
    initializeSensors();

    // Initialize connectivity
    initializeConnectivity();

    // Initialize walk tracker
    walkTracker.begin();

    // Ready beep
    buzzerBeep(100, 100, 2);

    Serial.println("\n=== SYSTEM READY ===");
    printStatus();

    deviceStatus.state = STATE_IDLE;
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
    uint32_t now = millis();

    // Update accelerometer (high frequency)
    if (now - lastAccelUpdate >= 20) {  // 50Hz
        lastAccelUpdate = now;
        if (accelInitialized) {
            accelerometer.update();
            activityData = accelerometer.getActivityData();
        }
    }

    // Update GPS
    if (gpsInitialized) {
        gpsModule.update();

        // Get fresh GPS data periodically
        uint32_t gpsInterval = walkTracker.isWalkInProgress() ?
            GPS_UPDATE_INTERVAL_ACTIVE : GPS_UPDATE_INTERVAL_NORMAL;

        if (now - lastGPSUpdate >= gpsInterval) {
            lastGPSUpdate = now;
            gpsData = gpsModule.getCurrentPosition();

            if (gpsData.valid) {
                DEBUG_PRINTF("GPS: %.6f, %.6f | Speed: %.1f km/h | Sats: %d\n",
                             gpsData.latitude, gpsData.longitude,
                             gpsData.speed, gpsData.satellites);
            }
        }
    }

    // Update walk tracking
    updateWalkTracking();

    // Sync to server
    if (now - lastServerSync >= SYNC_INTERVAL_MS) {
        lastServerSync = now;
        syncToServer();
    }

    // Send heartbeat
    if (now - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
        lastHeartbeat = now;
        sendHeartbeat();
    }

    // Update battery status
    updateBattery();

    // Handle alerts
    handleAlerts();

    // Handle serial commands for debugging
    handleSerialCommands();

    // Status update every 10 seconds
    if (now - lastDisplayUpdate >= 10000) {
        lastDisplayUpdate = now;
        printStatus();
    }

    // Small delay to prevent watchdog issues
    delay(10);
}

// ============================================================================
// INITIALIZATION
// ============================================================================
void initializeHardware() {
    Serial.println("Initializing hardware...");

    // Configure output pins
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(MOTOR_PIN, OUTPUT);
    pinMode(MODEM_POWER_ON, OUTPUT);

    // Ensure outputs are off
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(MOTOR_PIN, LOW);

    // Board power hold (keep device powered)
    digitalWrite(MODEM_POWER_ON, HIGH);

    Serial.println("Hardware GPIO configured");
}

void initializeSensors() {
    Serial.println("\nInitializing sensors...");

    // Initialize I2C with custom pins (since GPS uses default I2C pins)
    Wire.begin(I2C_SDA, I2C_SCL, I2C_FREQ);
    Serial.printf("I2C initialized on SDA=%d, SCL=%d\n", I2C_SDA, I2C_SCL);

    // Initialize accelerometer
    Serial.println("\n--- Accelerometer (ADXL345) ---");
    if (accelerometer.begin()) {
        accelInitialized = true;
        Serial.println("ADXL345: OK");
    } else {
        Serial.println("ADXL345: FAILED");
    }

    // Initialize GPS
    Serial.println("\n--- GPS (L76K) ---");
    if (gpsModule.begin()) {
        gpsInitialized = true;
        Serial.println("GPS: OK (waiting for fix...)");
    } else {
        Serial.println("GPS: FAILED");
    }
}

void initializeConnectivity() {
    Serial.println("\n--- Cellular (A7670G) ---");

    if (cellular.begin()) {
        modemInitialized = true;
        Serial.println("Modem initialized");

        // Connect to network
        Serial.println("Connecting to cellular network...");
        if (cellular.connect()) {
            Serial.println("Connected to network!");
            Serial.printf("Operator: %s\n", cellular.getOperator().c_str());
            Serial.printf("Signal: %d\n", cellular.getSignalStrength());
            deviceStatus.modemConnected = true;
        } else {
            Serial.println("Failed to connect to network");
            deviceStatus.modemConnected = false;
        }
    } else {
        Serial.println("Modem initialization failed");
        modemInitialized = false;
    }
}

// ============================================================================
// UPDATE FUNCTIONS
// ============================================================================
void updateWalkTracking() {
    // Update walk tracker with current sensor data
    walkTracker.update(gpsData, activityData);

    // Check for walk state changes
    static WalkState lastState = WALK_IDLE;
    WalkState currentState = walkTracker.getState();

    if (currentState != lastState) {
        switch (currentState) {
            case WALK_STARTING:
                Serial.println("\n*** WALK STARTING ***");
                buzzerBeep(150, 100, 2);
                deviceStatus.state = STATE_WALK;
                break;

            case WALK_ACTIVE:
                Serial.println("*** WALK ACTIVE ***");
                break;

            case WALK_PAUSED:
                Serial.println("*** WALK PAUSED ***");
                motorVibrate(200, 100, 2);
                break;

            case WALK_COMPLETED:
                Serial.println("\n*** WALK COMPLETED ***");
                Serial.println(walkTracker.generateWalkReport());

                // Sync walk data immediately
                WalkSession walk = walkTracker.getCurrentWalk();
                cellular.sendWalkData(walk);

                // Alert if walk was bad
                if (walkTracker.wasWalkerSlacking()) {
                    Serial.println("!!! WALKER WAS SLACKING !!!");
                    buzzerBeep(200, 100, 5);
                    cellular.sendAlert("BAD_WALK", "Walker did not walk dog properly!");
                } else {
                    buzzerBeep(300, 100, 1);  // Happy beep
                }

                deviceStatus.state = STATE_IDLE;
                break;

            default:
                break;
        }
        lastState = currentState;
    }
}

void syncToServer() {
    if (!cellular.isConnected()) {
        DEBUG_PRINTLN("Not connected, skipping sync");
        return;
    }

    // Send current location and activity
    if (gpsData.valid) {
        cellular.sendLocation(gpsData, activityData);
        deviceStatus.lastSyncTime = millis();
    }
}

void sendHeartbeat() {
    if (!cellular.isConnected()) return;

    deviceStatus.uptime = (millis() - bootTime) / 1000;
    deviceStatus.gpsConnected = gpsInitialized && gpsData.valid;
    deviceStatus.signalStrength = cellular.getSignalStrength();

    cellular.sendHeartbeat(deviceStatus);
}

void handleAlerts() {
    // Check for scratching (potential skin issue)
    static bool scratchAlertSent = false;
    if (activityData.isScratching && !scratchAlertSent) {
        Serial.println("ALERT: Excessive scratching detected!");
        cellular.sendAlert("SCRATCHING", "Dog is scratching excessively - check for skin issues");
        scratchAlertSent = true;
    } else if (!activityData.isScratching) {
        scratchAlertSent = false;
    }

    // Check battery
    if (deviceStatus.batteryPercent <= 20 && deviceStatus.batteryPercent > 5) {
        static uint32_t lastLowBatAlert = 0;
        if (millis() - lastLowBatAlert > 300000) {  // Every 5 min
            Serial.println("ALERT: Low battery!");
            cellular.sendAlert("LOW_BATTERY", "Battery below 20%");
            lastLowBatAlert = millis();
        }
    }
}

void updateBattery() {
    // Read battery ADC
    int rawValue = 0;
    for (int i = 0; i < BAT_ADC_SAMPLES; i++) {
        rawValue += analogRead(BAT_ADC_PIN);
        delay(2);
    }
    rawValue /= BAT_ADC_SAMPLES;

    // Convert to voltage (assuming 50% voltage divider)
    // ADC is 12-bit (0-4095) for 0-3.3V
    float voltage = rawValue * 2.0 * 3.3 / 4095.0;
    deviceStatus.batteryVoltage = voltage;

    // Calculate percentage (linear approximation)
    // 4.2V = 100%, 3.0V = 0%
    float percent = (voltage - 3.0) / (4.2 - 3.0) * 100.0;
    if (percent > 100) percent = 100;
    if (percent < 0) percent = 0;
    deviceStatus.batteryPercent = (uint8_t)percent;
}

void enterSleepMode() {
    Serial.println("Entering sleep mode...");

    // Disable outputs
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(MOTOR_PIN, LOW);

    // Put modem in low power
    cellular.powerOff();

    // Enable accelerometer wake on movement
    accelerometer.enableWakeOnMovement();

    // Enter deep sleep
    esp_sleep_enable_ext0_wakeup((gpio_num_t)ADXL345_INT1, HIGH);
    esp_deep_sleep_start();
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================
void buzzerBeep(int onMs, int offMs, int count) {
    for (int i = 0; i < count; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(onMs);
        digitalWrite(BUZZER_PIN, LOW);
        if (i < count - 1) delay(offMs);
    }
}

void motorVibrate(int onMs, int offMs, int count) {
    for (int i = 0; i < count; i++) {
        digitalWrite(MOTOR_PIN, HIGH);
        delay(onMs);
        digitalWrite(MOTOR_PIN, LOW);
        if (i < count - 1) delay(offMs);
    }
}

void handleSerialCommands() {
    if (!Serial.available()) return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "status") {
        printStatus();
    } else if (cmd == "gps") {
        Serial.printf("GPS: %.6f, %.6f | Alt: %.1f | Speed: %.1f | Sats: %d | Valid: %s\n",
                      gpsData.latitude, gpsData.longitude, gpsData.altitude,
                      gpsData.speed, gpsData.satellites,
                      gpsData.valid ? "Yes" : "No");
    } else if (cmd == "accel") {
        AccelData acc = accelerometer.getCurrentReading();
        Serial.printf("Accel: X=%.2f Y=%.2f Z=%.2f | Mag=%.2f | Steps=%u\n",
                      acc.x, acc.y, acc.z, acc.magnitude, activityData.stepCount);
    } else if (cmd == "walk") {
        Serial.printf("Walk State: %d | Duration: %u s | Distance: %.1f m | Grade: %d\n",
                      walkTracker.getState(), walkTracker.getWalkDuration(),
                      walkTracker.getWalkDistance(), walkTracker.getCurrentGrade());
    } else if (cmd == "startwalk") {
        walkTracker.startWalk();
        Serial.println("Walk started manually");
    } else if (cmd == "endwalk") {
        walkTracker.endWalk();
        Serial.println("Walk ended manually");
    } else if (cmd == "beep") {
        buzzerBeep(100, 100, 3);
    } else if (cmd == "vibrate") {
        motorVibrate(200, 100, 2);
    } else if (cmd == "sethome") {
        if (gpsData.valid) {
            walkTracker.setHomeLocation(gpsData.latitude, gpsData.longitude);
            gpsModule.setHomeLocation(gpsData.latitude, gpsData.longitude);
            Serial.printf("Home set to: %.6f, %.6f\n", gpsData.latitude, gpsData.longitude);
        } else {
            Serial.println("GPS not valid, cannot set home");
        }
    } else if (cmd == "reset") {
        ESP.restart();
    } else if (cmd == "help") {
        Serial.println("Commands:");
        Serial.println("  status    - Show device status");
        Serial.println("  gps       - Show GPS data");
        Serial.println("  accel     - Show accelerometer data");
        Serial.println("  walk      - Show walk status");
        Serial.println("  startwalk - Start walk manually");
        Serial.println("  endwalk   - End walk manually");
        Serial.println("  beep      - Test buzzer");
        Serial.println("  vibrate   - Test motor");
        Serial.println("  sethome   - Set current location as home");
        Serial.println("  reset     - Restart device");
    }
}

void printStatus() {
    Serial.println("\n┌─────────────────────────────────────────┐");
    Serial.println("│            POPSTRACKER STATUS           │");
    Serial.println("├─────────────────────────────────────────┤");

    // Battery
    Serial.printf("│ Battery: %.2fV (%d%%)                   \n",
                  deviceStatus.batteryVoltage, deviceStatus.batteryPercent);

    // GPS
    if (gpsData.valid) {
        Serial.printf("│ GPS: %.5f, %.5f (%d sats)     \n",
                      gpsData.latitude, gpsData.longitude, gpsData.satellites);
    } else {
        Serial.println("│ GPS: Searching...                       │");
    }

    // Activity
    const char* levels[] = {"Resting", "Light", "Active", "Very Active"};
    Serial.printf("│ Activity: %s | Steps: %u          \n",
                  levels[activityData.level], activityData.stepCount);

    // Walk status
    if (walkTracker.isWalkInProgress()) {
        const char* grades[] = {"A", "B", "C", "F"};
        Serial.printf("│ WALK: %u min | %.0fm | Grade %s         \n",
                      walkTracker.getWalkDuration() / 60,
                      walkTracker.getWalkDistance(),
                      grades[walkTracker.getCurrentGrade()]);
    } else {
        Serial.println("│ Walk: Idle                              │");
    }

    // Connectivity
    Serial.printf("│ Signal: %d | Connected: %s            \n",
                  deviceStatus.signalStrength,
                  cellular.isConnected() ? "Yes" : "No");

    Serial.println("└─────────────────────────────────────────┘");
}
