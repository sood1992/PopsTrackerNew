/**
 * PopsTracker - OLED Display Module Implementation
 */

#include "display.h"

// Global instance
DisplayModule display;

DisplayModule::DisplayModule() : oled(nullptr), initialized(false), sleeping(false), lastUpdate(0) {
}

bool DisplayModule::begin() {
    // Create display object using the already-initialized Wire
    oled = new Adafruit_SSD1306(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

    // Initialize display
    if (!oled->begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        DEBUG_PRINTLN("SSD1306 initialization failed!");
        return false;
    }

    initialized = true;

    // Clear the display buffer completely
    oled->clearDisplay();
    oled->display();
    delay(100);

    // Set normal display mode (not inverted)
    oled->invertDisplay(false);

    // Configure text settings
    oled->setTextColor(SSD1306_WHITE);
    oled->setTextSize(1);
    oled->cp437(true);  // Use full 256 char font

    DEBUG_PRINTLN("Display initialized");
    return true;
}

void DisplayModule::clear() {
    if (!initialized) return;
    oled->clearDisplay();
    oled->display();
}

void DisplayModule::update() {
    if (!initialized || sleeping) return;
    oled->display();
    lastUpdate = millis();
}

void DisplayModule::showBoot() {
    if (!initialized) return;

    oled->clearDisplay();
    oled->fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT, SSD1306_BLACK);  // Force clear

    oled->setTextColor(SSD1306_WHITE, SSD1306_BLACK);  // White text, black background
    oled->setTextSize(2);
    oled->setCursor(10, 10);
    oled->print("Popcorn");

    oled->setTextSize(1);
    oled->setCursor(20, 35);
    oled->print("GPS Tracker");
    oled->setCursor(25, 50);
    oled->print("v");
    oled->print(FIRMWARE_VERSION);

    oled->display();
}

void DisplayModule::showStatus(DeviceStatus& status, GPSData& gps, ActivityData& activity) {
    if (!initialized) return;

    oled->clearDisplay();
    oled->setTextColor(SSD1306_WHITE, SSD1306_BLACK);

    // Top bar: battery and signal
    drawBattery(0, 0, status.batteryPercent, status.isCharging);
    drawSignal(100, 0, status.signalStrength);

    // GPS status
    drawGPS(50, 0, gps.valid, gps.satellites);

    // Line separator
    oled->drawLine(0, 12, 127, 12, SSD1306_WHITE);

    // Main content
    oled->setTextSize(1);

    // Activity
    oled->setCursor(0, 16);
    const char* levels[] = {"Resting", "Light", "Active", "V.Active"};
    oled->print("Activity: ");
    oled->println(levels[activity.level]);

    // Steps
    oled->setCursor(0, 26);
    oled->print("Steps: ");
    oled->println(activity.stepCount);

    // GPS coordinates (if valid)
    if (gps.valid) {
        oled->setCursor(0, 38);
        oled->print("Lat:");
        oled->println(gps.latitude, 5);
        oled->setCursor(0, 48);
        oled->print("Lon:");
        oled->println(gps.longitude, 5);
    } else {
        oled->setCursor(0, 40);
        oled->print("GPS: Searching...");
    }

    // Connection status at bottom
    oled->setCursor(0, 56);
    oled->print(status.modemConnected ? "LTE:OK " : "LTE:-- ");
    oled->print(status.serverConnected ? "SRV:OK" : "SRV:--");

    oled->display();
}

void DisplayModule::showWalk(WalkSession& walk, GPSData& gps) {
    if (!initialized) return;

    oled->clearDisplay();

    // Header
    oled->setTextSize(1);
    oled->setCursor(0, 0);
    oled->print("WALK IN PROGRESS");
    oled->drawLine(0, 10, 127, 10, SSD1306_WHITE);

    // Duration
    uint32_t mins = walk.duration / 60;
    uint32_t secs = walk.duration % 60;
    oled->setCursor(0, 14);
    oled->print("Time: ");
    oled->print(mins);
    oled->print(":");
    if (secs < 10) oled->print("0");
    oled->println(secs);

    // Distance
    oled->setCursor(0, 24);
    oled->print("Dist: ");
    if (walk.distanceMeters >= 1000) {
        oled->print(walk.distanceMeters / 1000.0, 2);
        oled->println(" km");
    } else {
        oled->print((int)walk.distanceMeters);
        oled->println(" m");
    }

    // Steps
    oled->setCursor(0, 34);
    oled->print("Steps: ");
    oled->println(walk.steps);

    // Current grade
    oled->setCursor(0, 46);
    oled->setTextSize(2);
    oled->print("Grade:");
    const char* grades[] = {"A", "B", "C", "F"};
    oled->println(grades[walk.grade]);

    oled->display();
}

void DisplayModule::showAlert(const char* title, const char* message) {
    if (!initialized) return;

    oled->clearDisplay();

    // Alert border
    oled->drawRect(0, 0, 128, 64, SSD1306_WHITE);
    oled->drawRect(2, 2, 124, 60, SSD1306_WHITE);

    // Title
    oled->setTextSize(1);
    oled->setCursor(10, 8);
    oled->print("! ");
    oled->print(title);
    oled->print(" !");

    // Message (word wrap would be nice, but keep it simple)
    oled->setCursor(8, 28);
    oled->print(message);

    oled->display();
}

void DisplayModule::showFindMe() {
    if (!initialized) return;

    oled->clearDisplay();
    oled->setTextSize(2);
    oled->setCursor(20, 10);
    oled->print("FIND ME!");
    oled->setTextSize(1);
    oled->setCursor(15, 40);
    oled->print("Looking for dog...");
    oled->display();
}

void DisplayModule::showCharging(uint8_t percent) {
    if (!initialized) return;

    oled->clearDisplay();

    // Large battery icon
    oled->drawRect(24, 15, 80, 35, SSD1306_WHITE);
    oled->fillRect(104, 25, 6, 15, SSD1306_WHITE);

    // Fill level
    int fillWidth = (percent * 74) / 100;
    oled->fillRect(27, 18, fillWidth, 29, SSD1306_WHITE);

    // Percentage text
    oled->setTextSize(2);
    oled->setCursor(45, 55);
    oled->print(percent);
    oled->print("%");

    oled->display();
}

void DisplayModule::showError(const char* error) {
    if (!initialized) return;

    oled->clearDisplay();
    oled->setTextSize(1);
    oled->setCursor(0, 0);
    oled->println("ERROR:");
    oled->println();
    oled->println(error);
    oled->display();
}

void DisplayModule::drawBattery(int x, int y, uint8_t percent, bool charging) {
    // Battery outline (20x8)
    oled->drawRect(x, y, 20, 8, SSD1306_WHITE);
    oled->fillRect(x + 20, y + 2, 2, 4, SSD1306_WHITE);  // Tip

    // Fill based on percentage
    int fillWidth = (percent * 16) / 100;
    if (fillWidth > 0) {
        oled->fillRect(x + 2, y + 2, fillWidth, 4, SSD1306_WHITE);
    }

    // Charging indicator
    if (charging) {
        oled->setCursor(x + 24, y);
        oled->print("+");
    }
}

void DisplayModule::drawSignal(int x, int y, int strength) {
    // Signal bars (4 bars)
    // strength: 0-31 from modem
    int bars = 0;
    if (strength > 20) bars = 4;
    else if (strength > 15) bars = 3;
    else if (strength > 10) bars = 2;
    else if (strength > 5) bars = 1;

    for (int i = 0; i < 4; i++) {
        int barHeight = 2 + (i * 2);
        int barY = y + 8 - barHeight;
        if (i < bars) {
            oled->fillRect(x + (i * 5), barY, 3, barHeight, SSD1306_WHITE);
        } else {
            oled->drawRect(x + (i * 5), barY, 3, barHeight, SSD1306_WHITE);
        }
    }
}

void DisplayModule::drawGPS(int x, int y, bool valid, uint8_t sats) {
    oled->setCursor(x, y);
    if (valid) {
        oled->print("GPS:");
        oled->print(sats);
    } else {
        oled->print("GPS:--");
    }
}

void DisplayModule::sleep() {
    if (!initialized) return;
    oled->ssd1306_command(SSD1306_DISPLAYOFF);
    sleeping = true;
}

void DisplayModule::wake() {
    if (!initialized) return;
    oled->ssd1306_command(SSD1306_DISPLAYON);
    sleeping = false;
}

void DisplayModule::setBrightness(uint8_t level) {
    if (!initialized) return;
    oled->ssd1306_command(SSD1306_SETCONTRAST);
    oled->ssd1306_command(level);
}
