# PopsTracker Setup Guide

Complete guide for setting up the PopsTracker dog collar system from scratch.

## Table of Contents
1. [Hardware Assembly](#hardware-assembly)
2. [Firmware Installation](#firmware-installation)
3. [Backend Deployment](#backend-deployment)
4. [Frontend Deployment](#frontend-deployment)
5. [Configuration](#configuration)
6. [Testing](#testing)

---

## 1. Hardware Assembly

### Required Tools
- Soldering iron (if needed)
- Wire strippers
- Multimeter (for testing)
- Small screwdrivers

### Assembly Steps

1. **Prepare the LILYGO T-A7670G R2 board**
   - Insert Nano SIM card (Airtel IoT)
   - Connect GPS antenna to GPS ANT port
   - Connect LTE antenna to LTE ANT port
   - Insert 18650 battery (charged)

2. **Connect ADXL345 Accelerometer**

   **⚠️ CRITICAL: I2C uses GPIO 33/23, NOT GPIO 21/22!**
   (GPIO 21/22 are used by the onboard L76K GPS)

   ```
   ADXL345  →  ESP32
   VCC      →  3.3V
   GND      →  GND
   SDA      →  GPIO 33  (⚠️ NOT GPIO 21!)
   SCL      →  GPIO 23  (⚠️ NOT GPIO 22!)
   CS       →  3.3V (for I2C mode)
   SDO      →  GND (for address 0x53)
   INT1     →  GPIO 32
   ```

3. **Connect Buzzer Circuit**
   - Buzzer (+) → 3.3V
   - Buzzer (-) → 2N2222A Collector
   - 1K resistor between GPIO 19 and 2N2222A Base
   - 2N2222A Emitter → GND

4. **Connect Vibration Motor Circuit**
   - Motor (+) → 3.3V
   - Motor (-) → 2N2222A Collector
   - 1N4001 diode across motor (cathode to +)
   - 1K resistor between GPIO 25 and 2N2222A Base
   - 2N2222A Emitter → GND

5. **Verify Connections**
   - Use multimeter to check for shorts
   - Verify 3.3V and GND connections
   - Check I2C pull-ups (if needed)

See [WIRING.md](WIRING.md) for detailed diagrams.

---

## 2. Firmware Installation

### Prerequisites
- [PlatformIO](https://platformio.org/) installed (VS Code extension recommended)
- USB-C cable
- Remove SD card before programming

### Steps

1. **Clone the repository**
   ```bash
   git clone https://github.com/your-repo/PopsTrackerNew.git
   cd PopsTrackerNew/firmware
   ```

2. **Configure your settings**

   Edit `src/config.h`:
   ```cpp
   // Your server details
   #define SERVER_HOST         "your-server.com"
   #define SERVER_PORT         443

   // APN for your SIM card
   #define APN_NAME            "airteliot.com"

   // Home location (get from Google Maps)
   #define HOME_LATITUDE       28.6139
   #define HOME_LONGITUDE      77.2090
   #define HOME_RADIUS_METERS  50

   // Device ID
   #define DEVICE_ID           "POPCORN_001"
   #define DOG_NAME            "Popcorn"
   ```

3. **Build the firmware**
   ```bash
   # Using PlatformIO CLI
   pio run

   # Or in VS Code: Click the checkmark in bottom toolbar
   ```

4. **Upload to board**
   ```bash
   # Connect USB-C cable to the PROGRAMMING port (not UART)
   # Remove SD card if inserted

   pio run --target upload

   # Or in VS Code: Click the arrow in bottom toolbar
   ```

5. **Open Serial Monitor**
   ```bash
   pio device monitor --baud 115200
   ```

   You should see:
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
   Connected to network!
   === SYSTEM READY ===
   ```

### Troubleshooting

| Problem | Solution |
|---------|----------|
| Upload fails | Remove SD card, use correct USB port |
| ADXL345 not found | Check I2C wiring, verify address with I2C scanner |
| GPS no fix | Move outdoors, check antenna connection |
| Modem not responding | Check SIM card, verify APN settings |
| No power | Check battery polarity, verify power switch |

---

## 3. Backend Deployment

### Option A: Local Development

1. **Install dependencies**
   ```bash
   cd backend
   npm install
   ```

2. **Start MongoDB**
   ```bash
   # Using Docker
   docker run -d -p 27017:27017 --name popstracker-mongo mongo:latest

   # Or install MongoDB locally
   ```

3. **Configure environment**
   ```bash
   cp .env.example .env
   # Edit .env with your settings
   ```

   `.env` file:
   ```
   PORT=3000
   MONGODB_URI=mongodb://localhost:27017/popstracker
   NODE_ENV=development
   ```

4. **Start server**
   ```bash
   npm run dev
   ```

### Option B: Deploy to Railway (Recommended - Free Tier)

1. **Create Railway account** at [railway.app](https://railway.app)

2. **Create new project**
   - Click "New Project"
   - Select "Deploy from GitHub repo"
   - Choose your PopsTracker repository

3. **Add MongoDB**
   - Click "New" → "Database" → "MongoDB"
   - Railway will auto-configure the connection

4. **Configure variables**
   - Go to your service → Variables
   - Add:
     - `NODE_ENV=production`

5. **Deploy**
   - Railway auto-deploys on git push
   - Get your domain from Settings → Domains

### Option C: Deploy to DigitalOcean

1. **Create Droplet**
   - Ubuntu 22.04
   - 1GB RAM minimum
   - Enable monitoring

2. **Setup server**
   ```bash
   # SSH into server
   ssh root@your-server-ip

   # Install Node.js
   curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
   apt-get install -y nodejs

   # Install MongoDB
   wget -qO - https://www.mongodb.org/static/pgp/server-7.0.asc | apt-key add -
   echo "deb http://repo.mongodb.org/apt/ubuntu jammy/mongodb-org/7.0 multiverse" | tee /etc/apt/sources.list.d/mongodb-org-7.0.list
   apt-get update
   apt-get install -y mongodb-org
   systemctl start mongod
   systemctl enable mongod

   # Clone and setup
   git clone https://github.com/your-repo/PopsTrackerNew.git
   cd PopsTrackerNew/backend
   npm install --production

   # Install PM2 for process management
   npm install -g pm2
   pm2 start src/index.js --name popstracker
   pm2 save
   pm2 startup

   # Setup nginx (optional, for SSL)
   apt-get install -y nginx certbot python3-certbot-nginx
   ```

3. **Configure Nginx**
   ```nginx
   server {
       listen 80;
       server_name your-domain.com;

       location / {
           proxy_pass http://localhost:3000;
           proxy_http_version 1.1;
           proxy_set_header Upgrade $http_upgrade;
           proxy_set_header Connection 'upgrade';
           proxy_set_header Host $host;
           proxy_cache_bypass $http_upgrade;
       }
   }
   ```

4. **Enable SSL**
   ```bash
   certbot --nginx -d your-domain.com
   ```

---

## 4. Frontend Deployment

### Option A: Serve with Backend

The frontend is static HTML and can be served from the backend:

1. **Copy frontend files**
   ```bash
   cp -r frontend/public backend/public
   ```

2. **Add static file serving to backend**

   Add to `backend/src/index.js`:
   ```javascript
   app.use(express.static('public'));
   ```

### Option B: Deploy to Vercel (Recommended)

1. **Install Vercel CLI**
   ```bash
   npm install -g vercel
   ```

2. **Deploy**
   ```bash
   cd frontend
   vercel
   ```

3. **Configure API URL**
   - Set `CONFIG.serverUrl` in `index.html` to your backend URL

### Option C: Deploy to Netlify

1. **Create `netlify.toml`**
   ```toml
   [build]
     publish = "public"
   ```

2. **Deploy via Netlify CLI or GitHub integration**

---

## 5. Configuration

### Set Home Location

1. **From Serial Console**
   ```
   sethome
   ```
   (Sets current GPS location as home)

2. **From Dashboard**
   - Open dashboard
   - Navigate to Settings
   - Enter home coordinates

3. **Via API**
   ```bash
   curl -X POST https://your-server.com/api/v1/device/POPCORN_001/set-home \
     -H "Content-Type: application/json" \
     -d '{"lat": 28.6139, "lon": 77.2090, "radius": 50}'
   ```

### Configure Notifications

Edit device settings via API:
```bash
curl -X PUT https://your-server.com/api/v1/device/POPCORN_001 \
  -H "Content-Type: application/json" \
  -d '{
    "notifications": {
      "pushEnabled": true,
      "emailEnabled": true,
      "badWalkAlert": true
    }
  }'
```

### Add Walker

```bash
curl -X POST https://your-server.com/api/v1/device/POPCORN_001/walker \
  -H "Content-Type: application/json" \
  -d '{"name": "John Walker", "phone": "+919876543210"}'
```

---

## 6. Testing

### Hardware Test

1. **Serial Console Commands**
   ```
   help      - Show all commands
   status    - Show device status
   gps       - Show GPS data
   accel     - Show accelerometer data
   beep      - Test buzzer
   vibrate   - Test motor
   ```

2. **Component Test Sketch**
   Upload `test_components.ino` from WIRING.md

### API Test

```bash
# Health check
curl https://your-server.com/health

# Get device info
curl https://your-server.com/api/v1/device/POPCORN_001

# Get recent walks
curl https://your-server.com/api/v1/walk/POPCORN_001

# Get weekly summary
curl https://your-server.com/api/v1/walk/POPCORN_001/weekly-summary
```

### Walk Test

1. Ensure GPS has fix (take outdoors)
2. Walk away from home location (>30m)
3. Walk for at least 2 minutes
4. Return home
5. Check dashboard for walk report

---

## Next Steps

- [DEPLOYMENT.md](DEPLOYMENT.md) - Advanced deployment options
- [WIRING.md](WIRING.md) - Hardware wiring details
- Join our Discord for support!
