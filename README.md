# PopsTracker - Dog Collar GPS & Activity Tracker

A comprehensive dog collar tracker to monitor your dog's activity, track walks, and hold dog walkers accountable. Built for **Popcorn** (but works for any pup!).

## Features

### Activity & Health Monitoring
- **Step Counting** - ADXL345 accelerometer-based
- **Activity Level Detection** - Resting / Light / Active / Very Active
- **Daily Active Minutes** - Cumulative tracking
- **Movement Detection** - Is dog moving or stationary?
- **Sleep Detection** - Tracks rest periods
- **Scratching/Licking Detection** - Pattern recognition (indicates skin issues)

### Walk Session Tracking (Walker Accountability)
| Feature | Description |
|---------|-------------|
| Auto Walk Detection | Starts when movement begins outside home |
| Walk Duration | Total time of walk |
| Walk Distance | GPS-calculated distance covered |
| Active vs Stationary Time | How much was actual walking |
| Walk Grading | A/B/C/F based on activity percentage |
| Walk History | All walks with maps and stats |
| Walker Report Card | Weekly summary with average scores |

### Walk Scoring System
| Grade | Active % | Meaning |
|-------|----------|---------|
| A | ≥70% | Excellent walk |
| B | ≥50% | Good walk |
| C | ≥30% | Acceptable |
| F | <30% | Walker slacking! |

### Live GPS Tracking
- Real-time location updates
- Geofencing (home zone detection)
- Find My Dog feature
- Walk route mapping

## Hardware Components

### Main Board
- **LILYGO T-A7670G R2** - ESP32 with 4G LTE modem and GPS

### Sensors
- **ADXL345** - 3-axis accelerometer

### Output Devices
- **Piezo Buzzer** (~12mm) - For alerts
- **Vibration Motor** (coin type ~10mm) - For haptic feedback

### Supporting Components
- 2x 2N2222A Transistors - Driving buzzer and motor
- 2x 1K Resistors - Base resistors for transistors
- 1x 1N4001 Flyback Diode - Motor protection
- Capacitors - Decoupling/filtering
- Jumper Wires
- LiPo Battery (or 18650)
- M2M SIM Card (Airtel IoT)

## Project Structure

```
PopsTrackerNew/
├── firmware/              # ESP32 Arduino code
│   ├── src/
│   │   ├── main.cpp
│   │   ├── gps.cpp
│   │   ├── accelerometer.cpp
│   │   ├── cellular.cpp
│   │   ├── activity.cpp
│   │   └── config.h
│   └── platformio.ini
├── backend/               # Node.js API server
│   ├── src/
│   │   ├── index.js
│   │   ├── routes/
│   │   ├── models/
│   │   ├── services/
│   │   └── middleware/
│   ├── package.json
│   └── Dockerfile
├── frontend/              # React dashboard
│   ├── src/
│   ├── public/
│   ├── package.json
│   └── Dockerfile
├── docs/                  # Documentation
│   ├── WIRING.md
│   ├── SETUP.md
│   └── DEPLOYMENT.md
├── docker-compose.yml
└── README.md
```

## Quick Start

See [docs/SETUP.md](docs/SETUP.md) for complete setup instructions.

## License

MIT License - Built with love for Popcorn!
