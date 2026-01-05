# Hardware Wiring Guide - PopsTracker V7.0

Complete wiring instructions for the PopsTracker dog collar for **Popcorn**.

> **CRITICAL NOTE**: This board uses the **ONBOARD L76K GPS** on GPIO 21/22.
> Therefore, **I2C MUST use GPIO 33/23** instead of the standard 21/22!

---

## Pin Configuration Summary

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    POPSTRACKER V7.0 - PIN CONFIGURATION                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ⚠️ IMPORTANT: Onboard L76K GPS uses GPIO 21/22!                            │
│  ⚠️ I2C MUST use alternative pins (GPIO 33/23)!                             │
│                                                                             │
│  GPS MODULE (L76K - ONBOARD):                                               │
│  ────────────────────────────                                               │
│    GPIO 21 = GPS_TX      (ESP32 receives from onboard GPS)                  │
│    GPIO 22 = GPS_RX      (ESP32 sends to onboard GPS)                       │
│                                                                             │
│  I2C BUS (ADXL345 Accelerometer):                                           │
│  ─────────────────────────────────                                          │
│    GPIO 33 = I2C_SDA     (⚠️ NOT GPIO 21 - conflicts with GPS!)            │
│    GPIO 23 = I2C_SCL     (⚠️ NOT GPIO 22 - conflicts with GPS!)            │
│    GPIO 32 = ADXL345_INT1 (Interrupt for shake-to-wake)                     │
│                                                                             │
│  CELLULAR MODEM (A7670G - ONBOARD):                                         │
│  ──────────────────────────────────                                         │
│    GPIO 26 = MODEM_TX    (ESP32 sends to Modem RX)                          │
│    GPIO 27 = MODEM_RX    (ESP32 receives from Modem TX)                     │
│    GPIO 4  = MODEM_PWRKEY (Wake/sleep modem)                                │
│    GPIO 12 = BOARD_POWER_HOLD                                               │
│                                                                             │
│  OUTPUTS:                                                                   │
│  ────────                                                                   │
│    GPIO 19 = BUZZER_PIN  (Via transistor circuit)                           │
│    GPIO 25 = MOTOR_PIN   (Via transistor + flyback diode)                   │
│                                                                             │
│  BATTERY MONITORING:                                                        │
│  ───────────────────                                                        │
│    GPIO 35 = BAT_ADC     (Voltage divider input)                            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Wiring Diagrams

### 1. I2C Bus (ADXL345 Accelerometer)

**⚠️ CRITICAL: I2C uses GPIO 33/23, NOT GPIO 21/22!**

The onboard L76K GPS uses GPIO 21/22, so I2C MUST use alternative pins.

```
                                3.3V
                                 │
              ┌──────────────────┴──────────────────┐
              │                                     │
    ┌─────────┴─────────┐                 ┌─────────┴─────────┐
    │     ADXL345       │                 │   4.7K Pull-ups   │
    │   Accelerometer   │                 │   (if needed)     │
    │                   │                 └─────────┬─────────┘
    │  VCC ──── 3.3V    │                           │
    │  GND ──── GND     │                           │
    │  SDA ────┐        │                           │
    │  SCL ────┼───┐    │                           │
    │  CS ───── 3.3V    │  (enables I2C mode)       │
    │  SDO ──── GND     │  (sets address 0x53)      │
    │  INT1 ───┐        │                           │
    └──────────┼───┼────┘                           │
               │   │                                │
               │   │                                │
               ▼   ▼                                ▼
    ┌──────────────────────────────────────────────────────────┐
    │                   LILYGO T-A7670G R2                     │
    │                                                          │
    │  ⚠️ GPIO 21/22 are used by onboard GPS - DO NOT USE!     │
    │                                                          │
    │  GPIO 33 ●───────────────────── I2C SDA (Data)           │
    │  GPIO 23 ●───────────────────── I2C SCL (Clock)          │
    │  GPIO 32 ●───────────────────── ADXL345 INT1             │
    │                                                          │
    └──────────────────────────────────────────────────────────┘

    I2C ADDRESS:
    ═════════════
    • ADXL345: 0x53 (when SDO→GND)
```

### 2. GPS Module (L76K - ONBOARD)

This board has a built-in L76K GPS module. No external GPS needed!

```
    ┌──────────────────────────────────────────────────────────┐
    │                   LILYGO T-A7670G R2                     │
    │                                                          │
    │  ┌─────────────────────────────────────────────────┐     │
    │  │              ONBOARD L76K GPS                   │     │
    │  │                                                 │     │
    │  │  The GPS is built into the board!              │     │
    │  │  No external wiring needed.                    │     │
    │  │                                                 │     │
    │  │  Internal connections:                          │     │
    │  │    GPIO 21 ← GPS TX (receive data FROM GPS)     │     │
    │  │    GPIO 22 → GPS RX (send commands TO GPS)      │     │
    │  │                                                 │     │
    │  └─────────────────────────────────────────────────┘     │
    │                                                          │
    │  [GPS ANTENNA PORT] ← Connect GPS antenna here           │
    │                                                          │
    └──────────────────────────────────────────────────────────┘

    IMPORTANT:
    ══════════
    • Connect the GPS antenna to the GPS ANT port
    • GPS needs clear sky view for satellite fix
    • First fix may take 2-5 minutes outdoors
    • GPIO 21/22 are RESERVED for GPS - do not use for I2C!
```

### 3. Buzzer (GPIO 19)

Your setup uses GPIO 19 directly (Active HIGH). If using a transistor:

```
    OPTION A: Direct Drive (small piezo buzzers)
    ═══════════════════════════════════════════

                              ┌───────────┐
    GPIO 19 ──────────────────┤  BUZZER   │
                              │   (+)(-)  ├──── GND
                              └───────────┘

    Note: Only works for small buzzers <20mA


    OPTION B: Transistor Drive (recommended)
    ════════════════════════════════════════

                                      3.3V
                                       │
                           ┌───────────┴───────────┐
                           │                       │
                           │    ┌───────────┐      │
                           │    │  BUZZER   │      │
                           │    │   (+)(-)  │      │
                           │    └─────┬─────┘      │
                           │          │            │
                           │          ▼            │
                           │    ┌───────────┐      │
                           │    │  2N2222A  │      │
          GPIO 19 ────[1KΩ]────►B  (NPN)  C├──────┘
                                │     E     │
                                └─────┬─────┘
                                      │
                                     GND
```

### 4. Vibration Motor (GPIO 25)

Using a transistor or MOSFET to drive the motor.

```
                                      3.3V
                                       │
                           ┌───────────┴───────────┐
                           │                       │
                      ┌────┴────┐                  │
                      │ 1N4001  │ ← Flyback Diode  │
                      │ (stripe │   (REQUIRED!)    │
                      │  to +)  │                  │
                      └────┬────┘                  │
                           │                       │
                           ├───────────────────────┤
                           │                       │
                           │    ┌───────────┐      │
                           │    │  MOTOR    │      │
                           │    │  ◐ ◐ ◐    │      │
                           │    └─────┬─────┘      │
                           │          │            │
                           └──────────┤            │
                                      │            │
                                      ▼            │
                                ┌───────────┐      │
                                │  2N2222A  │      │
          GPIO 25 ─────[1KΩ]────►B  (NPN)  C├──────┘
                                │     E     │
                                └─────┬─────┘
                                      │
                                     GND
```

### 5. MicroSD Card (SPI)

```
    MICROSD MODULE                   LILYGO T-A7670G R2
    ┌───────────┐                    ┌─────────────────┐
    │           │                    │                 │
    │    3V3  ●─┼───── Red ──────────┼─● 3.3V          │
    │           │                    │                 │
    │    GND  ●─┼───── Black ────────┼─● GND           │
    │           │                    │                 │
    │    CS   ●─┼───── Orange ───────┼─● GPIO 13       │
    │           │                    │                 │
    │    SCK  ●─┼───── Yellow ───────┼─● GPIO 14       │
    │           │                    │                 │
    │    MOSI ●─┼───── Green ────────┼─● GPIO 15       │
    │           │                    │                 │
    │    MISO ●─┼───── Blue ─────────┼─● GPIO 2        │
    │           │                    │                 │
    └───────────┘                    └─────────────────┘

    ⚠️ WARNING: GPIO 2 is a strapping pin!
    ═══════════════════════════════════════
    If the SD card pulls GPIO 2 LOW during boot,
    the ESP32 may fail to start.

    SOLUTIONS:
    • Remove SD card when uploading new firmware
    • Add 10K pull-up resistor on MISO line
    • Use a different SD card module with buffer chip
```

### 6. Battery Voltage Monitoring (GPIO 35)

```
    BATTERY (3.7-4.2V)
         │
         │
    ┌────┴────┐
    │   R1    │  100K (voltage divider top)
    └────┬────┘
         │
         ├────────────────● GPIO 35 (ADC Input)
         │
    ┌────┴────┐
    │   R2    │  100K (voltage divider bottom)
    └────┬────┘
         │
        GND

    CALCULATION:
    ═════════════
    With R1 = R2 = 100K:
    V_GPIO35 = V_BAT × (R2 / (R1 + R2))
    V_GPIO35 = V_BAT × 0.5

    So 4.2V battery → 2.1V at GPIO
    ESP32 ADC range: 0-3.3V → Safe!

    In code: V_BAT = ADC_reading × 2 × (3.3 / 4095)
```

---

## Complete Wiring Summary

### Pin Assignment Table

| Function | GPIO | Direction | Component |
|----------|------|-----------|-----------|
| **I2C SDA** | **33** | Bidirectional | **ADXL345 (⚠️ NOT 21!)** |
| **I2C SCL** | **23** | Output | **ADXL345 (⚠️ NOT 22!)** |
| ADXL345 INT1 | 32 | Input | Shake-to-wake interrupt |
| **GPS TX** | **21** | Input | **Onboard L76K GPS (reserved!)** |
| **GPS RX** | **22** | Output | **Onboard L76K GPS (reserved!)** |
| Modem TX | 26 | Output | A7670G modem RX |
| Modem RX | 27 | Input | A7670G modem TX |
| Modem PWR | 4 | Output | Wake/sleep modem |
| Power Hold | 12 | Output | Board power control |
| Buzzer | 19 | Output | Alert sounds (via transistor) |
| Motor | 25 | Output | Vibration feedback (via transistor) |
| Battery ADC | 35 | Input | Voltage monitoring |

### Complete Schematic

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           POPSTRACKER V7.0 - COMPLETE WIRING                        │
└─────────────────────────────────────────────────────────────────────────────────────┘

                                         3.3V RAIL
                                            │
         ┌──────────────────────────────────┼──────────────────────────────────┐
         │                                  │                                  │
    ┌────┴────┐        ┌────────────────────┼────────────────────┐        ┌────┴────┐
    │  OLED   │        │                    │                    │        │  GPS    │
    │ SSD1306 │        │               ┌────┴────┐               │        │ Module  │
    │         │        │               │ ADXL345 │               │        │         │
    │ VCC─3.3V│        │               │         │               │        │ VCC─3.3V│
    │ GND─GND │        │               │ VCC─3.3V│               │        │ GND─GND │
    │ SDA─────┼────────┼───────────────┤ SDA     │               │        │ TX──────┼───→ G16
    │ SCL─────┼────────┼───────────────┤ SCL     │               │        │ RX──────┼───→ G17
    └─────────┘        │               │ CS──3.3V│               │        └─────────┘
                       │               │ SDO──GND│               │
                       │               │ INT1────┼───────────────┼──────→ G32
                       │               └─────────┘               │
                       │                                         │
    ┌─────────┐        │                                         │        ┌─────────┐
    │ SD CARD │        │                                         │        │ BUZZER  │
    │         │        │                                         │        │         │
    │ 3V3─3.3V│        │                                         │        │ (+)─3.3V│
    │ GND─GND │        │                                         │        │ (-)──┐  │
    │ CS──────┼───→ G13│                                         │        └──────┼──┘
    │ SCK─────┼───→ G14│                                         │               │
    │ MOSI────┼───→ G15│                                         │          [2N2222A]
    │ MISO────┼───→ G2 │                                         │               │
    └─────────┘        │                                         │          G19──┤
                       │                                         │               │
                       │                                         │              GND
                       │                                         │
                       │        ┌─────────────────────────────────┐
                       │        │    LILYGO T-A7670G R2          │
                       │        │                                 │
                       │        │    [A7670G MODEM - Internal]   │
                       │        │    TX=G26, RX=G27, PWR=G4      │
                       │        │                                 │
                       │        │    G21 ● ───── I2C SDA         │
                       │        │    G22 ● ───── I2C SCL         │
                       │        │    G32 ● ───── ADXL INT1       │
                       │        │    G16 ● ───── GPS RX          │
                       │        │    G17 ● ───── GPS TX          │
                       │        │    G19 ● ───── BUZZER          │
                       │        │    G25 ● ───── MOTOR           │
                       │        │    G35 ● ───── BAT ADC         │
                       │        │    G12 ● ───── PWR HOLD        │
                       │        │                                 │
                       │        │    [USB-C] [SIM] [ANTENNAS]    │
                       │        │    [────── 18650 BATTERY ─────]│
                       │        └─────────────────────────────────┘
                       │                                         │
    ┌─────────┐        │                                         │
    │VIB MOTOR│        │                                         │
    │ ◐ ◐ ◐   │        │                                         │
    │ (+)─3.3V│        │                                         │
    │ (-)─┐   │  ┌─────┴──────────────────────────────────────────┘
    └─────┼───┘  │ [1N4001 DIODE across motor terminals]
          │      │
     [2N2222A]   │
          │      │
     G25──┤      │
          │      │
         GND─────┘
```

---

## Pre-Assembly Checklist

### Components Needed
- [ ] LILYGO T-A7670G R2 board
- [ ] ADXL345 accelerometer module
- [ ] OLED display (SSD1306, 128x64 or 128x32)
- [ ] External GPS module (NEO-6M or similar)
- [ ] MicroSD card module
- [ ] Piezo buzzer (~12mm)
- [ ] Coin vibration motor (~10mm)
- [ ] 2x 2N2222A transistors
- [ ] 2x 1K resistors
- [ ] 1x 1N4001 diode
- [ ] 2x 100K resistors (for battery divider)
- [ ] Jumper wires
- [ ] Nano SIM card (Airtel IoT)
- [ ] 18650 battery
- [ ] GPS antenna (if using external)
- [ ] LTE antenna

### Wiring Verification

**⚠️ CRITICAL: I2C uses GPIO 33/23, NOT GPIO 21/22!**

- [ ] ADXL345: VCC→3.3V, GND→GND, **SDA→GPIO 33**, **SCL→GPIO 23**, CS→3.3V, SDO→GND, INT1→G32
- [ ] Buzzer: Transistor circuit connected to GPIO 19
- [ ] Motor: Transistor circuit + flyback diode connected to GPIO 25
- [ ] GPS Antenna connected to GPS ANT port
- [ ] LTE Antenna connected to LTE ANT port
- [ ] SIM card inserted (Nano SIM, chip facing down)
- [ ] 18650 Battery inserted (correct polarity)

---

## Next Steps

1. **[SETUP.md](SETUP.md)** - Software installation and firmware upload
2. **[DEPLOYMENT.md](DEPLOYMENT.md)** - Backend and frontend deployment
