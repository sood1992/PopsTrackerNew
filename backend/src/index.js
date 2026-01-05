/**
 * PopsTracker Backend API Server
 *
 * Handles GPS tracking data, walk sessions, and real-time updates
 */

require('dotenv').config();
const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const mongoose = require('mongoose');
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');

// Import routes
const locationRoutes = require('./routes/location');
const walkRoutes = require('./routes/walk');
const activityRoutes = require('./routes/activity');
const deviceRoutes = require('./routes/device');
const alertRoutes = require('./routes/alert');

// Initialize Express
const app = express();
const server = http.createServer(app);

// Initialize Socket.io for real-time updates
const io = new Server(server, {
    cors: {
        origin: process.env.FRONTEND_URL || '*',
        methods: ['GET', 'POST']
    }
});

// Make io accessible to routes
app.set('io', io);

// ============================================================================
// MIDDLEWARE
// ============================================================================
app.use(helmet());
app.use(cors());
app.use(morgan('combined'));
app.use(express.json({ limit: '10mb' }));
app.use(express.urlencoded({ extended: true }));

// Device ID validation middleware
app.use('/api/v1', (req, res, next) => {
    const deviceId = req.headers['x-device-id'] || req.body.device_id;
    if (!deviceId && req.method !== 'GET') {
        return res.status(400).json({ error: 'Device ID required' });
    }
    req.deviceId = deviceId;
    next();
});

// ============================================================================
// ROUTES
// ============================================================================
app.use('/api/v1/location', locationRoutes);
app.use('/api/v1/walk', walkRoutes);
app.use('/api/v1/activity', activityRoutes);
app.use('/api/v1/device', deviceRoutes);
app.use('/api/v1/alert', alertRoutes);

// Health check
app.get('/health', (req, res) => {
    res.json({
        status: 'healthy',
        uptime: process.uptime(),
        timestamp: new Date().toISOString()
    });
});

// Direct heartbeat endpoint (firmware sends to /api/v1/heartbeat)
const Device = require('./models/Device');
app.post('/api/v1/heartbeat', async (req, res) => {
    try {
        const {
            device_id,
            battery_voltage,
            battery_percent,
            is_charging,
            gps_connected,
            signal_strength,
            uptime,
            firmware_version
        } = req.body;

        const device = await Device.findOneAndUpdate(
            { deviceId: device_id },
            {
                $set: {
                    lastSeen: new Date(),
                    isOnline: true,
                    batteryLevel: battery_percent,
                    firmwareVersion: firmware_version
                }
            },
            { upsert: true, new: true }
        );

        // Emit status update
        const ioInstance = req.app.get('io');
        ioInstance.to(device_id).emit('heartbeat', {
            batteryPercent: battery_percent,
            batteryVoltage: battery_voltage,
            isCharging: is_charging,
            gpsConnected: gps_connected,
            signalStrength: signal_strength,
            uptime: uptime
        });

        res.json({ success: true });
    } catch (error) {
        console.error('Heartbeat error:', error);
        res.status(500).json({ error: 'Failed to process heartbeat' });
    }
});

// API docs
app.get('/api/v1', (req, res) => {
    res.json({
        name: 'PopsTracker API',
        version: '1.0.0',
        endpoints: {
            location: '/api/v1/location',
            walk: '/api/v1/walk',
            activity: '/api/v1/activity',
            device: '/api/v1/device',
            alert: '/api/v1/alert'
        }
    });
});

// ============================================================================
// SOCKET.IO
// ============================================================================
io.on('connection', (socket) => {
    console.log(`Client connected: ${socket.id}`);

    // Join device room for targeted updates
    socket.on('subscribe', (deviceId) => {
        socket.join(deviceId);
        console.log(`Client ${socket.id} subscribed to device: ${deviceId}`);
    });

    // Handle manual walk controls from dashboard
    socket.on('startWalk', (deviceId) => {
        io.to(deviceId).emit('command', { type: 'START_WALK' });
    });

    socket.on('endWalk', (deviceId) => {
        io.to(deviceId).emit('command', { type: 'END_WALK' });
    });

    socket.on('findDog', (deviceId) => {
        io.to(deviceId).emit('command', { type: 'FIND_DOG' });
    });

    socket.on('disconnect', () => {
        console.log(`Client disconnected: ${socket.id}`);
    });
});

// ============================================================================
// DATABASE CONNECTION
// ============================================================================
const MONGODB_URI = process.env.MONGODB_URI || 'mongodb://localhost:27017/popstracker';

mongoose.connect(MONGODB_URI)
    .then(() => {
        console.log('Connected to MongoDB');
    })
    .catch((err) => {
        console.error('MongoDB connection error:', err);
        process.exit(1);
    });

// ============================================================================
// ERROR HANDLING
// ============================================================================
app.use((err, req, res, next) => {
    console.error(err.stack);
    res.status(500).json({
        error: 'Internal server error',
        message: process.env.NODE_ENV === 'development' ? err.message : undefined
    });
});

// 404 handler
app.use((req, res) => {
    res.status(404).json({ error: 'Not found' });
});

// ============================================================================
// START SERVER
// ============================================================================
const PORT = process.env.PORT || 3000;

server.listen(PORT, () => {
    console.log(`
╔═══════════════════════════════════════════════╗
║         PopsTracker Backend Server            ║
╠═══════════════════════════════════════════════╣
║  Port: ${PORT}                                    ║
║  MongoDB: ${MONGODB_URI.split('@').pop() || 'localhost'}
║  Environment: ${process.env.NODE_ENV || 'development'}
╚═══════════════════════════════════════════════╝
    `);
});

module.exports = { app, io };
