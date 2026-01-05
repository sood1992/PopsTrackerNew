/**
 * Device Routes - Handle device registration and configuration
 */

const express = require('express');
const router = express.Router();
const Device = require('../models/Device');

// POST /api/v1/device/register - Register new device
router.post('/register', async (req, res) => {
    try {
        const {
            device_id,
            dog_name,
            owner_name,
            owner_email,
            owner_phone,
            home_lat,
            home_lon,
            firmware_version
        } = req.body;

        const device = await Device.findOneAndUpdate(
            { deviceId: device_id },
            {
                $set: {
                    deviceId: device_id,
                    dogName: dog_name,
                    ownerName: owner_name,
                    ownerEmail: owner_email,
                    ownerPhone: owner_phone,
                    firmwareVersion: firmware_version,
                    lastSeen: new Date(),
                    isOnline: true
                },
                $setOnInsert: {
                    homeLocation: {
                        type: 'Point',
                        coordinates: [home_lon || 0, home_lat || 0]
                    }
                }
            },
            { upsert: true, new: true }
        );

        res.status(201).json({
            success: true,
            deviceId: device.deviceId,
            message: 'Device registered successfully'
        });
    } catch (error) {
        console.error('Device registration error:', error);
        res.status(500).json({ error: 'Failed to register device' });
    }
});

// GET /api/v1/device/:deviceId - Get device info
router.get('/:deviceId', async (req, res) => {
    try {
        const device = await Device.findOne({ deviceId: req.params.deviceId });
        if (!device) {
            return res.status(404).json({ error: 'Device not found' });
        }
        res.json(device);
    } catch (error) {
        console.error('Device fetch error:', error);
        res.status(500).json({ error: 'Failed to fetch device' });
    }
});

// PUT /api/v1/device/:deviceId - Update device settings
router.put('/:deviceId', async (req, res) => {
    try {
        const {
            dog_name,
            owner_name,
            owner_email,
            owner_phone,
            home_lat,
            home_lon,
            home_radius,
            notifications
        } = req.body;

        const update = {};
        if (dog_name) update.dogName = dog_name;
        if (owner_name) update.ownerName = owner_name;
        if (owner_email) update.ownerEmail = owner_email;
        if (owner_phone) update.ownerPhone = owner_phone;
        if (home_radius) update.homeRadius = home_radius;
        if (notifications) update.notifications = notifications;

        if (home_lat && home_lon) {
            update.homeLocation = {
                type: 'Point',
                coordinates: [home_lon, home_lat]
            };
        }

        const device = await Device.findOneAndUpdate(
            { deviceId: req.params.deviceId },
            { $set: update },
            { new: true }
        );

        if (!device) {
            return res.status(404).json({ error: 'Device not found' });
        }

        res.json(device);
    } catch (error) {
        console.error('Device update error:', error);
        res.status(500).json({ error: 'Failed to update device' });
    }
});

// POST /api/v1/device/:deviceId/heartbeat - Receive heartbeat
router.post('/:deviceId/heartbeat', async (req, res) => {
    try {
        const {
            battery_voltage,
            battery_percent,
            is_charging,
            gps_connected,
            signal_strength,
            uptime,
            firmware_version
        } = req.body;

        const device = await Device.findOneAndUpdate(
            { deviceId: req.params.deviceId },
            {
                $set: {
                    lastSeen: new Date(),
                    isOnline: true,
                    batteryLevel: battery_percent,
                    firmwareVersion: firmware_version
                }
            },
            { new: true }
        );

        if (!device) {
            return res.status(404).json({ error: 'Device not found' });
        }

        // Emit status update
        const io = req.app.get('io');
        io.to(req.params.deviceId).emit('heartbeat', {
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

// POST /api/v1/device/:deviceId/set-home - Set home location
router.post('/:deviceId/set-home', async (req, res) => {
    try {
        const { lat, lon, radius } = req.body;

        const device = await Device.findOneAndUpdate(
            { deviceId: req.params.deviceId },
            {
                $set: {
                    homeLocation: {
                        type: 'Point',
                        coordinates: [lon, lat]
                    },
                    homeRadius: radius || 50
                }
            },
            { new: true }
        );

        if (!device) {
            return res.status(404).json({ error: 'Device not found' });
        }

        res.json({
            success: true,
            homeLocation: { lat, lon },
            homeRadius: device.homeRadius
        });
    } catch (error) {
        console.error('Set home error:', error);
        res.status(500).json({ error: 'Failed to set home location' });
    }
});

// POST /api/v1/device/:deviceId/walker - Add/update walker
router.post('/:deviceId/walker', async (req, res) => {
    try {
        const { name, phone, email } = req.body;

        const device = await Device.findOneAndUpdate(
            {
                deviceId: req.params.deviceId,
                'walkers.name': { $ne: name }
            },
            {
                $push: {
                    walkers: { name, phone, email, totalWalks: 0 }
                }
            },
            { new: true }
        );

        if (!device) {
            // Walker might already exist, try updating
            await Device.findOneAndUpdate(
                {
                    deviceId: req.params.deviceId,
                    'walkers.name': name
                },
                {
                    $set: {
                        'walkers.$.phone': phone,
                        'walkers.$.email': email
                    }
                }
            );
        }

        res.json({ success: true });
    } catch (error) {
        console.error('Add walker error:', error);
        res.status(500).json({ error: 'Failed to add walker' });
    }
});

// GET /api/v1/device/:deviceId/walkers - Get all walkers
router.get('/:deviceId/walkers', async (req, res) => {
    try {
        const device = await Device.findOne({ deviceId: req.params.deviceId });
        if (!device) {
            return res.status(404).json({ error: 'Device not found' });
        }
        res.json(device.walkers || []);
    } catch (error) {
        console.error('Get walkers error:', error);
        res.status(500).json({ error: 'Failed to fetch walkers' });
    }
});

module.exports = router;
