/**
 * Walk Routes - Handle walk session data
 */

const express = require('express');
const router = express.Router();
const Walk = require('../models/Walk');
const Location = require('../models/Location');
const Alert = require('../models/Alert');

// POST /api/v1/walk - Save completed walk
router.post('/', async (req, res) => {
    try {
        const {
            device_id,
            dog_name,
            walk_id,
            start_time,
            end_time,
            duration_seconds,
            distance_meters,
            avg_speed_kmh,
            max_speed_kmh,
            steps,
            active_seconds,
            stationary_seconds,
            active_percent,
            grade,
            was_carried,
            was_in_vehicle,
            pause_count,
            start_location,
            end_location
        } = req.body;

        // Create walk record
        const walk = new Walk({
            deviceId: device_id,
            dogName: dog_name,
            walkId: walk_id,
            startTime: new Date(start_time),
            endTime: new Date(end_time),
            duration: duration_seconds,
            distanceMeters: distance_meters,
            avgSpeed: avg_speed_kmh,
            maxSpeed: max_speed_kmh,
            steps: steps,
            activeSeconds: active_seconds,
            stationarySeconds: stationary_seconds,
            activePercent: active_percent,
            grade: grade,
            wasCarried: was_carried,
            wasInVehicle: was_in_vehicle,
            pauseCount: pause_count,
            startLocation: {
                type: 'Point',
                coordinates: [start_location.lon, start_location.lat]
            },
            endLocation: {
                type: 'Point',
                coordinates: [end_location.lon, end_location.lat]
            },
            status: 'completed'
        });

        await walk.save();

        // Emit walk completed event
        const io = req.app.get('io');
        io.to(device_id).emit('walkCompleted', {
            walkId: walk._id,
            grade: grade,
            duration: duration_seconds,
            distance: distance_meters,
            activePercent: active_percent,
            wasCarried: was_carried,
            wasInVehicle: was_in_vehicle
        });

        // Create alert if bad walk
        if (grade === 'F' || was_carried || was_in_vehicle) {
            const alert = new Alert({
                deviceId: device_id,
                dogName: dog_name,
                alertType: 'BAD_WALK',
                severity: 'warning',
                message: `Walk graded ${grade}. ${was_carried ? 'Dog was carried! ' : ''}${was_in_vehicle ? 'Dog was in vehicle!' : ''}`,
                walkId: walk._id,
                endLocation: {
                    type: 'Point',
                    coordinates: [end_location.lon, end_location.lat]
                }
            });
            await alert.save();

            io.to(device_id).emit('alert', {
                type: 'BAD_WALK',
                message: alert.message,
                grade: grade
            });
        }

        res.status(201).json({
            success: true,
            walkId: walk._id,
            message: `Walk saved with grade ${grade}`
        });
    } catch (error) {
        console.error('Walk save error:', error);
        res.status(500).json({ error: 'Failed to save walk' });
    }
});

// GET /api/v1/walk/:deviceId - Get recent walks
router.get('/:deviceId', async (req, res) => {
    try {
        const limit = parseInt(req.query.limit) || 10;
        const walks = await Walk.getRecent(req.params.deviceId, limit);
        res.json(walks);
    } catch (error) {
        console.error('Walk fetch error:', error);
        res.status(500).json({ error: 'Failed to fetch walks' });
    }
});

// GET /api/v1/walk/:deviceId/today - Get today's walks
router.get('/:deviceId/today', async (req, res) => {
    try {
        const walks = await Walk.getToday(req.params.deviceId);
        res.json(walks);
    } catch (error) {
        console.error('Walk fetch error:', error);
        res.status(500).json({ error: 'Failed to fetch today\'s walks' });
    }
});

// GET /api/v1/walk/:deviceId/stats - Get walk statistics
router.get('/:deviceId/stats', async (req, res) => {
    try {
        const days = parseInt(req.query.days) || 7;
        const stats = await Walk.getStats(req.params.deviceId, days);
        res.json(stats);
    } catch (error) {
        console.error('Walk stats error:', error);
        res.status(500).json({ error: 'Failed to fetch walk stats' });
    }
});

// GET /api/v1/walk/:walkId/route - Get walk route
router.get('/:walkId/route', async (req, res) => {
    try {
        const locations = await Location.getWalkRoute(req.params.walkId);
        res.json(locations);
    } catch (error) {
        console.error('Walk route error:', error);
        res.status(500).json({ error: 'Failed to fetch walk route' });
    }
});

// GET /api/v1/walk/:deviceId/walker-report - Get walker report card
router.get('/:deviceId/walker-report', async (req, res) => {
    try {
        const walkerName = req.query.walker;
        if (!walkerName) {
            return res.status(400).json({ error: 'Walker name required' });
        }

        const report = await Walk.getWalkerReportCard(req.params.deviceId, walkerName);
        res.json(report);
    } catch (error) {
        console.error('Walker report error:', error);
        res.status(500).json({ error: 'Failed to generate walker report' });
    }
});

// GET /api/v1/walk/:deviceId/weekly-summary - Get weekly summary
router.get('/:deviceId/weekly-summary', async (req, res) => {
    try {
        const stats = await Walk.getStats(req.params.deviceId, 7);

        // Calculate averages
        const avgWalkDuration = stats.totalWalks > 0
            ? Math.round(stats.totalDuration / stats.totalWalks / 60)
            : 0;

        const avgWalkDistance = stats.totalWalks > 0
            ? Math.round(stats.totalDistance / stats.totalWalks)
            : 0;

        // Determine overall grade
        let overallGrade;
        const totalGraded = stats.gradeA + stats.gradeB + stats.gradeC + stats.gradeF;
        if (totalGraded === 0) {
            overallGrade = 'N/A';
        } else {
            const gradeScore = (stats.gradeA * 4 + stats.gradeB * 3 + stats.gradeC * 2) / totalGraded;
            if (gradeScore >= 3.5) overallGrade = 'A';
            else if (gradeScore >= 2.5) overallGrade = 'B';
            else if (gradeScore >= 1.5) overallGrade = 'C';
            else overallGrade = 'F';
        }

        res.json({
            period: '7 days',
            totalWalks: stats.totalWalks,
            totalDistance: Math.round(stats.totalDistance),
            totalDuration: Math.round(stats.totalDuration / 60),
            totalSteps: stats.totalSteps,
            avgWalkDuration,
            avgWalkDistance,
            avgActivePercent: Math.round(stats.avgActivePercent),
            overallGrade,
            gradeBreakdown: {
                A: stats.gradeA,
                B: stats.gradeB,
                C: stats.gradeC,
                F: stats.gradeF
            },
            cheatingIncidents: stats.carriedCount + stats.vehicleCount,
            carriedCount: stats.carriedCount,
            vehicleCount: stats.vehicleCount
        });
    } catch (error) {
        console.error('Weekly summary error:', error);
        res.status(500).json({ error: 'Failed to generate weekly summary' });
    }
});

module.exports = router;
