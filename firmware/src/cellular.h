/**
 * PopsTracker - Cellular Module (A7670G)
 *
 * Handles LTE connectivity and data transmission to server
 */

#ifndef CELLULAR_H
#define CELLULAR_H

// SIM7600 driver is compatible with A7670G modem
// TINY_GSM_MODEM_SIM7600 defined in platformio.ini build_flags
#ifndef TINY_GSM_RX_BUFFER
#define TINY_GSM_RX_BUFFER 1024
#endif

#include <TinyGsmClient.h>
#include <SSLClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include "config.h"

class CellularModule {
public:
    CellularModule();

    bool begin();
    bool connect();
    bool isConnected();
    void disconnect();

    // Power management
    bool powerOn();
    bool powerOff();
    bool restart();

    // Network status
    int getSignalStrength();
    String getOperator();
    bool isNetworkConnected();
    bool isGPRSConnected();

    // HTTP API calls
    bool sendLocation(GPSData& gps, ActivityData& activity);
    bool sendWalkData(WalkSession& walk);
    bool sendHeartbeat(DeviceStatus& status);
    bool sendAlert(const char* alertType, const char* message);

    // Raw HTTP
    bool httpGet(const char* path, String& response);
    bool httpPost(const char* path, const char* body, String& response);

    // SMS (optional)
    bool sendSMS(const char* number, const char* message);

    // Utility
    String getIMEI();
    String getICCID();
    bool syncTime();
    uint32_t getNetworkTime();

private:
    HardwareSerial* modemSerial;
    TinyGsm modem;
    TinyGsmClient gsmClient;
    SSLClient* sslClient;       // SSL wrapper for HTTPS
    HttpClient* http;

    bool modemReady;
    bool networkConnected;
    int signalStrength;
    String operatorName;
    uint32_t lastConnectAttempt;

    // Helper methods
    bool waitForNetwork(uint32_t timeoutMs = 60000);
    bool waitForGPRS(uint32_t timeoutMs = 30000);
    String buildJSON(GPSData& gps, ActivityData& activity);
    String buildWalkJSON(WalkSession& walk);
    String buildStatusJSON(DeviceStatus& status);
};

extern CellularModule cellular;

#endif // CELLULAR_H
