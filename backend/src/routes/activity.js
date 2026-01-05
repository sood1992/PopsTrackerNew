/**
 * Activity Routes - Handle activity and health data
 */

const express = require('express');
const router = express.Router();
const Location = require('../models/Location');
const Walk = require('../models/Walk');

// GET /api/v1/activity/:deviceId - Get current activity
router.get('/:deviceId', async (req, res) => {
    try {
        const latest = await Location.getLatest(req.params.deviceId);
        if (!latest) {
            return res.status(404).json({ error: 'No activity data found' });
        }

        res.json({
            steps: latest.activity?.steps || 0,
            activeMinutes: latest.activity?.activeMinutes || 0,
            activityLevel: latest.activity?.level || 0,
            isMoving: latest.activity?.isMoving || false,
            isSleeping: latest.activity?.isSleeping || false,
            isScratching: latest.activity?.isScratching || false,
            timestamp: latest.timestamp
        });
    } catch (error) {
        console.error('Activity fetch error:', error);
        res.status(500).json({ error: 'Failed to fetch activity' });
    }
});

// GET /api/v1/activity/:deviceId/daily - Get daily activity summary
router.get('/:deviceId/daily', async (req, res) => {
    try {
        const startOfDay = new Date();
        startOfDay.setHours(0, 0, 0, 0);

        // Get all locations for today
        const locations = await Location.find({
            deviceId: req.params.deviceId,
            timestamp: { $gte: startOfDay }
        });

        // Get today's walks
        const walks = await Walk.getToday(req.params.deviceId);

        // Calculate daily stats
        let totalSteps = 0;
        let activeMinutes = 0;
        let scratchingEvents = 0;

        locations.forEach(loc => {
            if (loc.activity?.steps > totalSteps) {
                totalSteps = loc.activity.steps;
            }
            if (loc.activity?.activeMinutes > activeMinutes) {
                activeMinutes = loc.activity.activeMinutes;
            }
            if (loc.activity?.isScratching) {
                scratchingEvents++;
            }
        });

        const totalWalkMinutes = walks.reduce((sum, walk) => sum + (walk.duration || 0), 0) / 60;
        const totalWalkDistance = walks.reduce((sum, walk) => sum + (walk.distanceMeters || 0), 0);

        res.json({
            date: startOfDay.toISOString().split('T')[0],
            steps: totalSteps,
            activeMinutes: Math.round(activeMinutes),
            walkCount: walks.length,
            totalWalkMinutes: Math.round(totalWalkMinutes),
            totalWalkDistance: Math.round(totalWalkDistance),
            scratchingEvents: scratchingEvents,
            activityLevelBreakdown: calculateActivityBreakdown(locations)
        });
    } catch (error) {
        console.error('Daily activity error:', error);
        res.status(500).json({ error: 'Failed to fetch daily activity' });
    }
});

// GET /api/v1/activity/:deviceId/weekly - Get weekly activity summary
router.get('/:deviceId/weekly', async (req, res) => {
    try {
        const days = [];
        for (let i = 6; i >= 0; i--) {
            const date = new Date();
            date.setDate(date.getDate() - i);
            date.setHours(0, 0, 0, 0);

            const nextDay = new Date(date);
            nextDay.setDate(nextDay.getDate() + 1);

            // Get walks for this day
            const walks = await Walk.find({
                deviceId: req.params.deviceId,
                startTime: { $gte: date, $lt: nextDay }
            });

            const totalMinutes = walks.reduce((sum, w) => sum + (w.duration || 0) / 60, 0);
            const totalDistance = walks.reduce((sum, w) => sum + (w.distanceMeters || 0), 0);
            const totalSteps = walks.reduce((sum, w) => sum + (w.steps || 0), 0);

            days.push({
                date: date.toISOString().split('T')[0],
                dayOfWeek: date.toLocaleDateString('en-US', { weekday: 'short' }),
                walks: walks.length,
                activeMinutes: Math.round(totalMinutes),
                distance: Math.round(totalDistance),
                steps: totalSteps
            });
        }

        res.json({
            period: 'Last 7 days',
            dailyData: days,
            totals: {
                walks: days.reduce((sum, d) => sum + d.walks, 0),
                activeMinutes: days.reduce((sum, d) => sum + d.activeMinutes, 0),
                distance: days.reduce((sum, d) => sum + d.distance, 0),
                steps: days.reduce((sum, d) => sum + d.steps, 0)
            }
        });
    } catch (error) {
        console.error('Weekly activity error:', error);
        res.status(500).json({ error: 'Failed to fetch weekly activity' });
    }
});

// Helper function to calculate activity level breakdown
function calculateActivityBreakdown(locations) {
    const breakdown = {
        resting: 0,
        light: 0,
        active: 0,
        veryActive: 0
    };

    locations.forEach(loc => {
        switch (loc.activity?.level) {
            case 0: breakdown.resting++; break;
            case 1: breakdown.light++; break;
            case 2: breakdown.active++; break;
            case 3: breakdown.veryActive++; break;
        }
    });

    const total = locations.length || 1;
    return {
        resting: Math.round(breakdown.resting / total * 100),
        light: Math.round(breakdown.light / total * 100),
        active: Math.round(breakdown.active / total * 100),
        veryActive: Math.round(breakdown.veryActive / total * 100)
    };
}

module.exports = router;
