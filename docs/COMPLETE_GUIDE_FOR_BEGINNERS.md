# PopsTracker Complete Beginner's Guide

## For Complete Beginners Who Know NOTHING About Hardware/Coding

This guide assumes you've never done any electronics or coding before. Follow each step exactly.

---

## TABLE OF CONTENTS

1. [What You're Building](#1-what-youre-building)
2. [Shopping List - What to Buy](#2-shopping-list---what-to-buy)
3. [Tools You'll Need](#3-tools-youll-need)
4. [Setting Up Your Computer](#4-setting-up-your-computer)
5. [Hardware Assembly - Step by Step](#5-hardware-assembly---step-by-step)
6. [Uploading the Code to the Device](#6-uploading-the-code-to-the-device)
7. [Setting Up the Server (Backend)](#7-setting-up-the-server-backend)
8. [Setting Up the Dashboard (Frontend)](#8-setting-up-the-dashboard-frontend)
9. [Testing Everything](#9-testing-everything)
10. [Putting It All Together](#10-putting-it-all-together)
11. [Troubleshooting Common Problems](#11-troubleshooting-common-problems)

---

## 1. WHAT YOU'RE BUILDING

You're building a **GPS tracker collar** for your dog Popcorn that will:

- **Track location** - See where Popcorn is on a map in real-time
- **Count steps** - Like a Fitbit for dogs
- **Monitor activity** - Know if Popcorn is resting, walking, or very active
- **Grade walks** - Rate your dog walker's performance (A, B, C, or F)
- **Detect cheating** - Know if the walker carried Popcorn or put them in a car
- **Alert you** - Beep and vibrate to find Popcorn

**The system has 3 parts:**
```
┌─────────────────┐        ┌─────────────────┐        ┌─────────────────┐
│   DOG COLLAR    │───────►│     SERVER      │───────►│   YOUR PHONE/   │
│  (The Device)   │ 4G LTE │  (The Backend)  │Internet│   COMPUTER      │
│                 │        │                 │        │  (Dashboard)    │
└─────────────────┘        └─────────────────┘        └─────────────────┘
```

---

## 2. SHOPPING LIST - WHAT TO BUY

### Main Board (The Brain) - CRITICAL
| Item | Where to Buy | Approx Price | Notes |
|------|-------------|--------------|-------|
| **LILYGO T-A7670G R2** | AliExpress, Amazon | ₹3,500-4,500 | This is the main board. Make sure it's the **R2 version** with **A7670G** (not A7670E) |

**What this board includes:**
- ESP32 microcontroller (the brain)
- A7670G 4G LTE modem (sends data over cellular)
- L76K GPS module (tracks location)
- 18650 battery holder
- USB-C port for programming

### Sensors and Components
| Item | Where to Buy | Approx Price | Notes |
|------|-------------|--------------|-------|
| ADXL345 Accelerometer Module | Robu.in, Amazon | ₹150-300 | Detects movement. Get the **purple module** with pins |
| Piezo Buzzer | Robu.in, Amazon | ₹30-50 | Small 12mm active buzzer |
| Coin Vibration Motor | Robu.in, Amazon | ₹50-100 | 10mm flat coin motor |
| 2x 2N2222A Transistors | Robu.in, Amazon | ₹10-20 | NPN transistor, TO-92 package |
| 2x 1K Ohm Resistors | Robu.in, Amazon | ₹5 | Brown-Black-Red bands |
| 1x 1N4001 Diode | Robu.in, Amazon | ₹5 | Protects motor circuit |
| Jumper Wires | Robu.in, Amazon | ₹100 | Female-to-female, various colors |

### SIM Card and Antennas
| Item | Where to Buy | Approx Price | Notes |
|------|-------------|--------------|-------|
| **Nano SIM Card** | Airtel store | ₹100-200 | Get **Airtel IoT SIM** or regular data SIM with at least 1GB/month |
| LTE Antenna | (Usually included) | - | SMA connector, for 4G signal |
| GPS Antenna | (Usually included) | - | Small ceramic or patch antenna |

### Battery
| Item | Where to Buy | Approx Price | Notes |
|------|-------------|--------------|-------|
| 18650 Battery (3.7V) | Amazon, local shops | ₹300-500 | Get a good brand like Samsung or Panasonic. **2500mAh minimum** |

### Total Cost: Approximately ₹4,500 - ₹6,000

---

## 3. TOOLS YOU'LL NEED

### Essential Tools
| Tool | Why You Need It | Where to Buy |
|------|-----------------|--------------|
| USB-C Cable | Connect board to computer | ₹200 (Amazon) |
| Soldering Iron | Join wires (30W is fine) | ₹300-500 (local electronics shop) |
| Solder Wire | Metal that melts to join parts | ₹50 |
| Wire Stripper | Remove plastic from wires | ₹100 |
| Multimeter | Test connections | ₹300-500 |
| Small Screwdriver Set | Open things | ₹100 |
| Hot Glue Gun | Secure components | ₹150 |

### Nice to Have
- Helping hands (holds things while soldering)
- Heat shrink tubing
- Electrical tape

---

## 4. SETTING UP YOUR COMPUTER

### Step 4.1: Install Visual Studio Code (VS Code)

VS Code is a free program where you'll write and upload code.

1. **Go to:** https://code.visualstudio.com/download
2. **Download** the version for your computer (Windows/Mac/Linux)
3. **Run the installer** - just click Next, Next, Install
4. **Open VS Code** when done

### Step 4.2: Install PlatformIO Extension

PlatformIO is a plugin that lets you upload code to the dog collar.

1. **Open VS Code**
2. **Click the Extensions icon** (looks like 4 squares on the left sidebar)
   ```
   Or press: Ctrl+Shift+X (Windows) or Cmd+Shift+X (Mac)
   ```
3. **Search for:** `PlatformIO`
4. **Click "Install"** on "PlatformIO IDE"
5. **Wait** - it takes 5-10 minutes to install everything
6. **Restart VS Code** when prompted

### Step 4.3: Install Git (For Downloading Code)

1. **Go to:** https://git-scm.com/downloads
2. **Download and install** for your operating system
3. **Accept all default options** during installation

### Step 4.4: Install Node.js (For the Server)

1. **Go to:** https://nodejs.org/
2. **Download the LTS version** (the one that says "Recommended")
3. **Run the installer** - click Next through everything
4. **Verify installation:**
   - Open Terminal (Mac) or Command Prompt (Windows)
   - Type: `node --version`
   - You should see something like: `v20.10.0`

### Step 4.5: Install Docker Desktop (For the Database)

1. **Go to:** https://www.docker.com/products/docker-desktop/
2. **Download and install** Docker Desktop
3. **Start Docker Desktop** and let it run in background

---

## 5. HARDWARE ASSEMBLY - STEP BY STEP

### CRITICAL WARNING - READ THIS FIRST

```
⚠️  IMPORTANT PIN CONFIGURATION  ⚠️

This board uses the ONBOARD L76K GPS which is connected to GPIO 21 and 22.
Because of this, the I2C bus (for accelerometer) MUST use DIFFERENT pins:

   ✓ I2C SDA = GPIO 33 (NOT GPIO 21!)
   ✓ I2C SCL = GPIO 23 (NOT GPIO 22!)

If you wire I2C to GPIO 21/22, it will conflict with GPS and NOT WORK!
```

### Step 5.1: Prepare the Main Board

1. **Unbox your LILYGO T-A7670G R2**
2. **Locate these ports:**
   ```
   ┌─────────────────────────────────────────────┐
   │  LILYGO T-A7670G R2 - Top View              │
   │                                             │
   │  [LTE ANT]  [GPS ANT]                       │
   │       ○         ○       ← Antenna ports     │
   │                                             │
   │  ┌─────────────────────────────────────┐    │
   │  │                                     │    │
   │  │     ESP32 + A7670G Module           │    │
   │  │                                     │    │
   │  └─────────────────────────────────────┘    │
   │                                             │
   │  [SIM CARD SLOT]    ← Push-push type        │
   │                                             │
   │  [USB-C]  [USB-C]                           │
   │   PROG     UART     ← Use PROG for upload   │
   │                                             │
   │  [─────── 18650 BATTERY ───────]            │
   │                                             │
   └─────────────────────────────────────────────┘
   ```

3. **Insert the SIM card:**
   - Use a Nano SIM
   - Push gently until it clicks
   - Chip side facing down

4. **Connect the antennas:**
   - Screw on the LTE antenna to the LTE port
   - Screw on the GPS antenna to the GPS port

5. **Insert the 18650 battery:**
   - Positive (+) end goes toward the + marking
   - Negative (-) end goes toward the spring

### Step 5.2: Understanding the Pin Numbers

The ESP32 has numbered pins called GPIOs. Here's where to find them on the board:

```
LILYGO T-A7670G R2 Pin Header (Looking at the board with USB ports at bottom)

Left Side:                          Right Side:
┌─────────┐                        ┌─────────┐
│  3.3V   │◄─ Power output         │   5V    │
│   GND   │◄─ Ground              │   GND   │
│  GPIO36 │                       │  GPIO23 │◄─ I2C SCL (use this!)
│  GPIO39 │                       │  GPIO22 │◄─ GPS RX (internal, don't use)
│  GPIO34 │                       │  GPIO1  │
│  GPIO35 │◄─ Battery ADC         │  GPIO3  │
│  GPIO32 │◄─ ADXL345 INT1        │  GPIO21 │◄─ GPS TX (internal, don't use)
│  GPIO33 │◄─ I2C SDA (use this!) │  GPIO19 │◄─ Buzzer
│  GPIO25 │◄─ Vibration Motor     │  GPIO18 │
│  GPIO26 │◄─ Modem TX (internal) │   GND   │
│  GPIO27 │◄─ Modem RX (internal) │  GPIO5  │
│         │                       │  GPIO17 │
│   GND   │                       │  GPIO16 │
└─────────┘                        └─────────┘
```

### Step 5.3: Wire the ADXL345 Accelerometer

The accelerometer detects motion, steps, and activity.

**What you need:**
- ADXL345 module (purple board with 8 pins)
- 5 jumper wires (female-to-female)

**Connections:**
```
ADXL345 Module          LILYGO Board
──────────────          ────────────
    VCC    ────────────►  3.3V
    GND    ────────────►  GND
    SDA    ────────────►  GPIO 33  (⚠️ NOT GPIO 21!)
    SCL    ────────────►  GPIO 23  (⚠️ NOT GPIO 22!)
    CS     ────────────►  3.3V     (enables I2C mode)
    SDO    ────────────►  GND      (sets address to 0x53)
    INT1   ────────────►  GPIO 32  (interrupt for wake)
    INT2   ────────────►  (leave unconnected)
```

**Step-by-step:**
1. Take a jumper wire and connect ADXL345's VCC pin to the board's 3.3V pin
2. Connect ADXL345's GND to board's GND
3. Connect ADXL345's SDA to **GPIO 33** (NOT 21!)
4. Connect ADXL345's SCL to **GPIO 23** (NOT 22!)
5. Connect ADXL345's CS to 3.3V
6. Connect ADXL345's SDO to GND
7. Connect ADXL345's INT1 to GPIO 32

**Verify:** Use your multimeter in continuity mode to check each connection.

### Step 5.4: Wire the Buzzer (with Transistor)

The buzzer makes sounds to alert you.

**What you need:**
- Piezo buzzer
- 1x 2N2222A transistor
- 1x 1K resistor
- 4 jumper wires

**Understanding the transistor:**
```
        2N2222A Transistor (flat side facing you)

              ┌───┐
              │   │
              └┬─┬┘
               │ │ │
               E B C
               │ │ │
          Emitter Base Collector
          (to GND) (to GPIO) (to Buzzer-)
```

**Wiring diagram:**
```
         3.3V
          │
          ▼
    ┌───────────┐
    │  BUZZER   │
    │   + ─     │
    └──┬────┬───┘
       │    │
       │    │ (Buzzer negative)
       │    ▼
       │    C (Collector)
       │    │
       │  ┌─┴─┐
       │  │2N │
       │  │222│
       │  │2A │
       │  └─┬─┘
       │    │
       │    B (Base) ◄───[1K Resistor]───► GPIO 19
       │    │
       │    E (Emitter)
       │    │
       │    ▼
      GND  GND
```

**Step-by-step:**
1. Connect buzzer positive (+) to 3.3V
2. Connect buzzer negative (-) to the Collector (C) of the 2N2222A
3. Connect the Emitter (E) to GND
4. Connect one end of the 1K resistor to the Base (B)
5. Connect the other end of the 1K resistor to GPIO 19

### Step 5.5: Wire the Vibration Motor (with Transistor and Diode)

The motor vibrates the collar. It needs a protection diode!

**What you need:**
- Coin vibration motor
- 1x 2N2222A transistor
- 1x 1K resistor
- 1x 1N4001 diode
- 4 jumper wires

**⚠️ IMPORTANT: The diode protects the circuit from voltage spikes when the motor turns off. Without it, you can damage the board!**

**Wiring diagram:**
```
         3.3V
          │
          ▼
    ┌─────┴─────┐
    │           │
    │   ┌───┐   │
    │   │ D │   │◄── 1N4001 Diode (stripe toward +)
    │   └─┬─┘   │
    │     │     │
    │  ┌──┴──┐  │
    │  │MOTOR│  │
    │  │ ◐◐◐ │  │
    │  └──┬──┘  │
    │     │     │
    └─────┼─────┘
          │
          ▼
          C (Collector)
          │
        ┌─┴─┐
        │2N │
        │222│
        │2A │
        └─┬─┘
          │
          B (Base) ◄───[1K Resistor]───► GPIO 25
          │
          E (Emitter)
          │
          ▼
         GND
```

**Step-by-step:**
1. Connect motor positive (+) to 3.3V
2. Connect motor negative (-) to the Collector (C) of the 2N2222A
3. Connect the Emitter (E) to GND
4. Connect one end of the 1K resistor to the Base (B)
5. Connect the other end of the 1K resistor to GPIO 25
6. **IMPORTANT:** Connect the 1N4001 diode ACROSS the motor:
   - Diode stripe (silver band) goes to the + side (3.3V)
   - Other end goes to the - side (Collector)

### Step 5.6: Final Wiring Check

Use this checklist to verify every connection:

```
COMPLETE WIRING CHECKLIST
═════════════════════════

ADXL345 Accelerometer:
[ ] VCC → 3.3V
[ ] GND → GND
[ ] SDA → GPIO 33
[ ] SCL → GPIO 23
[ ] CS  → 3.3V
[ ] SDO → GND
[ ] INT1 → GPIO 32

Buzzer Circuit:
[ ] Buzzer (+) → 3.3V
[ ] Buzzer (-) → Transistor Collector
[ ] Transistor Emitter → GND
[ ] Transistor Base → 1K Resistor → GPIO 19

Motor Circuit:
[ ] Motor (+) → 3.3V
[ ] Motor (-) → Transistor Collector
[ ] Transistor Emitter → GND
[ ] Transistor Base → 1K Resistor → GPIO 25
[ ] 1N4001 Diode across motor (stripe to +)

Power:
[ ] SIM card inserted
[ ] Battery inserted (correct polarity)
[ ] LTE antenna connected
[ ] GPS antenna connected
```

---

## 6. UPLOADING THE CODE TO THE DEVICE

### Step 6.1: Download the Code

1. **Open Terminal** (Mac) or **Command Prompt** (Windows)
   - Windows: Press `Win+R`, type `cmd`, press Enter
   - Mac: Open Spotlight (Cmd+Space), type `Terminal`, press Enter

2. **Navigate to a folder** where you want to keep the project:
   ```bash
   cd Documents
   ```

3. **Download the code:**
   ```bash
   git clone https://github.com/your-repo/PopsTrackerNew.git
   ```

4. **Open in VS Code:**
   ```bash
   cd PopsTrackerNew
   code .
   ```

### Step 6.2: Configure Your Settings

You need to tell the device your server address and home location.

1. **In VS Code**, open the file: `firmware/src/config.h`

2. **Find and change these lines:**

   ```cpp
   // Line 26-27: Your server address (we'll set up later)
   #define SERVER_HOST         "your-server.com"    // Change this!
   #define SERVER_PORT         443

   // Line 75-77: Your home location (get from Google Maps)
   #define HOME_LATITUDE       28.6139     // Change to your latitude
   #define HOME_LONGITUDE      77.2090     // Change to your longitude
   #define HOME_RADIUS_METERS  50          // 50 meters is good
   ```

   **How to get your home coordinates:**
   1. Open Google Maps on your phone
   2. Long-press on your home
   3. The coordinates appear at the top
   4. Example: 28.6139, 77.2090 means:
      - Latitude = 28.6139
      - Longitude = 77.2090

3. **Save the file:** Press `Ctrl+S` (Windows) or `Cmd+S` (Mac)

### Step 6.3: Connect and Upload

1. **Connect the board to your computer:**
   - Use the USB-C cable
   - Plug into the **PROG** port (not UART)
   - The board should power on

2. **Open PlatformIO in VS Code:**
   - Click the alien/ant icon on the left sidebar
   - Or press `Ctrl+Shift+P` and type "PlatformIO Home"

3. **Build the code:**
   - In the PlatformIO sidebar, click **"Build"** (checkmark icon)
   - Wait for "SUCCESS" message
   - If you see errors, check the Troubleshooting section

4. **Upload to the board:**
   - Click **"Upload"** (arrow icon)
   - Wait for "SUCCESS" message
   - The board will restart automatically

### Step 6.4: Test the Upload

1. **Open Serial Monitor:**
   - Click **"Monitor"** in PlatformIO (plug icon)
   - Set baud rate to **115200**

2. **You should see:**
   ```
   ╔═══════════════════════════════════════════╗
   ║     PopsTracker - Dog Collar Tracker      ║
   ║         Made with ❤ for Popcorn           ║
   ╠═══════════════════════════════════════════╣
   ║  Firmware: v1.0.0                         ║
   ║  Device: POPCORN_001                      ║
   ╚═══════════════════════════════════════════╝

   Initializing hardware...
   I2C initialized on SDA=33, SCL=23
   ADXL345: OK
   GPS: OK (waiting for fix...)
   Modem initialized
   Connecting to cellular network...
   ```

3. **Test commands (type and press Enter):**
   ```
   help      - Show all commands
   status    - Show device status
   beep      - Test buzzer (should beep!)
   vibrate   - Test motor (should vibrate!)
   finddog   - Play loud SOS pattern
   ```

---

## 7. SETTING UP THE SERVER (BACKEND)

The server receives data from the collar and stores it. We'll use Railway (free tier).

### Step 7.1: Create a GitHub Account

1. Go to https://github.com/signup
2. Create an account (it's free)
3. Verify your email

### Step 7.2: Fork the Repository

1. Go to the PopsTracker GitHub page
2. Click **"Fork"** (top right)
3. This creates your own copy

### Step 7.3: Deploy to Railway

1. **Go to:** https://railway.app/
2. **Click** "Start a New Project"
3. **Choose** "Deploy from GitHub repo"
4. **Connect** your GitHub account
5. **Select** your PopsTrackerNew repository
6. **Configure** root directory: `/backend`
7. **Wait** for deployment (2-3 minutes)

### Step 7.4: Add MongoDB Database

1. In Railway dashboard, click **"+ New"**
2. Select **"Database"** → **"MongoDB"**
3. Railway automatically connects it

### Step 7.5: Get Your Server URL

1. Go to your Railway project
2. Click on your backend service
3. Go to **"Settings"** → **"Domains"**
4. Click **"Generate Domain"**
5. **Copy the URL** (looks like: `popstracker-production.up.railway.app`)

### Step 7.6: Update Firmware with Server URL

1. Go back to VS Code
2. Open `firmware/src/config.h`
3. Update line 26:
   ```cpp
   #define SERVER_HOST         "popstracker-production.up.railway.app"
   ```
4. Save and **re-upload** to the board

---

## 8. SETTING UP THE DASHBOARD (FRONTEND)

### Step 8.1: Deploy to Vercel (Free)

1. **Go to:** https://vercel.com/signup
2. **Sign up** with your GitHub account
3. **Click** "New Project"
4. **Import** your PopsTrackerNew repository
5. **Configure:**
   - Root Directory: `frontend`
   - Framework Preset: Other
6. **Add Environment Variable:**
   - Name: `VITE_API_URL`
   - Value: `https://your-railway-url.up.railway.app`
7. **Click** "Deploy"

### Step 8.2: Access Your Dashboard

1. Vercel gives you a URL like: `popstracker.vercel.app`
2. Open it in your browser
3. You should see the dashboard with a map!

---

## 9. TESTING EVERYTHING

### Step 9.1: Hardware Test

1. **Open Serial Monitor** in VS Code
2. **Type these commands:**
   ```
   status     → Should show battery, GPS, signal info
   beep       → Buzzer should beep 3 times
   vibrate    → Motor should vibrate
   finddog    → Loud SOS pattern (buzzer + motor)
   accel      → Should show X, Y, Z values changing when you move it
   ```

### Step 9.2: GPS Test

1. **Take the device outside** (GPS needs sky view)
2. **Wait 2-3 minutes** for GPS fix
3. **Type:** `gps`
4. **You should see:**
   ```
   GPS: 28.613900, 77.209000 | Alt: 215.3 | Speed: 0.5 | Sats: 8 | Valid: Yes
   ```

### Step 9.3: Server Connection Test

1. **Type:** `status`
2. **Look for:**
   ```
   Modem: Connected
   Signal: 15+
   Server: Connected
   ```

3. **Check your Railway logs:**
   - Go to Railway dashboard
   - Click on your service
   - Check "Logs" tab
   - You should see incoming data

### Step 9.4: Dashboard Test

1. **Open your dashboard URL**
2. **Check:**
   - [ ] Map shows your location
   - [ ] Status shows "Online"
   - [ ] Battery percentage updates
   - [ ] Activity level shows

### Step 9.5: Walk Test

1. **Go outside** with the device
2. **Walk away from home** (at least 50 meters)
3. **Walk for 2-3 minutes**
4. **Return home**
5. **Check dashboard:**
   - A new walk should appear
   - It should have a grade (A, B, C, or F)
   - Distance and duration should be recorded

---

## 10. PUTTING IT ALL TOGETHER

### Step 10.1: Enclosure Ideas

You need a waterproof case that attaches to the collar:

**Option 1: 3D Printed Case**
- Design files available in `/docs/enclosure/`
- Print with PETG or ABS (waterproof)
- Add silicone gasket

**Option 2: Small Waterproof Box**
- Get a small IP65 junction box from hardware store
- Cut holes for USB charging
- Seal with silicone

**Dimensions needed:**
- Board: 80mm x 50mm x 20mm
- Battery: 65mm x 18mm diameter
- Allow space for wires

### Step 10.2: Attaching to Collar

1. **Use a wide collar** (2-3 inches wide works best)
2. **Attach with zip ties** through mounting holes
3. **Position GPS antenna facing UP**
4. **Don't cover the GPS antenna with metal**

### Step 10.3: Charging

- Connect USB-C cable to PROG port
- Red LED = Charging
- Green LED = Fully charged
- Full charge takes 3-4 hours
- Battery lasts 2-3 days typical use

---

## 11. TROUBLESHOOTING COMMON PROBLEMS

### Problem: "Upload failed"

**Solution:**
1. Remove SD card if inserted
2. Try the other USB-C port (UART instead of PROG)
3. Press the RESET button while uploading
4. Check USB cable (some cables are charge-only)

### Problem: "ADXL345 not found"

**Solution:**
1. Check wiring - SDA must be on GPIO 33, SCL on GPIO 23
2. Make sure CS is connected to 3.3V
3. Make sure SDO is connected to GND
4. Check for loose connections

### Problem: "GPS no fix"

**Solution:**
1. Go outside - GPS needs clear sky view
2. Wait 5-10 minutes (first fix takes longest)
3. Check GPS antenna connection
4. Try a different location (away from tall buildings)

### Problem: "Modem not responding"

**Solution:**
1. Check SIM card is inserted correctly
2. Make sure SIM has active data plan
3. Check APN settings in config.h
4. Try power cycling the board

### Problem: "Server not responding"

**Solution:**
1. Check Railway/Vercel deployment is running
2. Verify SERVER_HOST in config.h is correct
3. Check Railway logs for errors
4. Make sure HTTPS is enabled (port 443)

### Problem: "Buzzer not working"

**Solution:**
1. Check transistor orientation (flat side, E-B-C order)
2. Verify 1K resistor is connected to Base
3. Test buzzer directly with 3.3V (should buzz)
4. Check GPIO 19 connection

### Problem: "Motor not working"

**Solution:**
1. Check transistor wiring (same as buzzer)
2. Verify diode orientation (stripe to +)
3. Test motor directly with 3.3V (should vibrate)
4. Check GPIO 25 connection

---

## QUICK REFERENCE CARD

Print this and keep it handy:

```
╔═══════════════════════════════════════════════════════════════╗
║              POPSTRACKER QUICK REFERENCE                      ║
╠═══════════════════════════════════════════════════════════════╣
║                                                               ║
║  WIRING (⚠️ I2C is NOT on GPIO 21/22!)                       ║
║  ───────────────────────────────────                          ║
║  ADXL345 SDA → GPIO 33    │    Buzzer → GPIO 19               ║
║  ADXL345 SCL → GPIO 23    │    Motor  → GPIO 25               ║
║  ADXL345 INT → GPIO 32    │    Battery → GPIO 35              ║
║                                                               ║
║  SERIAL COMMANDS                                              ║
║  ───────────────                                              ║
║  help     - Show all commands                                 ║
║  status   - Show device status                                ║
║  gps      - Show GPS coordinates                              ║
║  accel    - Show accelerometer data                           ║
║  beep     - Test buzzer                                       ║
║  vibrate  - Test motor                                        ║
║  finddog  - Activate Find My Dog                              ║
║  battery  - Show battery status                               ║
║  sethome  - Set current location as home                      ║
║  startwalk- Manually start walk                               ║
║  endwalk  - Manually end walk                                 ║
║                                                               ║
║  WALK GRADES                                                  ║
║  ───────────                                                  ║
║  A = 70%+ active time (Excellent!)                            ║
║  B = 50-69% active (Good)                                     ║
║  C = 30-49% active (Needs improvement)                        ║
║  F = <30% active or cheating detected                         ║
║                                                               ║
║  USEFUL URLS                                                  ║
║  ───────────                                                  ║
║  Railway Dashboard: https://railway.app/                      ║
║  PlatformIO Docs: https://docs.platformio.org/                ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝
```

---

## GETTING HELP

If you're stuck:

1. **Check the logs** - Serial monitor and Railway logs usually show the error
2. **Take photos** of your wiring and compare to diagrams
3. **Ask on Discord** - Join the PopsTracker community
4. **GitHub Issues** - Report bugs at the repository

Good luck with Popcorn's tracker! 🐕

