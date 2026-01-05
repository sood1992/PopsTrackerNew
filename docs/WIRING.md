# Hardware Wiring Guide - PopsTracker V7.0

Complete wiring instructions for the PopsTracker dog collar for **Popcorn**.

> **Note**: This guide reflects YOUR actual wiring configuration with an external GPS module.

---

## Pin Configuration Summary

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    POPSTRACKER V7.0 - PIN CONFIGURATION                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  I2C BUS (OLED + ADXL345 Accelerometer):                                   │
│  ───────────────────────────────────────                                    │
│    GPIO 21 = I2C_SDA     (Data line - shared by OLED & ADXL345)            │
│    GPIO 22 = I2C_SCL     (Clock line - shared by OLED & ADXL345)           │
│    GPIO 32 = ADXL345_INT1 (Interrupt for shake-to-wake)                    │
│                                                                             │
│  GPS MODULE (External UART):                                                │
│  ───────────────────────────                                                │
│    GPIO 16 = GPS_RX      (ESP32 receives from GPS TX)                      │
│    GPIO 17 = GPS_TX      (ESP32 sends to GPS RX)                           │
│                                                                             │
│  CELLULAR MODEM (A7670G):                                                   │
│  ────────────────────────                                                   │
│    GPIO 26 = MODEM_TX    (ESP32 sends to Modem RX)                         │
│    GPIO 27 = MODEM_RX    (ESP32 receives from Modem TX)                    │
│    GPIO 4  = MODEM_PWRKEY (Wake/sleep modem)                               │
│    GPIO 12 = BOARD_POWER_HOLD                                              │
│                                                                             │
│  MICROSD CARD (SPI):                                                        │
│  ───────────────────                                                        │
│    GPIO 13 = SD_CS       (Chip Select)                                     │
│    GPIO 14 = SD_SCK      (SPI Clock)                                       │
│    GPIO 15 = SD_MOSI     (Master Out Slave In)                             │
│    GPIO 2  = SD_MISO     (Master In Slave Out) ⚠️ Strapping pin!           │
│                                                                             │
│  OUTPUTS:                                                                   │
│  ────────                                                                   │
│    GPIO 19 = BUZZER_PIN  (Active HIGH)                                     │
│    GPIO 25 = MOTOR_PIN   (Via transistor/MOSFET)                           │
│                                                                             │
│  BATTERY MONITORING:                                                        │
│  ───────────────────                                                        │
│    GPIO 35 = BAT_ADC     (Voltage divider input)                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Wiring Diagrams

### 1. I2C Bus (OLED Display + ADXL345)

Both the OLED display and ADXL345 accelerometer share the same I2C bus.

```
                                3.3V
                                 │
              ┌──────────────────┼──────────────────┐
              │                  │                  │
              │                  │                  │
    ┌─────────┴─────────┐  ┌─────┴─────┐  ┌─────────┴─────────┐
    │     OLED          │  │  4.7K     │  │     ADXL345       │
    │   (SSD1306)       │  │ Pull-ups  │  │   Accelerometer   │
    │                   │  │ (if needed│  │                   │
    │  VCC ──── 3.3V    │  │           │  │  VCC ──── 3.3V    │
    │  GND ──── GND     │  │     │     │  │  GND ──── GND     │
    │  SDA ────┐        │  │     │     │  │  SDA ────┐        │
    │  SCL ────┼───┐    │  └─────┼─────┘  │  SCL ────┼───┐    │
    └──────────┼───┼────┘        │        │  CS ───── 3.3V    │
               │   │             │        │  SDO ──── GND     │
               │   │             │        │  INT1 ───┐        │
               │   │             │        └──────────┼────────┘
               │   │             │                   │
               │   │             │                   │
               ▼   ▼             │                   ▼
    ┌──────────────────────────────────────────────────────────┐
    │                   LILYGO T-A7670G R2                     │
    │                                                          │
    │  GPIO 21 ●───────────────────── SDA Bus                  │
    │  GPIO 22 ●───────────────────── SCL Bus                  │
    │  GPIO 32 ●───────────────────── ADXL345 INT1             │
    │                                                          │
    └──────────────────────────────────────────────────────────┘

    I2C ADDRESSES:
    ══════════════
    • OLED (SSD1306): 0x3C or 0x3D
    • ADXL345:        0x53 (SDO→GND) or 0x1D (SDO→VCC)
```

### 2. External GPS Module

Using external GPS on dedicated UART (not the built-in L76K).

```
    GPS MODULE                       LILYGO T-A7670G R2
    (NEO-6M, etc.)                   ┌─────────────────┐
    ┌───────────┐                    │                 │
    │           │                    │                 │
    │    VCC  ●─┼───── Red ──────────┼─● 3.3V          │
    │           │                    │                 │
    │    GND  ●─┼───── Black ────────┼─● GND           │
    │           │                    │                 │
    │    TX   ●─┼───── Green ────────┼─● GPIO 16 (RX)  │
    │           │     (GPS TX →      │                 │
    │           │      ESP32 RX)     │                 │
    │    RX   ●─┼───── Yellow ───────┼─● GPIO 17 (TX)  │
    │           │     (ESP32 TX →    │                 │
    │           │      GPS RX)       │                 │
    └───────────┘                    └─────────────────┘

    IMPORTANT:
    ══════════
    • TX → RX (crossed connection, not straight!)
    • GPS modules typically output at 9600 baud
    • Make sure GPS has clear sky view for fix
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
| I2C SDA | 21 | Bidirectional | OLED + ADXL345 |
| I2C SCL | 22 | Output | OLED + ADXL345 |
| ADXL345 INT1 | 32 | Input | Shake-to-wake interrupt |
| GPS RX | 16 | Input | External GPS module TX |
| GPS TX | 17 | Output | External GPS module RX |
| Modem TX | 26 | Output | A7670G modem RX |
| Modem RX | 27 | Input | A7670G modem TX |
| Modem PWR | 4 | Output | Wake/sleep modem |
| Power Hold | 12 | Output | Board power control |
| SD CS | 13 | Output | SD card chip select |
| SD SCK | 14 | Output | SD card clock |
| SD MOSI | 15 | Output | SD card data out |
| SD MISO | 2 | Input | SD card data in |
| Buzzer | 19 | Output | Alert sounds |
| Motor | 25 | Output | Vibration feedback |
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
- [ ] OLED: VCC→3.3V, GND→GND, SDA→G21, SCL→G22
- [ ] ADXL345: VCC→3.3V, GND→GND, SDA→G21, SCL→G22, CS→3.3V, SDO→GND, INT1→G32
- [ ] GPS: VCC→3.3V, GND→GND, TX→G16, RX→G17
- [ ] SD Card: 3V3→3.3V, GND→GND, CS→G13, SCK→G14, MOSI→G15, MISO→G2
- [ ] Buzzer: Circuit connected to G19
- [ ] Motor: Circuit connected to G25
- [ ] Battery divider: Output to G35
- [ ] SIM card inserted
- [ ] Antennas connected

---

## Next Steps

1. **[SETUP.md](SETUP.md)** - Software installation and firmware upload
2. **[DEPLOYMENT.md](DEPLOYMENT.md)** - Backend and frontend deployment
