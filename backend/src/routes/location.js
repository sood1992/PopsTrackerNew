/**
 * Location Routes - Handle GPS tracking data
 */

const express = require('express');
const router = express.Router();
const Location = require('../models/Location');
const Device = require('../models/Device');

// POST /api/v1/location - Receive location from device
router.post('/', async (req, res) => {
    try {
        const {
            device_id,
            dog_name,
            location,
            activity,
            timestamp
        } = req.body;

        // Create location record
        const locationData = new Location({
            deviceId: device_id,
            dogName: dog_name,
            location: {
                type: 'Point',
                coordinates: [location.lon, location.lat]  // GeoJSON: [lon, lat]
            },
            altitude: location.alt,
            speed: location.speed,
            course: location.course,
            satellites: location.satellites,
            activity: {
                steps: activity.steps,
                activeMinutes: activity.active_minutes,
                level: activity.level,
                isMoving: activity.is_moving,
                isSleeping: activity.is_sleeping,
                isScratching: activity.is_scratching
            },
            timestamp: new Date(timestamp)
        });

        await locationData.save();

        // Update device's current location
        await Device.findOneAndUpdate(
            { deviceId: device_id },
            {
                $set: {
                    currentLocation: {
                        lat: location.lat,
                        lon: location.lon,
                        speed: location.speed,
                        timestamp: new Date()
                    },
                    lastSeen: new Date(),
                    isOnline: true
                }
            },
            { upsert: true }
        );

        // Emit real-time update
        const io = req.app.get('io');
        io.to(device_id).emit('location', {
            lat: location.lat,
            lon: location.lon,
            speed: location.speed,
            activity: activity.level,
            timestamp: new Date()
        });

        res.status(201).json({ success: true });
    } catch (error) {
        console.error('Location save error:', error);
        res.status(500).json({ error: 'Failed to save location' });
    }
});

// GET /api/v1/location/:deviceId - Get current location
router.get('/:deviceId', async (req, res) => {
    try {
        const location = await Location.getLatest(req.params.deviceId);
        if (!location) {
            return res.status(404).json({ error: 'No location data found' });
        }
        res.json(location);
    } catch (error) {
        console.error('Location fetch error:', error);
        res.status(500).json({ error: 'Failed to fetch location' });
    }
});

// GET /api/v1/location/:deviceId/history - Get location history
router.get('/:deviceId/history', async (req, res) => {
    try {
        const hours = parseInt(req.query.hours) || 24;
        const locations = await Location.getHistory(req.params.deviceId, hours);
        res.json(locations);
    } catch (error) {
        console.error('Location history error:', error);
        res.status(500).json({ error: 'Failed to fetch location history' });
    }
});

// GET /api/v1/location/:deviceId/live - SSE for live updates
router.get('/:deviceId/live', (req, res) => {
    res.setHeader('Content-Type', 'text/event-stream');
    res.setHeader('Cache-Control', 'no-cache');
    res.setHeader('Connection', 'keep-alive');

    const deviceId = req.params.deviceId;

    // Send initial connection message
    res.write(`data: ${JSON.stringify({ type: 'connected', deviceId })}\n\n`);

    // Listen for location updates
    const io = req.app.get('io');
    const handler = (data) => {
        res.write(`data: ${JSON.stringify(data)}\n\n`);
    };

    io.on(`location:${deviceId}`, handler);

    // Clean up on disconnect
    req.on('close', () => {
        io.off(`location:${deviceId}`, handler);
    });
});

module.exports = router;
