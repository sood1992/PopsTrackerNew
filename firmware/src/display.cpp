/**
 * PopsTracker - OLED Display Module Implementation (U8g2)
 */

#include "display.h"

// Global instance
DisplayModule display;

DisplayModule::DisplayModule() : u8g2(nullptr), initialized(false), sleeping(false), lastUpdate(0) {
}

bool DisplayModule::begin() {
    // Create U8g2 display object for SH1106 128x64 I2C
    // Using hardware I2C with custom pins defined in Wire.begin()
    u8g2 = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);

    // Initialize with pre-configured Wire
    u8g2->setBusClock(I2C_FREQ);
    u8g2->begin();

    initialized = true;
    u8g2->clearBuffer();
    u8g2->sendBuffer();

    DEBUG_PRINTLN("Display initialized (SH1106)");
    return true;
}

void DisplayModule::clear() {
    if (!initialized) return;
    u8g2->clearBuffer();
    u8g2->sendBuffer();
}

void DisplayModule::update() {
    if (!initialized || sleeping) return;
    u8g2->sendBuffer();
    lastUpdate = millis();
}

void DisplayModule::showBoot() {
    if (!initialized) return;

    u8g2->clearBuffer();

    // Large title
    u8g2->setFont(u8g2_font_helvB14_tr);
    u8g2->drawStr(15, 25, "Popcorn");

    // Subtitle
    u8g2->setFont(u8g2_font_helvR08_tr);
    u8g2->drawStr(25, 42, "GPS Tracker");

    // Version
    char version[20];
    snprintf(version, sizeof(version), "v%s", FIRMWARE_VERSION);
    u8g2->drawStr(45, 58, version);

    u8g2->sendBuffer();
}

void DisplayModule::showStatus(DeviceStatus& status, GPSData& gps, ActivityData& activity) {
    if (!initialized) return;

    u8g2->clearBuffer();

    // Top bar icons
    drawBattery(0, 0, status.batteryPercent, status.isCharging);
    drawSignal(100, 0, status.signalStrength);
    drawGPS(50, 0, gps.valid, gps.satellites);

    // Separator line
    u8g2->drawHLine(0, 12, 128);

    // Activity info
    u8g2->setFont(u8g2_font_helvR08_tr);

    const char* levels[] = {"Resting", "Light", "Active", "V.Active"};
    char line[32];

    snprintf(line, sizeof(line), "Activity: %s", levels[activity.level]);
    u8g2->drawStr(0, 24, line);

    snprintf(line, sizeof(line), "Steps: %lu", (unsigned long)activity.stepCount);
    u8g2->drawStr(0, 35, line);

    // GPS coordinates
    if (gps.valid) {
        snprintf(line, sizeof(line), "Lat: %.5f", gps.latitude);
        u8g2->drawStr(0, 46, line);
        snprintf(line, sizeof(line), "Lon: %.5f", gps.longitude);
        u8g2->drawStr(0, 57, line);
    } else {
        u8g2->drawStr(0, 46, "GPS: Searching...");
    }

    u8g2->sendBuffer();
}

void DisplayModule::showWalk(WalkSession& walk, GPSData& gps) {
    if (!initialized) return;

    u8g2->clearBuffer();

    // Header
    u8g2->setFont(u8g2_font_helvB08_tr);
    u8g2->drawStr(15, 10, "WALK IN PROGRESS");
    u8g2->drawHLine(0, 12, 128);

    u8g2->setFont(u8g2_font_helvR08_tr);
    char line[32];

    // Duration
    uint32_t mins = walk.duration / 60;
    uint32_t secs = walk.duration % 60;
    snprintf(line, sizeof(line), "Time: %lu:%02lu", (unsigned long)mins, (unsigned long)secs);
    u8g2->drawStr(0, 24, line);

    // Distance
    if (walk.distanceMeters >= 1000) {
        snprintf(line, sizeof(line), "Dist: %.2f km", walk.distanceMeters / 1000.0);
    } else {
        snprintf(line, sizeof(line), "Dist: %.0f m", walk.distanceMeters);
    }
    u8g2->drawStr(0, 35, line);

    // Steps
    snprintf(line, sizeof(line), "Steps: %lu", (unsigned long)walk.steps);
    u8g2->drawStr(0, 46, line);

    // Grade - larger font
    u8g2->setFont(u8g2_font_helvB14_tr);
    const char* grades[] = {"A", "B", "C", "F"};
    snprintf(line, sizeof(line), "Grade: %s", grades[walk.grade]);
    u8g2->drawStr(0, 62, line);

    u8g2->sendBuffer();
}

void DisplayModule::showAlert(const char* title, const char* message) {
    if (!initialized) return;

    u8g2->clearBuffer();

    // Border
    u8g2->drawFrame(0, 0, 128, 64);
    u8g2->drawFrame(2, 2, 124, 60);

    // Title
    u8g2->setFont(u8g2_font_helvB08_tr);
    char titleLine[32];
    snprintf(titleLine, sizeof(titleLine), "! %s !", title);
    u8g2->drawStr(10, 18, titleLine);

    // Message
    u8g2->setFont(u8g2_font_helvR08_tr);
    u8g2->drawStr(8, 40, message);

    u8g2->sendBuffer();
}

void DisplayModule::showFindMe() {
    if (!initialized) return;

    u8g2->clearBuffer();

    u8g2->setFont(u8g2_font_helvB14_tr);
    u8g2->drawStr(15, 30, "FIND ME!");

    u8g2->setFont(u8g2_font_helvR08_tr);
    u8g2->drawStr(10, 50, "Looking for dog...");

    u8g2->sendBuffer();
}

void DisplayModule::showCharging(uint8_t percent) {
    if (!initialized) return;

    u8g2->clearBuffer();

    // Large battery outline
    u8g2->drawFrame(24, 15, 80, 35);
    u8g2->drawBox(104, 25, 6, 15);

    // Fill level
    int fillWidth = (percent * 74) / 100;
    if (fillWidth > 0) {
        u8g2->drawBox(27, 18, fillWidth, 29);
    }

    // Percentage
    u8g2->setFont(u8g2_font_helvB14_tr);
    char pct[10];
    snprintf(pct, sizeof(pct), "%d%%", percent);
    u8g2->drawStr(45, 62, pct);

    u8g2->sendBuffer();
}

void DisplayModule::showError(const char* error) {
    if (!initialized) return;

    u8g2->clearBuffer();

    u8g2->setFont(u8g2_font_helvB08_tr);
    u8g2->drawStr(0, 12, "ERROR:");

    u8g2->setFont(u8g2_font_helvR08_tr);
    u8g2->drawStr(0, 30, error);

    u8g2->sendBuffer();
}

void DisplayModule::drawBattery(int x, int y, uint8_t percent, bool charging) {
    // Battery outline (20x8)
    u8g2->drawFrame(x, y, 20, 8);
    u8g2->drawBox(x + 20, y + 2, 2, 4);  // Tip

    // Fill based on percentage
    int fillWidth = (percent * 16) / 100;
    if (fillWidth > 0) {
        u8g2->drawBox(x + 2, y + 2, fillWidth, 4);
    }

    // Charging indicator
    if (charging) {
        u8g2->setFont(u8g2_font_helvR08_tr);
        u8g2->drawStr(x + 24, y + 8, "+");
    }
}

void DisplayModule::drawSignal(int x, int y, int strength) {
    // Signal bars (4 bars)
    int bars = 0;
    if (strength > 20) bars = 4;
    else if (strength > 15) bars = 3;
    else if (strength > 10) bars = 2;
    else if (strength > 5) bars = 1;

    for (int i = 0; i < 4; i++) {
        int barHeight = 2 + (i * 2);
        int barY = y + 8 - barHeight;
        if (i < bars) {
            u8g2->drawBox(x + (i * 5), barY, 3, barHeight);
        } else {
            u8g2->drawFrame(x + (i * 5), barY, 3, barHeight);
        }
    }
}

void DisplayModule::drawGPS(int x, int y, bool valid, uint8_t sats) {
    u8g2->setFont(u8g2_font_helvR08_tr);
    char gpsStr[12];
    if (valid) {
        snprintf(gpsStr, sizeof(gpsStr), "GPS:%d", sats);
    } else {
        snprintf(gpsStr, sizeof(gpsStr), "GPS:--");
    }
    u8g2->drawStr(x, y + 8, gpsStr);
}

void DisplayModule::sleep() {
    if (!initialized) return;
    u8g2->setPowerSave(1);
    sleeping = true;
}

void DisplayModule::wake() {
    if (!initialized) return;
    u8g2->setPowerSave(0);
    sleeping = false;
}
