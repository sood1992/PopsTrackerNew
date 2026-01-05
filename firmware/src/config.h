/**
 * PopsTracker Dog Collar - Configuration Header
 *
 * Hardware: LILYGO T-A7670G R2 with ESP32
 * Features: GPS tracking, activity monitoring, walker accountability
 *
 * Pin configuration for onboard L76K GPS usage
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// DEVICE IDENTIFICATION
// ============================================================================
#define DEVICE_NAME         "PopcornTracker"
#define DEVICE_ID           "POPCORN_001"       // Unique device ID
#define FIRMWARE_VERSION    "1.0.0"
#define DOG_NAME            "Popcorn"

// ============================================================================
// SERVER CONFIGURATION
// ============================================================================
#define SERVER_HOST         "your-server.com"    // Replace with your server
#define SERVER_PORT         443
#define SERVER_PATH         "/api/v1/tracking"
#define USE_HTTPS           true

// API endpoints
#define API_LOCATION        "/api/v1/location"
#define API_ACTIVITY        "/api/v1/activity"
#define API_WALK            "/api/v1/walk"
#define API_HEARTBEAT       "/api/v1/heartbeat"
#define API_ALERT           "/api/v1/alert"

// ============================================================================
// CELLULAR MODEM (A7670G) - Internal connections
// ============================================================================
#define MODEM_TX            26      // ESP32 TX -> Modem RX
#define MODEM_RX            27      // ESP32 RX <- Modem TX
#define MODEM_PWRKEY        4       // Power key (pulse to toggle power)
#define MODEM_RST           5       // Reset pin
#define MODEM_POWER_ON      12      // Power enable / Board power hold
// Note: MODEM_DTR and MODEM_RI are optional and not used in this project
// to avoid conflicts with MOTOR_PIN (GPIO 25) and I2C_SDA (GPIO 33)
// #define MODEM_DTR           25      // Data Terminal Ready (NOT USED)
// #define MODEM_RI            33      // Ring Indicator (NOT USED)

// Modem serial settings
#define MODEM_BAUD          115200
#define SerialAT            Serial1

// APN Settings (Airtel India IoT)
#define APN_NAME            "airteliot.com"      // or "airtelgprs.com" for consumer
#define APN_USER            ""
#define APN_PASS            ""

// ============================================================================
// GPS MODULE (L76K) - Onboard GPS via UART
// ============================================================================
// Note: L76K GPS uses GPIO 21/22, so I2C must use alternative pins
#define GPS_TX              21      // ESP32 RX <- GPS TX (receive from GPS)
#define GPS_RX              22      // ESP32 TX -> GPS RX (send to GPS)
#define GPS_BAUD            9600    // L76K default baud rate
#define SerialGPS           Serial2

// GPS update intervals
#define GPS_UPDATE_INTERVAL_ACTIVE      5000    // 5 seconds during walk
#define GPS_UPDATE_INTERVAL_NORMAL      30000   // 30 seconds idle
#define GPS_UPDATE_INTERVAL_SLEEP       300000  // 5 minutes power save

// Geofencing (Home zone)
#define HOME_LATITUDE       0.0     // Set your home latitude
#define HOME_LONGITUDE      0.0     // Set your home longitude
#define HOME_RADIUS_METERS  50      // Radius for "at home" detection

// ============================================================================
// I2C BUS (OLED + ADXL345)
// ============================================================================
// Since GPS uses GPIO 21/22, I2C must use alternative pins
#define I2C_SDA             33      // I2C Data (moved from 21)
#define I2C_SCL             23      // I2C Clock (moved from 22)
#define I2C_FREQ            400000  // 400kHz I2C clock

// ============================================================================
// ACCELEROMETER (ADXL345)
// ============================================================================
#define ADXL345_ADDR        0x53    // I2C address (SDO tied to GND)
#define ADXL345_INT1        32      // Interrupt 1 (shake-to-wake)

// Activity detection thresholds
#define ACCEL_SAMPLE_RATE   50      // Samples per second
#define STEP_THRESHOLD      1.2     // G-force threshold for step detection
#define MOVEMENT_THRESHOLD  0.3     // G-force threshold for movement
#define SCRATCH_THRESHOLD   2.5     // G-force for scratching detection
#define VERY_ACTIVE_THRESH  2.0     // Very active threshold
#define ACTIVE_THRESH       1.0     // Active threshold
#define LIGHT_THRESH        0.5     // Light activity threshold

// Activity detection windows
#define ACTIVITY_WINDOW_MS  1000    // 1 second window for activity level
#define STEP_DEBOUNCE_MS    200     // Minimum time between steps

// ============================================================================
// OLED DISPLAY (SSD1306)
// ============================================================================
#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_ADDR           0x3C    // I2C address (or 0x3D)
#define OLED_RESET          -1      // No reset pin

// ============================================================================
// MICROSD CARD (SPI)
// ============================================================================
#define SD_CS               13      // Chip Select
#define SD_SCK              14      // SPI Clock
#define SD_MOSI             15      // Master Out Slave In
#define SD_MISO             2       // Master In Slave Out (strapping pin!)

// ============================================================================
// BUZZER
// ============================================================================
#define BUZZER_PIN          19      // Active HIGH
#define BUZZER_FREQ         2000    // Default frequency (Hz)
#define BUZZER_CHANNEL      0       // PWM channel

// Buzzer patterns (on_ms, off_ms, repeats)
#define BEEP_SHORT          100, 100, 1
#define BEEP_LONG           500, 100, 1
#define BEEP_ALERT          200, 100, 3
#define BEEP_SOS            100, 100, 3     // Then 300, 100, 3 then 100, 100, 3
#define BEEP_WALK_START     150, 100, 2
#define BEEP_WALK_END       300, 100, 1
#define BEEP_LOW_BATTERY    500, 500, 2

// ============================================================================
// VIBRATION MOTOR
// ============================================================================
#define MOTOR_PIN           25      // Via transistor/MOSFET
#define MOTOR_CHANNEL       1       // PWM channel

// Vibration patterns (on_ms, off_ms, repeats)
#define VIBRATE_SHORT       100, 100, 1
#define VIBRATE_LONG        500, 100, 1
#define VIBRATE_ALERT       200, 100, 3
#define VIBRATE_FIND_ME     300, 200, 5

// ============================================================================
// BATTERY MONITORING
// ============================================================================
#define BAT_ADC_PIN         35      // ADC input (input-only pin)
#define BAT_ADC_SAMPLES     10      // Number of samples for averaging

// Battery voltage thresholds (assuming 50% voltage divider)
// Actual voltage = ADC reading × 2 × 3.3 / 4095
#define BAT_VOLTAGE_FULL    4.2     // 100%
#define BAT_VOLTAGE_NOMINAL 3.7     // ~50%
#define BAT_VOLTAGE_LOW     3.3     // ~20% - warn user
#define BAT_VOLTAGE_CRIT    3.0     // ~5% - enter sleep mode

// ============================================================================
// WALK DETECTION & SCORING
// ============================================================================

// Walk detection
#define WALK_START_DISTANCE_M   30      // Must move 30m from home to start walk
#define WALK_MIN_DURATION_S     120     // Minimum 2 minutes to count as walk
#define WALK_END_STATIONARY_S   300     // 5 minutes stationary = walk ended
#define WALK_MAX_STATIONARY_S   600     // 10 min stationary during walk = pause

// Walk scoring thresholds (percentage of active time)
#define GRADE_A_THRESHOLD   70      // >= 70% active = A (Excellent)
#define GRADE_B_THRESHOLD   50      // >= 50% active = B (Good)
#define GRADE_C_THRESHOLD   30      // >= 30% active = C (Acceptable)
                                    // < 30% = F (Walker slacking!)

// Activity classification during walk
#define STATIONARY_SPEED_KMH    0.5     // Speed below this = stationary
#define WALKING_SPEED_KMH       1.0     // Normal walking speed
#define RUNNING_SPEED_KMH       5.0     // Running/playing

// Cheating detection
#define CARRIED_ACCEL_THRESH    0.2     // Very low accel variation = being carried
#define CAR_SPEED_KMH           15.0    // Speed too fast = in vehicle

// ============================================================================
// DATA LOGGING & SYNC
// ============================================================================
#define LOG_INTERVAL_MS         1000    // Log data every 1 second
#define SYNC_INTERVAL_MS        30000   // Sync to server every 30 seconds
#define HEARTBEAT_INTERVAL_MS   60000   // Heartbeat every 1 minute

// Local storage
#define MAX_OFFLINE_RECORDS     1000    // Max records before forcing sync
#define LOG_FILE_PREFIX         "/walk_"
#define LOG_FILE_EXT            ".csv"

// ============================================================================
// POWER MANAGEMENT
// ============================================================================
#define SLEEP_AFTER_IDLE_MS     300000  // Enter light sleep after 5 min idle
#define DEEP_SLEEP_THRESHOLD    1800000 // Deep sleep after 30 min no movement

// Wake sources
#define WAKE_ON_MOVEMENT        true
#define WAKE_ON_TIMER           true
#define WAKE_TIMER_INTERVAL_S   300     // Wake every 5 minutes for GPS fix

// ============================================================================
// DEBUG & DEVELOPMENT
// ============================================================================
#ifdef POPS_DEBUG
    #define DEBUG_PRINT(x)      Serial.print(x)
    #define DEBUG_PRINTLN(x)    Serial.println(x)
    #define DEBUG_PRINTF(...)   Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

#define SERIAL_BAUD             115200

// ============================================================================
// ACTIVITY LEVELS
// ============================================================================
enum ActivityLevel {
    ACTIVITY_RESTING = 0,
    ACTIVITY_LIGHT = 1,
    ACTIVITY_ACTIVE = 2,
    ACTIVITY_VERY_ACTIVE = 3
};

// Walk grades
enum WalkGrade {
    GRADE_A = 0,    // Excellent (>= 70% active)
    GRADE_B = 1,    // Good (>= 50% active)
    GRADE_C = 2,    // Acceptable (>= 30% active)
    GRADE_F = 3     // Failing (< 30% active)
};

// Walk states
enum WalkState {
    WALK_IDLE = 0,          // At home, no walk
    WALK_STARTING = 1,      // Leaving home zone
    WALK_ACTIVE = 2,        // Walk in progress
    WALK_PAUSED = 3,        // Temporarily stationary
    WALK_ENDING = 4,        // Returning home
    WALK_COMPLETED = 5      // Walk finished
};

// Device states
enum DeviceState {
    STATE_BOOT = 0,
    STATE_IDLE = 1,
    STATE_TRACKING = 2,
    STATE_WALK = 3,
    STATE_ALERT = 4,
    STATE_SLEEP = 5,
    STATE_CHARGING = 6,
    STATE_ERROR = 7
};

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// GPS data
struct GPSData {
    double latitude;
    double longitude;
    double altitude;
    double speed;           // km/h
    double course;          // degrees
    uint8_t satellites;
    uint32_t hdop;          // Horizontal dilution of precision × 100
    bool valid;
    uint32_t timestamp;     // millis() when acquired
    uint32_t age;           // Age of fix in ms
};

// Accelerometer data
struct AccelData {
    float x, y, z;          // Raw G values
    float magnitude;        // Combined magnitude
    uint32_t timestamp;
};

// Activity data
struct ActivityData {
    uint32_t stepCount;
    uint32_t activeMinutes;
    ActivityLevel level;
    bool isMoving;
    bool isSleeping;
    bool isScratching;      // Potential skin issue indicator
    uint32_t lastActivityMs;
};

// Walk session data
struct WalkSession {
    uint32_t walkId;
    uint32_t startTime;     // Unix timestamp
    uint32_t endTime;       // Unix timestamp
    uint32_t duration;      // Seconds
    double distanceMeters;
    double avgSpeed;        // km/h
    uint32_t activeSeconds;
    uint32_t stationarySeconds;
    uint32_t steps;
    WalkGrade grade;
    WalkState state;
    double startLat, startLon;
    double endLat, endLon;
    double maxSpeed;
    bool wasCarried;        // Detected being carried
    bool wasInVehicle;      // Detected in car
    uint8_t pauseCount;     // Number of pauses
};

// Device status
struct DeviceStatus {
    DeviceState state;
    float batteryVoltage;
    uint8_t batteryPercent;
    bool isCharging;
    bool gpsConnected;
    bool modemConnected;
    bool serverConnected;
    int signalStrength;     // RSSI
    uint32_t uptime;
    uint32_t lastSyncTime;
};

#endif // CONFIG_H
