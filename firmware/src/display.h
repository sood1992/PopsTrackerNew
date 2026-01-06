/**
 * PopsTracker - OLED Display Module (SH1106 via U8g2)
 *
 * Shows status info on 128x64 OLED display via I2C
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Wire.h>
#include <U8g2lib.h>
#include "config.h"

class DisplayModule {
public:
    DisplayModule();

    bool begin();
    void clear();
    void update();

    // Display screens
    void showBoot();
    void showStatus(DeviceStatus& status, GPSData& gps, ActivityData& activity);
    void showWalk(WalkSession& walk, GPSData& gps);
    void showAlert(const char* title, const char* message);
    void showFindMe();
    void showCharging(uint8_t percent);
    void showError(const char* error);

    // Low-level drawing
    void drawBattery(int x, int y, uint8_t percent, bool charging);
    void drawSignal(int x, int y, int strength);
    void drawGPS(int x, int y, bool valid, uint8_t sats);

    // Power management
    void sleep();
    void wake();

private:
    U8G2_SH1106_128X64_NONAME_F_HW_I2C* u8g2;
    bool initialized;
    bool sleeping;
    uint32_t lastUpdate;
};

extern DisplayModule display;

#endif // DISPLAY_H
