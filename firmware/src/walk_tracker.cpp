/**
 * PopsTracker - Walk Tracker Implementation
 */

#include "walk_tracker.h"

// Global instance
WalkTracker walkTracker;

WalkTracker::WalkTracker() :
    state(WALK_IDLE),
    homeLat(HOME_LATITUDE),
    homeLon(HOME_LONGITUDE),
    leftHomeZone(false),
    leftHomeTime(0),
    lastMovementTime(0),
    pauseStartTime(0),
    activeTime(0),
    stationaryTime(0),
    carriedTime(0),
    vehicleTime(0),
    maxSpeed(0),
    speedSum(0),
    speedSamples(0),
    currentlyCarried(false),
    currentlyInVehicle(false),
    carriedEvents(0),
    vehicleEvents(0),
    walksToday(0),
    activeMinutesToday(0),
    lastDayReset(0)
{
    memset(&currentWalk, 0, sizeof(WalkSession));
    memset(&lastWalk, 0, sizeof(WalkSession));
    memset(walkerPhone, 0, sizeof(walkerPhone));
}

void WalkTracker::begin() {
    DEBUG_PRINTLN("Walk tracker initialized");
    resetDailyStats();
}

void WalkTracker::update(GPSData& gps, ActivityData& activity) {
    // Reset daily stats at midnight
    uint32_t now = millis();
    if (now - lastDayReset > 86400000) {  // 24 hours in ms
        resetDailyStats();
    }

    // State machine for walk tracking
    switch (state) {
        case WALK_IDLE:
            detectWalkStart(gps);
            break;

        case WALK_STARTING:
            // Confirm walk has really started
            if (gps.valid && gpsModule.getDistanceFromHome() > WALK_START_DISTANCE_M) {
                state = WALK_ACTIVE;
                DEBUG_PRINTLN("Walk confirmed - now active");
            } else if (now - leftHomeTime > 60000) {
                // False start - back to idle
                state = WALK_IDLE;
                DEBUG_PRINTLN("Walk start cancelled - didn't leave home zone");
            }
            break;

        case WALK_ACTIVE:
            trackWalkProgress(gps, activity);
            detectCheating(gps, activity);
            detectWalkEnd(gps);
            break;

        case WALK_PAUSED:
            if (activity.isMoving || gps.speed > STATIONARY_SPEED_KMH) {
                // Resumed walking
                state = WALK_ACTIVE;
                currentWalk.pauseCount++;
                DEBUG_PRINTLN("Walk resumed from pause");
            } else if (now - pauseStartTime > WALK_END_STATIONARY_S * 1000) {
                // Too long stationary - end walk
                state = WALK_ENDING;
                DEBUG_PRINTLN("Walk ending due to extended pause");
            }
            break;

        case WALK_ENDING:
            finalizeWalk();
            state = WALK_COMPLETED;
            break;

        case WALK_COMPLETED:
            // Wait for reset or new walk
            if (gpsModule.isAtHome()) {
                state = WALK_IDLE;
            }
            break;
    }
}

void WalkTracker::detectWalkStart(GPSData& gps) {
    if (!gps.valid) return;

    double distFromHome = gpsModule.getDistanceFromHome();

    // Detect leaving home zone
    if (distFromHome > HOME_RADIUS_METERS && !leftHomeZone) {
        leftHomeZone = true;
        leftHomeTime = millis();
        state = WALK_STARTING;

        // Initialize walk session
        memset(&currentWalk, 0, sizeof(WalkSession));
        currentWalk.walkId = generateWalkId();
        currentWalk.startTime = millis();
        currentWalk.startLat = gps.latitude;
        currentWalk.startLon = gps.longitude;
        currentWalk.state = WALK_STARTING;

        // Reset tracking variables
        activeTime = 0;
        stationaryTime = 0;
        carriedTime = 0;
        vehicleTime = 0;
        maxSpeed = 0;
        speedSum = 0;
        speedSamples = 0;
        carriedEvents = 0;
        vehicleEvents = 0;

        // Start GPS tracking
        gpsModule.startWalkTracking();
        gpsModule.startRouteRecording();

        DEBUG_PRINTLN("Walk starting - left home zone");
        DEBUG_PRINTF("Start location: %.6f, %.6f\n", gps.latitude, gps.longitude);
    }
}

void WalkTracker::trackWalkProgress(GPSData& gps, ActivityData& activity) {
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    uint32_t elapsed = now - lastUpdate;

    if (elapsed < 1000) return;  // Update every second
    lastUpdate = now;

    // Track active vs stationary time
    if (activity.isMoving && gps.speed > STATIONARY_SPEED_KMH) {
        activeTime += elapsed / 1000;
        lastMovementTime = now;
    } else {
        stationaryTime += elapsed / 1000;

        // Check for pause
        if (now - lastMovementTime > WALK_MAX_STATIONARY_S * 1000 / 2) {
            if (state == WALK_ACTIVE) {
                state = WALK_PAUSED;
                pauseStartTime = now;
                DEBUG_PRINTLN("Walk paused - stationary");
            }
        }
    }

    // Track speed
    if (gps.speed > 0.1) {
        speedSum += gps.speed;
        speedSamples++;

        if (gps.speed > maxSpeed) {
            maxSpeed = gps.speed;
        }
    }

    // Update walk session
    currentWalk.duration = (now - currentWalk.startTime) / 1000;
    currentWalk.distanceMeters = gpsModule.getTotalWalkDistance();
    currentWalk.activeSeconds = activeTime;
    currentWalk.stationarySeconds = stationaryTime;
    currentWalk.steps = activity.stepCount;
    currentWalk.maxSpeed = maxSpeed;

    if (speedSamples > 0) {
        currentWalk.avgSpeed = speedSum / speedSamples;
    }

    // Calculate current grade
    calculateGrade();
}

void WalkTracker::detectCheating(GPSData& gps, ActivityData& activity) {
    static uint32_t lastCheck = 0;
    uint32_t now = millis();

    if (now - lastCheck < 5000) return;  // Check every 5 seconds
    lastCheck = now;

    // Check if dog is being carried
    // Signs: GPS shows movement, but accelerometer shows very low variance
    bool isCarried = accelerometer.isCarried() && gps.speed > WALKING_SPEED_KMH;

    if (isCarried && !currentlyCarried) {
        currentlyCarried = true;
        carriedEvents++;
        DEBUG_PRINTLN("CHEATING DETECTED: Dog appears to be carried!");
    } else if (!isCarried && currentlyCarried) {
        currentlyCarried = false;
    }

    if (currentlyCarried) {
        carriedTime += 5;
        currentWalk.wasCarried = true;
    }

    // Check if in vehicle
    // Signs: Speed too fast for walking
    bool inVehicle = gps.speed > CAR_SPEED_KMH;

    if (inVehicle && !currentlyInVehicle) {
        currentlyInVehicle = true;
        vehicleEvents++;
        DEBUG_PRINTLN("CHEATING DETECTED: Dog appears to be in vehicle!");
    } else if (!inVehicle && currentlyInVehicle) {
        currentlyInVehicle = false;
    }

    if (currentlyInVehicle) {
        vehicleTime += 5;
        currentWalk.wasInVehicle = true;
    }
}

void WalkTracker::detectWalkEnd(GPSData& gps) {
    if (!gps.valid) return;

    double distFromHome = gpsModule.getDistanceFromHome();
    uint32_t walkDuration = (millis() - currentWalk.startTime) / 1000;

    // Walk ends when:
    // 1. Back in home zone AND walked for minimum duration
    // 2. OR stationary for too long (handled in WALK_PAUSED state)

    if (distFromHome >= 0 && distFromHome < HOME_RADIUS_METERS * 2) {
        if (walkDuration >= WALK_MIN_DURATION_S) {
            state = WALK_ENDING;
            DEBUG_PRINTLN("Walk ending - returned to home zone");
        }
    }
}

void WalkTracker::calculateGrade() {
    uint32_t totalTime = activeTime + stationaryTime;
    if (totalTime == 0) {
        currentWalk.grade = GRADE_F;
        return;
    }

    float activePercent = (float)activeTime / totalTime * 100;

    // Penalties for cheating
    if (currentWalk.wasCarried) {
        activePercent -= 20;  // 20% penalty for being carried
    }
    if (currentWalk.wasInVehicle) {
        activePercent -= 30;  // 30% penalty for vehicle
    }

    // Too many pauses
    if (currentWalk.pauseCount > 5) {
        activePercent -= (currentWalk.pauseCount - 5) * 2;  // 2% per extra pause
    }

    // Clamp to 0-100
    if (activePercent < 0) activePercent = 0;
    if (activePercent > 100) activePercent = 100;

    // Assign grade
    if (activePercent >= GRADE_A_THRESHOLD) {
        currentWalk.grade = GRADE_A;
    } else if (activePercent >= GRADE_B_THRESHOLD) {
        currentWalk.grade = GRADE_B;
    } else if (activePercent >= GRADE_C_THRESHOLD) {
        currentWalk.grade = GRADE_C;
    } else {
        currentWalk.grade = GRADE_F;
    }
}

void WalkTracker::finalizeWalk() {
    DEBUG_PRINTLN("Finalizing walk...");

    // Stop GPS tracking
    gpsModule.stopWalkTracking();
    gpsModule.stopRouteRecording();

    // Final calculations
    GPSData gps = gpsModule.getCurrentPosition();
    currentWalk.endTime = millis();
    currentWalk.endLat = gps.latitude;
    currentWalk.endLon = gps.longitude;
    currentWalk.state = WALK_COMPLETED;

    // Final grade calculation
    calculateGrade();

    // Update daily stats
    walksToday++;
    activeMinutesToday += activeTime / 60;

    // Save as last walk
    lastWalk = currentWalk;

    // Reset home zone detection
    leftHomeZone = false;

    // Log walk summary
    const char* grades[] = {"A", "B", "C", "F"};
    DEBUG_PRINTLN("=== WALK SUMMARY ===");
    DEBUG_PRINTF("Duration: %u minutes\n", currentWalk.duration / 60);
    DEBUG_PRINTF("Distance: %.2f meters\n", currentWalk.distanceMeters);
    DEBUG_PRINTF("Steps: %u\n", currentWalk.steps);
    DEBUG_PRINTF("Active: %u seconds (%.1f%%)\n", activeTime,
                 (float)activeTime / (activeTime + stationaryTime) * 100);
    DEBUG_PRINTF("Grade: %s\n", grades[currentWalk.grade]);
    DEBUG_PRINTF("Carried: %s\n", currentWalk.wasCarried ? "YES!" : "No");
    DEBUG_PRINTF("In Vehicle: %s\n", currentWalk.wasInVehicle ? "YES!" : "No");
    DEBUG_PRINTLN("====================");
}

void WalkTracker::resetDailyStats() {
    walksToday = 0;
    activeMinutesToday = 0;
    lastDayReset = millis();
    DEBUG_PRINTLN("Daily stats reset");
}

uint32_t WalkTracker::generateWalkId() {
    // Simple ID generation using millis
    return (millis() / 1000) ^ (micros() & 0xFFFF);
}

// Public methods

WalkState WalkTracker::getState() {
    return state;
}

bool WalkTracker::isWalkInProgress() {
    return state == WALK_ACTIVE || state == WALK_PAUSED;
}

bool WalkTracker::isWalkComplete() {
    return state == WALK_COMPLETED;
}

WalkSession WalkTracker::getCurrentWalk() {
    return currentWalk;
}

uint32_t WalkTracker::getWalkDuration() {
    if (state == WALK_IDLE) return 0;
    return (millis() - currentWalk.startTime) / 1000;
}

double WalkTracker::getWalkDistance() {
    return currentWalk.distanceMeters;
}

WalkGrade WalkTracker::getCurrentGrade() {
    return currentWalk.grade;
}

float WalkTracker::getActivePercent() {
    uint32_t total = activeTime + stationaryTime;
    if (total == 0) return 0;
    return (float)activeTime / total * 100;
}

void WalkTracker::startWalk() {
    if (state != WALK_IDLE) return;

    // Manual walk start
    GPSData gps = gpsModule.getCurrentPosition();

    memset(&currentWalk, 0, sizeof(WalkSession));
    currentWalk.walkId = generateWalkId();
    currentWalk.startTime = millis();
    currentWalk.startLat = gps.latitude;
    currentWalk.startLon = gps.longitude;

    state = WALK_ACTIVE;
    leftHomeZone = true;
    leftHomeTime = millis();

    gpsModule.startWalkTracking();
    gpsModule.startRouteRecording();

    DEBUG_PRINTLN("Walk manually started");
}

void WalkTracker::endWalk() {
    if (state == WALK_IDLE || state == WALK_COMPLETED) return;

    state = WALK_ENDING;
    DEBUG_PRINTLN("Walk manually ended");
}

void WalkTracker::pauseWalk() {
    if (state == WALK_ACTIVE) {
        state = WALK_PAUSED;
        pauseStartTime = millis();
        DEBUG_PRINTLN("Walk manually paused");
    }
}

void WalkTracker::resumeWalk() {
    if (state == WALK_PAUSED) {
        state = WALK_ACTIVE;
        currentWalk.pauseCount++;
        DEBUG_PRINTLN("Walk manually resumed");
    }
}

void WalkTracker::setHomeLocation(double lat, double lon) {
    homeLat = lat;
    homeLon = lon;
    gpsModule.setHomeLocation(lat, lon);
}

void WalkTracker::setWalkerPhone(const char* phone) {
    strncpy(walkerPhone, phone, sizeof(walkerPhone) - 1);
}

uint16_t WalkTracker::getTotalWalksToday() {
    return walksToday;
}

uint32_t WalkTracker::getTotalActiveMinutesToday() {
    return activeMinutesToday;
}

WalkSession WalkTracker::getLastWalk() {
    return lastWalk;
}

bool WalkTracker::wasWalkerSlacking() {
    return lastWalk.grade == GRADE_F;
}

String WalkTracker::generateWalkReport() {
    const char* grades[] = {"A - Excellent!", "B - Good", "C - Acceptable", "F - SLACKING!"};

    String report = "=== " + String(DOG_NAME) + "'s Walk Report ===\n";
    report += "Duration: " + String(lastWalk.duration / 60) + " min\n";
    report += "Distance: " + String(lastWalk.distanceMeters, 0) + " m\n";
    report += "Steps: " + String(lastWalk.steps) + "\n";
    report += "Active: " + String(lastWalk.activeSeconds / 60) + " min\n";
    report += "Grade: " + String(grades[lastWalk.grade]) + "\n";

    if (lastWalk.wasCarried) {
        report += "WARNING: Dog was carried!\n";
    }
    if (lastWalk.wasInVehicle) {
        report += "WARNING: Dog was in vehicle!\n";
    }

    return report;
}

String WalkTracker::generateWeeklyReport() {
    // This would need persistent storage to track weekly data
    // For now, return today's summary

    String report = "=== Weekly Walker Report Card ===\n";
    report += "Today's Walks: " + String(walksToday) + "\n";
    report += "Today's Active Minutes: " + String(activeMinutesToday) + "\n";

    if (lastWalk.walkId != 0) {
        const char* grades[] = {"A", "B", "C", "F"};
        report += "Last Walk Grade: " + String(grades[lastWalk.grade]) + "\n";
    }

    return report;
}
