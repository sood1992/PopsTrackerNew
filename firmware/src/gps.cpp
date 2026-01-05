/**
 * PopsTracker - GPS Module Implementation
 */

#include "gps.h"

// Global instance
GPSModule gpsModule;

GPSModule::GPSModule() :
    gpsSerial(&Serial2),
    homeLat(HOME_LATITUDE),
    homeLon(HOME_LONGITUDE),
    homeRadius(HOME_RADIUS_METERS),
    trackingWalk(false),
    walkDistance(0),
    lastLat(0), lastLon(0),
    hasLastPosition(false),
    recordingRoute(false),
    routePointCount(0),
    updateInterval(GPS_UPDATE_INTERVAL_NORMAL),
    lastUpdateTime(0)
{
    memset(&currentPosition, 0, sizeof(GPSData));
}

bool GPSModule::begin() {
    DEBUG_PRINTLN("Initializing GPS module (L76K)...");

    // Initialize GPS serial on GPIO 16 (RX) and GPIO 17 (TX)
    // Note: For L76K onboard, the pins are actually:
    // ESP32 RX (GPIO 21) <- L76K TX
    // ESP32 TX (GPIO 22) -> L76K RX
    gpsSerial->begin(GPS_BAUD, SERIAL_8N1, GPS_TX, GPS_RX);

    // Wait for GPS to initialize
    delay(1000);

    DEBUG_PRINTLN("GPS module initialized");
    DEBUG_PRINTF("Baud rate: %d\n", GPS_BAUD);

    // Check if we're receiving data
    uint32_t startTime = millis();
    while (millis() - startTime < 2000) {
        while (gpsSerial->available()) {
            char c = gpsSerial->read();
            if (c == '$') {
                DEBUG_PRINTLN("GPS NMEA data detected");
                return true;
            }
        }
        delay(10);
    }

    DEBUG_PRINTLN("WARNING: No GPS data received. Check antenna!");
    return true;  // Return true anyway, GPS might just need time for fix
}

void GPSModule::update() {
    uint32_t currentTime = millis();

    // Rate limit updates
    if (currentTime - lastUpdateTime < 100) {
        return;
    }

    // Read all available GPS data
    while (gpsSerial->available()) {
        char c = gpsSerial->read();
        gps.encode(c);
    }

    // Update position if we have new valid data
    if (gps.location.isUpdated() && gps.location.isValid()) {
        double newLat = gps.location.lat();
        double newLon = gps.location.lng();

        // Calculate distance from last position (for walk tracking)
        if (trackingWalk && hasLastPosition) {
            double dist = haversineDistance(lastLat, lastLon, newLat, newLon);

            // Only count movement > 2 meters (filter GPS noise)
            if (dist > 2.0) {
                walkDistance += dist;
                lastLat = newLat;
                lastLon = newLon;

                if (recordingRoute) {
                    routePointCount++;
                }
            }
        } else if (trackingWalk) {
            lastLat = newLat;
            lastLon = newLon;
            hasLastPosition = true;
        }

        // Update current position
        currentPosition.latitude = newLat;
        currentPosition.longitude = newLon;
        currentPosition.valid = true;
        currentPosition.timestamp = currentTime;
        currentPosition.age = 0;
    }

    // Update other GPS data
    if (gps.altitude.isValid()) {
        currentPosition.altitude = gps.altitude.meters();
    }

    if (gps.speed.isValid()) {
        currentPosition.speed = gps.speed.kmph();
    }

    if (gps.course.isValid()) {
        currentPosition.course = gps.course.deg();
    }

    if (gps.satellites.isValid()) {
        currentPosition.satellites = gps.satellites.value();
    }

    if (gps.hdop.isValid()) {
        currentPosition.hdop = gps.hdop.value();
    }

    // Update fix age
    if (currentPosition.valid) {
        currentPosition.age = currentTime - currentPosition.timestamp;
    }

    lastUpdateTime = currentTime;
}

GPSData GPSModule::getCurrentPosition() {
    return currentPosition;
}

bool GPSModule::hasValidFix() {
    return currentPosition.valid && currentPosition.age < 5000;  // Fix less than 5 seconds old
}

uint32_t GPSModule::getFixAge() {
    if (!currentPosition.valid) return UINT32_MAX;
    return millis() - currentPosition.timestamp;
}

double GPSModule::getDistanceFromHome() {
    if (!hasValidFix() || homeLat == 0 || homeLon == 0) {
        return -1;
    }
    return haversineDistance(currentPosition.latitude, currentPosition.longitude,
                             homeLat, homeLon);
}

double GPSModule::getDistanceTo(double lat, double lon) {
    if (!hasValidFix()) return -1;
    return haversineDistance(currentPosition.latitude, currentPosition.longitude,
                             lat, lon);
}

double GPSModule::getSpeed() {
    return currentPosition.speed;
}

double GPSModule::getCourse() {
    return currentPosition.course;
}

bool GPSModule::isAtHome() {
    double distance = getDistanceFromHome();
    return distance >= 0 && distance < homeRadius;
}

bool GPSModule::isOutsideHomeZone() {
    double distance = getDistanceFromHome();
    return distance >= 0 && distance >= homeRadius;
}

void GPSModule::setHomeLocation(double lat, double lon) {
    homeLat = lat;
    homeLon = lon;
    DEBUG_PRINTF("Home location set: %.6f, %.6f\n", lat, lon);
}

void GPSModule::setHomeRadius(double meters) {
    homeRadius = meters;
    DEBUG_PRINTF("Home radius set: %.1f meters\n", meters);
}

void GPSModule::startWalkTracking() {
    DEBUG_PRINTLN("Starting walk tracking");
    trackingWalk = true;
    walkDistance = 0;
    hasLastPosition = false;

    if (hasValidFix()) {
        lastLat = currentPosition.latitude;
        lastLon = currentPosition.longitude;
        hasLastPosition = true;
    }
}

void GPSModule::stopWalkTracking() {
    DEBUG_PRINTF("Stopping walk tracking. Total distance: %.2f meters\n", walkDistance);
    trackingWalk = false;
}

double GPSModule::getTotalWalkDistance() {
    return walkDistance;
}

void GPSModule::resetWalkDistance() {
    walkDistance = 0;
    hasLastPosition = false;
}

void GPSModule::startRouteRecording() {
    recordingRoute = true;
    routePointCount = 0;
    DEBUG_PRINTLN("Route recording started");
}

void GPSModule::stopRouteRecording() {
    recordingRoute = false;
    DEBUG_PRINTF("Route recording stopped. Points: %d\n", routePointCount);
}

uint16_t GPSModule::getRoutePointCount() {
    return routePointCount;
}

void GPSModule::enterLowPowerMode() {
    // Send command to put L76K into standby
    // L76K command: $PMTK161,0*28
    gpsSerial->println("$PMTK161,0*28");
    DEBUG_PRINTLN("GPS entering low power mode");
}

void GPSModule::exitLowPowerMode() {
    // Send any character to wake L76K
    gpsSerial->println("");
    delay(100);
    // Optionally send hot start command: $PMTK101*32
    gpsSerial->println("$PMTK101*32");
    DEBUG_PRINTLN("GPS exiting low power mode");
}

void GPSModule::setUpdateInterval(uint32_t intervalMs) {
    updateInterval = intervalMs;

    // L76K supports 1Hz (1000ms), 2Hz (500ms), 5Hz (200ms), 10Hz (100ms)
    // Command: $PMTK220,<interval>*checksum
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "$PMTK220,%u", intervalMs);

    // Calculate checksum
    uint8_t checksum = 0;
    for (int i = 1; cmd[i] != '\0'; i++) {
        checksum ^= cmd[i];
    }

    char fullCmd[40];
    snprintf(fullCmd, sizeof(fullCmd), "%s*%02X", cmd, checksum);
    gpsSerial->println(fullCmd);

    DEBUG_PRINTF("GPS update interval set to %u ms\n", intervalMs);
}

uint8_t GPSModule::getSatelliteCount() {
    return currentPosition.satellites;
}

uint32_t GPSModule::getHDOP() {
    return currentPosition.hdop;
}

// Haversine formula for calculating distance between two GPS coordinates
double GPSModule::haversineDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000;  // Earth's radius in meters

    double dLat = degreesToRadians(lat2 - lat1);
    double dLon = degreesToRadians(lon2 - lon1);

    lat1 = degreesToRadians(lat1);
    lat2 = degreesToRadians(lat2);

    double a = sin(dLat / 2) * sin(dLat / 2) +
               sin(dLon / 2) * sin(dLon / 2) * cos(lat1) * cos(lat2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return R * c;
}

double GPSModule::degreesToRadians(double degrees) {
    return degrees * M_PI / 180.0;
}
