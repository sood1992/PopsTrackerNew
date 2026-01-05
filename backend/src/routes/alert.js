/**
 * Alert Routes - Handle alerts and notifications
 */

const express = require('express');
const router = express.Router();
const Alert = require('../models/Alert');

// POST /api/v1/alert - Create new alert from device
router.post('/', async (req, res) => {
    try {
        const {
            device_id,
            alert_type,
            message,
            location,
            walk_id
        } = req.body;

        // Map alert type
        const alertTypeMap = {
            'LOW_BATTERY': 'LOW_BATTERY',
            'BAD_WALK': 'BAD_WALK',
            'SCRATCHING': 'SCRATCHING',
            'GEOFENCE_EXIT': 'GEOFENCE_EXIT',
            'GEOFENCE_ENTER': 'GEOFENCE_ENTER',
            'CARRIED': 'CARRIED',
            'VEHICLE': 'VEHICLE',
            'SOS': 'SOS'
        };

        const alert = new Alert({
            deviceId: device_id,
            alertType: alertTypeMap[alert_type] || 'CUSTOM',
            message: message,
            severity: determineSeverity(alert_type),
            location: location ? {
                type: 'Point',
                coordinates: [location.lon, location.lat]
            } : undefined,
            walkId: walk_id
        });

        await alert.save();

        // Emit real-time alert
        const io = req.app.get('io');
        io.to(device_id).emit('alert', {
            id: alert._id,
            type: alert.alertType,
            severity: alert.severity,
            message: alert.message,
            timestamp: alert.timestamp
        });

        // TODO: Send push notification, email, SMS based on device settings

        res.status(201).json({
            success: true,
            alertId: alert._id
        });
    } catch (error) {
        console.error('Alert create error:', error);
        res.status(500).json({ error: 'Failed to create alert' });
    }
});

// GET /api/v1/alert/:deviceId - Get alerts for device
router.get('/:deviceId', async (req, res) => {
    try {
        const limit = parseInt(req.query.limit) || 50;
        const unacknowledgedOnly = req.query.unacknowledged === 'true';

        let query = { deviceId: req.params.deviceId };
        if (unacknowledgedOnly) {
            query.acknowledged = false;
        }

        const alerts = await Alert.find(query)
            .sort({ timestamp: -1 })
            .limit(limit);

        res.json(alerts);
    } catch (error) {
        console.error('Alert fetch error:', error);
        res.status(500).json({ error: 'Failed to fetch alerts' });
    }
});

// GET /api/v1/alert/:deviceId/unacknowledged - Get unacknowledged alerts
router.get('/:deviceId/unacknowledged', async (req, res) => {
    try {
        const alerts = await Alert.getUnacknowledged(req.params.deviceId);
        res.json(alerts);
    } catch (error) {
        console.error('Alert fetch error:', error);
        res.status(500).json({ error: 'Failed to fetch alerts' });
    }
});

// PUT /api/v1/alert/:alertId/acknowledge - Acknowledge alert
router.put('/:alertId/acknowledge', async (req, res) => {
    try {
        const { acknowledged_by } = req.body;

        const alert = await Alert.findByIdAndUpdate(
            req.params.alertId,
            {
                $set: {
                    acknowledged: true,
                    acknowledgedAt: new Date(),
                    acknowledgedBy: acknowledged_by
                }
            },
            { new: true }
        );

        if (!alert) {
            return res.status(404).json({ error: 'Alert not found' });
        }

        res.json(alert);
    } catch (error) {
        console.error('Alert acknowledge error:', error);
        res.status(500).json({ error: 'Failed to acknowledge alert' });
    }
});

// PUT /api/v1/alert/:deviceId/acknowledge-all - Acknowledge all alerts
router.put('/:deviceId/acknowledge-all', async (req, res) => {
    try {
        const { acknowledged_by } = req.body;

        await Alert.updateMany(
            {
                deviceId: req.params.deviceId,
                acknowledged: false
            },
            {
                $set: {
                    acknowledged: true,
                    acknowledgedAt: new Date(),
                    acknowledgedBy: acknowledged_by
                }
            }
        );

        res.json({ success: true });
    } catch (error) {
        console.error('Acknowledge all error:', error);
        res.status(500).json({ error: 'Failed to acknowledge alerts' });
    }
});

// GET /api/v1/alert/:deviceId/stats - Get alert statistics
router.get('/:deviceId/stats', async (req, res) => {
    try {
        const days = parseInt(req.query.days) || 7;
        const since = new Date(Date.now() - days * 24 * 60 * 60 * 1000);

        const stats = await Alert.aggregate([
            {
                $match: {
                    deviceId: req.params.deviceId,
                    timestamp: { $gte: since }
                }
            },
            {
                $group: {
                    _id: '$alertType',
                    count: { $sum: 1 },
                    acknowledged: {
                        $sum: { $cond: ['$acknowledged', 1, 0] }
                    }
                }
            }
        ]);

        const result = {
            period: `${days} days`,
            totalAlerts: stats.reduce((sum, s) => sum + s.count, 0),
            byType: {}
        };

        stats.forEach(s => {
            result.byType[s._id] = {
                count: s.count,
                acknowledged: s.acknowledged
            };
        });

        res.json(result);
    } catch (error) {
        console.error('Alert stats error:', error);
        res.status(500).json({ error: 'Failed to fetch alert stats' });
    }
});

// Helper function to determine alert severity
function determineSeverity(alertType) {
    const severityMap = {
        'SOS': 'critical',
        'GEOFENCE_EXIT': 'warning',
        'BAD_WALK': 'warning',
        'VEHICLE': 'warning',
        'CARRIED': 'warning',
        'LOW_BATTERY': 'warning',
        'SCRATCHING': 'info',
        'GEOFENCE_ENTER': 'info',
        'DEVICE_OFFLINE': 'warning'
    };
    return severityMap[alertType] || 'info';
}

module.exports = router;
