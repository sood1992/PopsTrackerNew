/**
 * PopsTracker - GPS Module (L76K)
 *
 * Handles GPS positioning, distance calculation, and geofencing
 */

#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include "config.h"

class GPSModule {
public:
    GPSModule();

    bool begin();
    void update();

    // Position data
    GPSData getCurrentPosition();
    bool hasValidFix();
    uint32_t getFixAge();

    // Distance and speed
    double getDistanceFromHome();
    double getDistanceTo(double lat, double lon);
    double getSpeed();          // km/h
    double getCourse();         // degrees

    // Geofencing
    bool isAtHome();
    bool isOutsideHomeZone();
    void setHomeLocation(double lat, double lon);
    void setHomeRadius(double meters);

    // Walk tracking
    void startWalkTracking();
    void stopWalkTracking();
    double getTotalWalkDistance();
    void resetWalkDistance();

    // Route recording
    void startRouteRecording();
    void stopRouteRecording();
    uint16_t getRoutePointCount();

    // Power management
    void enterLowPowerMode();
    void exitLowPowerMode();
    void setUpdateInterval(uint32_t intervalMs);

    // Satellite info
    uint8_t getSatelliteCount();
    uint32_t getHDOP();

private:
    TinyGPSPlus gps;
    HardwareSerial* gpsSerial;

    GPSData currentPosition;

    // Home location
    double homeLat, homeLon;
    double homeRadius;

    // Walk tracking
    bool trackingWalk;
    double walkDistance;
    double lastLat, lastLon;
    bool hasLastPosition;

    // Route recording
    bool recordingRoute;
    uint16_t routePointCount;

    // Update timing
    uint32_t updateInterval;
    uint32_t lastUpdateTime;

    // Helper methods
    void parseGPS();
    double haversineDistance(double lat1, double lon1, double lat2, double lon2);
    double degreesToRadians(double degrees);
};

extern GPSModule gpsModule;

#endif // GPS_MODULE_H
