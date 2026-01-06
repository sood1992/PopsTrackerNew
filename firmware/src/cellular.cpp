/**
 * PopsTracker - Cellular Module Implementation
 *
 * Uses lewisxhe TinyGSM fork with built-in HTTPS support
 */

#include "cellular.h"

// Global instance
CellularModule cellular;

CellularModule::CellularModule() :
    modemSerial(&Serial1),
    modem(*modemSerial),
    modemReady(false),
    networkConnected(false),
    signalStrength(0),
    lastConnectAttempt(0)
{
}

bool CellularModule::begin() {
    DEBUG_PRINTLN("Initializing A7670G cellular modem...");

    // Configure modem control pins
    pinMode(MODEM_PWRKEY, OUTPUT);
    pinMode(MODEM_POWER_ON, OUTPUT);
    pinMode(MODEM_RST, OUTPUT);

    // Ensure modem is powered
    digitalWrite(MODEM_POWER_ON, HIGH);
    delay(100);

    // Initialize modem serial
    modemSerial->begin(MODEM_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);
    delay(500);

    // Power on the modem
    if (!powerOn()) {
        DEBUG_PRINTLN("ERROR: Failed to power on modem");
        return false;
    }

    // Wait for modem to be ready
    DEBUG_PRINTLN("Waiting for modem to initialize...");
    delay(3000);

    // Test AT communication
    int attempts = 0;
    while (attempts < 10) {
        modemSerial->println("AT");
        delay(500);
        if (modemSerial->find("OK")) {
            DEBUG_PRINTLN("Modem responding to AT commands");
            break;
        }
        attempts++;
    }

    if (attempts >= 10) {
        DEBUG_PRINTLN("ERROR: Modem not responding");
        return false;
    }

    // Initialize modem
    DEBUG_PRINTLN("Initializing modem...");
    if (!modem.init()) {
        DEBUG_PRINTLN("WARNING: Modem init returned false, continuing anyway...");
    }

    // Get modem info
    String modemInfo = modem.getModemInfo();
    DEBUG_PRINTF("Modem: %s\n", modemInfo.c_str());

    // Configure SSL settings
    configureSSL();

    modemReady = true;
    DEBUG_PRINTLN("Modem initialized successfully");

    return true;
}

bool CellularModule::configureSSL() {
    DEBUG_PRINTLN("Configuring SSL settings...");

    // Enable SNI (Server Name Indication) - critical for modern HTTPS
    sendATCommand("AT+CSSLCFG=\"enableSNI\",0,1", 2000);

    // Ignore RTC time validation (in case modem time is not set)
    sendATCommand("AT+CSSLCFG=\"ignorertctime\",0,1", 2000);

    // Set SSL version to all (auto-negotiate)
    sendATCommand("AT+CSSLCFG=\"sslversion\",0,4", 2000);

    return true;
}

bool CellularModule::powerOn() {
    DEBUG_PRINTLN("Powering on modem...");

    // Power on sequence for A7670G
    digitalWrite(MODEM_PWRKEY, LOW);
    delay(100);
    digitalWrite(MODEM_PWRKEY, HIGH);
    delay(1000);
    digitalWrite(MODEM_PWRKEY, LOW);
    delay(100);

    // Wait for modem to start
    delay(5000);

    return true;
}

bool CellularModule::powerOff() {
    DEBUG_PRINTLN("Powering off modem...");

    modem.poweroff();
    delay(1000);

    digitalWrite(MODEM_PWRKEY, HIGH);
    delay(3000);
    digitalWrite(MODEM_PWRKEY, LOW);

    modemReady = false;
    networkConnected = false;

    return true;
}

bool CellularModule::restart() {
    DEBUG_PRINTLN("Restarting modem...");

    digitalWrite(MODEM_RST, HIGH);
    delay(200);
    digitalWrite(MODEM_RST, LOW);
    delay(5000);

    return begin();
}

bool CellularModule::connect() {
    if (!modemReady) {
        DEBUG_PRINTLN("Modem not ready, initializing...");
        if (!begin()) return false;
    }

    DEBUG_PRINTLN("Connecting to network...");

    // Wait for network registration
    if (!waitForNetwork()) {
        DEBUG_PRINTLN("ERROR: Network registration failed");
        return false;
    }

    DEBUG_PRINTLN("Network registered. Connecting GPRS...");

    // Connect to GPRS/LTE
    if (!modem.gprsConnect(APN_NAME, APN_USER, APN_PASS)) {
        DEBUG_PRINTLN("ERROR: GPRS connection failed");
        return false;
    }

    if (!waitForGPRS()) {
        DEBUG_PRINTLN("ERROR: GPRS not ready");
        return false;
    }

    // Update status
    networkConnected = true;
    signalStrength = modem.getSignalQuality();
    operatorName = modem.getOperator();

    DEBUG_PRINTF("Connected! Operator: %s, Signal: %d\n",
                 operatorName.c_str(), signalStrength);

    return true;
}

bool CellularModule::isConnected() {
    return networkConnected && modem.isGprsConnected();
}

void CellularModule::disconnect() {
    DEBUG_PRINTLN("Disconnecting from network...");
    modem.gprsDisconnect();
    networkConnected = false;
}

bool CellularModule::waitForNetwork(uint32_t timeoutMs) {
    DEBUG_PRINT("Waiting for network registration");

    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        if (modem.isNetworkConnected()) {
            DEBUG_PRINTLN(" OK");
            return true;
        }
        DEBUG_PRINT(".");
        delay(1000);
    }

    DEBUG_PRINTLN(" TIMEOUT");
    return false;
}

bool CellularModule::waitForGPRS(uint32_t timeoutMs) {
    DEBUG_PRINT("Waiting for GPRS");

    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        if (modem.isGprsConnected()) {
            DEBUG_PRINTLN(" OK");
            return true;
        }
        DEBUG_PRINT(".");
        delay(1000);
    }

    DEBUG_PRINTLN(" TIMEOUT");
    return false;
}

int CellularModule::getSignalStrength() {
    if (!modemReady) return 0;
    signalStrength = modem.getSignalQuality();
    return signalStrength;
}

String CellularModule::getOperator() {
    if (!modemReady) return "";
    return modem.getOperator();
}

bool CellularModule::isNetworkConnected() {
    if (!modemReady) return false;
    return modem.isNetworkConnected();
}

bool CellularModule::isGPRSConnected() {
    if (!modemReady) return false;
    return modem.isGprsConnected();
}

// ============================================================================
// AT COMMAND HELPERS
// ============================================================================

String CellularModule::sendATCommand(const char* cmd, uint32_t timeout) {
    // Clear buffer
    while (modemSerial->available()) {
        modemSerial->read();
    }

    DEBUG_PRINTF("AT> %s\n", cmd);
    modemSerial->println(cmd);
    return readResponse(timeout);
}

String CellularModule::readResponse(uint32_t timeout) {
    String response = "";
    uint32_t start = millis();

    while (millis() - start < timeout) {
        while (modemSerial->available()) {
            char c = modemSerial->read();
            response += c;
        }
        // Check if we got OK or ERROR
        if (response.indexOf("OK") >= 0 || response.indexOf("ERROR") >= 0) {
            break;
        }
        delay(10);
    }

    DEBUG_PRINTF("AT< %s\n", response.c_str());
    return response;
}

// ============================================================================
// HTTPS POST using lewisxhe TinyGSM fork
// ============================================================================

bool CellularModule::httpsPost(const char* url, const char* contentType, const char* body, String& response, int& statusCode) {
    statusCode = 0;
    response = "";

    DEBUG_PRINTF("HTTPS POST: %s\n", url);

    // Try using modem's built-in HTTPS if available
    // lewisxhe fork has: modem.https_begin(), modem.https_post()

    // First, terminate any existing HTTP session
    sendATCommand("AT+HTTPTERM", 1000);
    delay(100);

    // Initialize HTTP service
    String resp = sendATCommand("AT+HTTPINIT", 3000);
    if (resp.indexOf("OK") < 0) {
        // Try terminating and reinitializing
        sendATCommand("AT+HTTPTERM", 1000);
        delay(500);
        resp = sendATCommand("AT+HTTPINIT", 3000);
        if (resp.indexOf("OK") < 0) {
            DEBUG_PRINTLN("HTTPINIT failed");
            return false;
        }
    }

    // Set CID (PDP context)
    resp = sendATCommand("AT+HTTPPARA=\"CID\",1", 1000);

    // Set URL
    char urlCmd[300];
    snprintf(urlCmd, sizeof(urlCmd), "AT+HTTPPARA=\"URL\",\"%s\"", url);
    resp = sendATCommand(urlCmd, 2000);
    if (resp.indexOf("OK") < 0) {
        DEBUG_PRINTLN("Failed to set URL");
        sendATCommand("AT+HTTPTERM", 1000);
        return false;
    }

    // Enable SSL/HTTPS
    resp = sendATCommand("AT+HTTPSSL=1", 1000);
    if (resp.indexOf("OK") < 0) {
        DEBUG_PRINTLN("Failed to enable SSL");
        sendATCommand("AT+HTTPTERM", 1000);
        return false;
    }

    // Set content type
    char contentCmd[128];
    snprintf(contentCmd, sizeof(contentCmd), "AT+HTTPPARA=\"CONTENT\",\"%s\"", contentType);
    sendATCommand(contentCmd, 1000);

    // Set custom header for device ID
    char headerCmd[128];
    snprintf(headerCmd, sizeof(headerCmd), "AT+HTTPPARA=\"USERDATA\",\"X-Device-ID: %s\"", DEVICE_ID);
    sendATCommand(headerCmd, 1000);

    // Prepare to send data
    int bodyLen = strlen(body);
    char dataCmd[64];
    snprintf(dataCmd, sizeof(dataCmd), "AT+HTTPDATA=%d,10000", bodyLen);

    // Clear serial buffer
    while (modemSerial->available()) modemSerial->read();

    modemSerial->println(dataCmd);

    // Wait for DOWNLOAD prompt
    String downloadResp = "";
    uint32_t startWait = millis();
    while (millis() - startWait < 5000) {
        while (modemSerial->available()) {
            downloadResp += (char)modemSerial->read();
        }
        if (downloadResp.indexOf("DOWNLOAD") >= 0) {
            break;
        }
        delay(10);
    }

    if (downloadResp.indexOf("DOWNLOAD") < 0) {
        DEBUG_PRINTLN("HTTPDATA failed - no DOWNLOAD prompt");
        DEBUG_PRINTF("Got: %s\n", downloadResp.c_str());
        sendATCommand("AT+HTTPTERM", 1000);
        return false;
    }

    // Send the body data
    modemSerial->print(body);

    // Wait for OK after data sent
    resp = readResponse(5000);
    if (resp.indexOf("OK") < 0) {
        DEBUG_PRINTLN("Failed to send HTTP body data");
        sendATCommand("AT+HTTPTERM", 1000);
        return false;
    }

    // Execute POST (action=1)
    // Clear buffer first
    while (modemSerial->available()) modemSerial->read();

    modemSerial->println("AT+HTTPACTION=1");

    // Wait for +HTTPACTION response (can take a while for HTTPS)
    String actionResp = "";
    startWait = millis();
    while (millis() - startWait < 60000) {  // 60 second timeout for HTTPS
        while (modemSerial->available()) {
            actionResp += (char)modemSerial->read();
        }
        if (actionResp.indexOf("+HTTPACTION:") >= 0) {
            break;
        }
        delay(100);
    }

    DEBUG_PRINTF("HTTPACTION response: %s\n", actionResp.c_str());

    // Parse status code from +HTTPACTION: 1,<status>,<datalen>
    int actionIdx = actionResp.indexOf("+HTTPACTION:");
    if (actionIdx >= 0) {
        int commaIdx1 = actionResp.indexOf(',', actionIdx);
        int commaIdx2 = actionResp.indexOf(',', commaIdx1 + 1);
        if (commaIdx1 > 0 && commaIdx2 > commaIdx1) {
            statusCode = actionResp.substring(commaIdx1 + 1, commaIdx2).toInt();
        }
    }

    DEBUG_PRINTF("HTTP Status: %d\n", statusCode);

    // Read response body if status is OK
    if (statusCode >= 200 && statusCode < 300) {
        resp = sendATCommand("AT+HTTPREAD=0,1024", 5000);
        int readIdx = resp.indexOf("+HTTPREAD:");
        if (readIdx >= 0) {
            int dataStart = resp.indexOf('\n', readIdx) + 1;
            int dataEnd = resp.indexOf("\r\nOK", dataStart);
            if (dataEnd < 0) dataEnd = resp.indexOf("OK", dataStart);
            if (dataEnd > dataStart) {
                response = resp.substring(dataStart, dataEnd);
                response.trim();
            }
        }
    }

    // Terminate HTTP session
    sendATCommand("AT+HTTPTERM", 1000);

    DEBUG_PRINTF("POST %s -> %d\n", url, statusCode);

    return statusCode >= 200 && statusCode < 300;
}

// ============================================================================
// API FUNCTIONS
// ============================================================================

bool CellularModule::sendLocation(GPSData& gps, ActivityData& activity) {
    if (!isConnected()) {
        DEBUG_PRINTLN("Not connected, skipping location send");
        return false;
    }

    String json = buildJSON(gps, activity);
    String response;
    int statusCode;

    char url[128];
    snprintf(url, sizeof(url), "https://%s%s", SERVER_HOST, API_LOCATION);

    DEBUG_PRINTLN("Sending location data...");
    bool success = httpsPost(url, "application/json", json.c_str(), response, statusCode);

    if (success) {
        DEBUG_PRINTLN("Location sent successfully");
    } else {
        DEBUG_PRINTF("Failed to send location (status: %d)\n", statusCode);
    }

    return success;
}

bool CellularModule::sendWalkData(WalkSession& walk) {
    if (!isConnected()) {
        DEBUG_PRINTLN("Not connected, skipping walk data send");
        return false;
    }

    String json = buildWalkJSON(walk);
    String response;
    int statusCode;

    char url[128];
    snprintf(url, sizeof(url), "https://%s%s", SERVER_HOST, API_WALK);

    DEBUG_PRINTLN("Sending walk data...");
    bool success = httpsPost(url, "application/json", json.c_str(), response, statusCode);

    if (success) {
        DEBUG_PRINTLN("Walk data sent successfully");
    } else {
        DEBUG_PRINTF("Failed to send walk data (status: %d)\n", statusCode);
    }

    return success;
}

bool CellularModule::sendHeartbeat(DeviceStatus& status) {
    if (!isConnected()) {
        return false;
    }

    String json = buildStatusJSON(status);
    String response;
    int statusCode;

    char url[128];
    snprintf(url, sizeof(url), "https://%s%s", SERVER_HOST, API_HEARTBEAT);

    bool success = httpsPost(url, "application/json", json.c_str(), response, statusCode);

    DEBUG_PRINTF("Heartbeat -> %d\n", statusCode);

    return success;
}

bool CellularModule::sendAlert(const char* alertType, const char* message) {
    if (!isConnected()) {
        DEBUG_PRINTLN("Not connected, cannot send alert");
        return false;
    }

    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["alert_type"] = alertType;
    doc["message"] = message;
    doc["timestamp"] = millis();

    String json;
    serializeJson(doc, json);

    String response;
    int statusCode;

    char url[128];
    snprintf(url, sizeof(url), "https://%s%s", SERVER_HOST, API_ALERT);

    bool success = httpsPost(url, "application/json", json.c_str(), response, statusCode);

    if (success) {
        DEBUG_PRINTF("Alert sent: %s - %s\n", alertType, message);
    }

    return success;
}

bool CellularModule::sendSMS(const char* number, const char* message) {
    if (!modemReady) return false;

    DEBUG_PRINTF("Sending SMS to %s: %s\n", number, message);
    return modem.sendSMS(String(number), String(message));
}

String CellularModule::getIMEI() {
    if (!modemReady) return "";
    return modem.getIMEI();
}

String CellularModule::getICCID() {
    if (!modemReady) return "";
    return modem.getSimCCID();
}

// ============================================================================
// TIME SYNCHRONIZATION
// ============================================================================

static uint32_t bootUnixTime = 0;
static uint32_t bootMillis = 0;

bool CellularModule::syncTime() {
    if (!modemReady) return false;

    // Enable network time sync
    sendATCommand("AT+CTZU=1", 1000);

    // Get the current network time
    String response = sendATCommand("AT+CCLK?", 2000);

    // Parse response: +CCLK: "YY/MM/DD,HH:MM:SS+TZ"
    int start = response.indexOf("\"");
    int end = response.lastIndexOf("\"");
    if (start >= 0 && end > start) {
        String timeStr = response.substring(start + 1, end);
        int year = 2000 + timeStr.substring(0, 2).toInt();
        int month = timeStr.substring(3, 5).toInt();
        int day = timeStr.substring(6, 8).toInt();
        int hour = timeStr.substring(9, 11).toInt();
        int minute = timeStr.substring(12, 14).toInt();
        int second = timeStr.substring(15, 17).toInt();

        // Convert to Unix timestamp
        uint32_t unixTime = 0;
        for (int y = 1970; y < year; y++) {
            unixTime += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;
        }
        static const int daysInMonth[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
        unixTime += daysInMonth[month - 1];
        if (month > 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) {
            unixTime += 1;
        }
        unixTime += day - 1;
        unixTime = unixTime * 86400 + hour * 3600 + minute * 60 + second;

        bootUnixTime = unixTime;
        bootMillis = millis();

        DEBUG_PRINTF("Time synced: %04d-%02d-%02d %02d:%02d:%02d (Unix: %u)\n",
                     year, month, day, hour, minute, second, unixTime);
        return true;
    }

    DEBUG_PRINTLN("Failed to parse network time");
    return false;
}

uint32_t CellularModule::getNetworkTime() {
    if (bootUnixTime == 0) {
        syncTime();
    }

    if (bootUnixTime > 0) {
        return bootUnixTime + ((millis() - bootMillis) / 1000);
    }

    return millis() / 1000;
}

// ============================================================================
// JSON BUILDERS
// ============================================================================

String CellularModule::buildJSON(GPSData& gps, ActivityData& activity) {
    JsonDocument doc;

    doc["device_id"] = DEVICE_ID;
    doc["dog_name"] = DOG_NAME;
    doc["timestamp"] = getNetworkTime() * 1000UL;

    JsonObject location = doc["location"].to<JsonObject>();
    location["lat"] = gps.latitude;
    location["lon"] = gps.longitude;
    location["alt"] = gps.altitude;
    location["speed"] = gps.speed;
    location["course"] = gps.course;
    location["satellites"] = gps.satellites;
    location["valid"] = gps.valid;

    JsonObject activityObj = doc["activity"].to<JsonObject>();
    activityObj["steps"] = activity.stepCount;
    activityObj["active_minutes"] = activity.activeMinutes;
    activityObj["level"] = activity.level;
    activityObj["is_moving"] = activity.isMoving;
    activityObj["is_sleeping"] = activity.isSleeping;
    activityObj["is_scratching"] = activity.isScratching;

    String json;
    serializeJson(doc, json);
    return json;
}

String CellularModule::buildWalkJSON(WalkSession& walk) {
    JsonDocument doc;

    doc["device_id"] = DEVICE_ID;
    doc["dog_name"] = DOG_NAME;
    doc["walk_id"] = walk.walkId;

    uint32_t currentUnixMs = getNetworkTime() * 1000UL;
    uint32_t currentMillis = millis();

    uint32_t startUnixMs = currentUnixMs - (currentMillis - walk.startTime);
    uint32_t endUnixMs = currentUnixMs - (currentMillis - walk.endTime);

    doc["start_time"] = startUnixMs;
    doc["end_time"] = endUnixMs;
    doc["duration_seconds"] = walk.duration;

    doc["distance_meters"] = walk.distanceMeters;
    doc["avg_speed_kmh"] = walk.avgSpeed;
    doc["max_speed_kmh"] = walk.maxSpeed;
    doc["steps"] = walk.steps;

    doc["active_seconds"] = walk.activeSeconds;
    doc["stationary_seconds"] = walk.stationarySeconds;

    float activePercent = 0;
    if (walk.duration > 0) {
        activePercent = (float)walk.activeSeconds / walk.duration * 100;
    }
    doc["active_percent"] = activePercent;

    const char* gradeStr[] = {"A", "B", "C", "F"};
    doc["grade"] = gradeStr[walk.grade];

    doc["was_carried"] = walk.wasCarried;
    doc["was_in_vehicle"] = walk.wasInVehicle;
    doc["pause_count"] = walk.pauseCount;

    JsonObject start = doc["start_location"].to<JsonObject>();
    start["lat"] = walk.startLat;
    start["lon"] = walk.startLon;

    JsonObject end = doc["end_location"].to<JsonObject>();
    end["lat"] = walk.endLat;
    end["lon"] = walk.endLon;

    String json;
    serializeJson(doc, json);
    return json;
}

String CellularModule::buildStatusJSON(DeviceStatus& status) {
    JsonDocument doc;

    doc["device_id"] = DEVICE_ID;
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["timestamp"] = getNetworkTime() * 1000UL;

    doc["battery_voltage"] = status.batteryVoltage;
    doc["battery_percent"] = status.batteryPercent;
    doc["is_charging"] = status.isCharging;

    doc["gps_connected"] = status.gpsConnected;
    doc["modem_connected"] = status.modemConnected;
    doc["signal_strength"] = status.signalStrength;

    doc["uptime"] = status.uptime;
    doc["state"] = status.state;

    String json;
    serializeJson(doc, json);
    return json;
}
