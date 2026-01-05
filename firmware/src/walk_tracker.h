/**
 * PopsTracker - Walk Tracker Module
 *
 * Handles walk detection, scoring, and walker accountability
 *
 * Walk Scoring System:
 * - Grade A (>= 70% active): Excellent walk
 * - Grade B (>= 50% active): Good walk
 * - Grade C (>= 30% active): Acceptable
 * - Grade F (< 30% active): Walker slacking!
 *
 * Cheating Detection:
 * - Dog being carried (low accel variance + GPS movement)
 * - In vehicle (speed > 15 km/h)
 * - Too many pauses
 */

#ifndef WALK_TRACKER_H
#define WALK_TRACKER_H

#include "config.h"
#include "gps.h"
#include "accelerometer.h"

class WalkTracker {
public:
    WalkTracker();

    void begin();
    void update(GPSData& gps, ActivityData& activity);

    // Walk state
    WalkState getState();
    bool isWalkInProgress();
    bool isWalkComplete();

    // Current walk data
    WalkSession getCurrentWalk();
    uint32_t getWalkDuration();
    double getWalkDistance();
    WalkGrade getCurrentGrade();
    float getActivePercent();

    // Manual controls
    void startWalk();
    void endWalk();
    void pauseWalk();
    void resumeWalk();

    // Configuration
    void setHomeLocation(double lat, double lon);
    void setWalkerPhone(const char* phone);

    // History
    uint16_t getTotalWalksToday();
    uint32_t getTotalActiveMinutesToday();
    WalkSession getLastWalk();

    // Walker accountability
    bool wasWalkerSlacking();
    String generateWalkReport();
    String generateWeeklyReport();

private:
    WalkSession currentWalk;
    WalkSession lastWalk;
    WalkState state;

    // Home location
    double homeLat, homeLon;

    // Walk detection
    bool leftHomeZone;
    uint32_t leftHomeTime;
    uint32_t lastMovementTime;
    uint32_t pauseStartTime;

    // Stats tracking
    uint32_t activeTime;
    uint32_t stationaryTime;
    uint32_t carriedTime;
    uint32_t vehicleTime;

    // Speed tracking
    double maxSpeed;
    double speedSum;
    uint32_t speedSamples;

    // Cheating detection
    bool currentlyCarried;
    bool currentlyInVehicle;
    uint8_t carriedEvents;
    uint8_t vehicleEvents;

    // Daily stats
    uint16_t walksToday;
    uint32_t activeMinutesToday;
    uint32_t lastDayReset;

    // Walker info
    char walkerPhone[20];

    // Helper methods
    void detectWalkStart(GPSData& gps);
    void trackWalkProgress(GPSData& gps, ActivityData& activity);
    void detectWalkEnd(GPSData& gps);
    void detectCheating(GPSData& gps, ActivityData& activity);
    void calculateGrade();
    void finalizeWalk();
    void resetDailyStats();
    uint32_t generateWalkId();
};

extern WalkTracker walkTracker;

#endif // WALK_TRACKER_H
