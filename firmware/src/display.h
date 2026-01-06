/**
 * PopsTracker - OLED Display Module (SSD1306)
 *
 * Shows status info on 128x64 OLED display via I2C
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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
    void setBrightness(uint8_t level);

private:
    Adafruit_SSD1306* oled;
    bool initialized;
    bool sleeping;
    uint32_t lastUpdate;
};

extern DisplayModule display;

#endif // DISPLAY_H
