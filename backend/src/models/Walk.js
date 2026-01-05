/**
 * Walk Model - Stores walk session data with grading
 */

const mongoose = require('mongoose');

const walkSchema = new mongoose.Schema({
    deviceId: {
        type: String,
        required: true,
        index: true
    },
    dogName: String,
    walkId: {
        type: Number,
        required: true
    },

    // Timing
    startTime: {
        type: Date,
        required: true,
        index: true
    },
    endTime: Date,
    duration: Number,  // seconds

    // Distance and speed
    distanceMeters: Number,
    avgSpeed: Number,    // km/h
    maxSpeed: Number,    // km/h
    steps: Number,

    // Activity breakdown
    activeSeconds: Number,
    stationarySeconds: Number,
    activePercent: Number,

    // Walk grade
    grade: {
        type: String,
        enum: ['A', 'B', 'C', 'F'],
        index: true
    },

    // Cheating detection
    wasCarried: {
        type: Boolean,
        default: false
    },
    wasInVehicle: {
        type: Boolean,
        default: false
    },
    pauseCount: {
        type: Number,
        default: 0
    },

    // Route
    startLocation: {
        type: {
            type: String,
            enum: ['Point'],
            default: 'Point'
        },
        coordinates: [Number]  // [longitude, latitude]
    },
    endLocation: {
        type: {
            type: String,
            enum: ['Point'],
            default: 'Point'
        },
        coordinates: [Number]
    },

    // Route polyline (encoded for efficiency)
    routePolyline: String,

    // Walker info (if assigned)
    walker: {
        name: String,
        phone: String
    },

    // Status
    status: {
        type: String,
        enum: ['in_progress', 'completed', 'cancelled'],
        default: 'in_progress'
    }
}, {
    timestamps: true
});

// Indexes
walkSchema.index({ deviceId: 1, startTime: -1 });
walkSchema.index({ startLocation: '2dsphere' });

// Virtual for formatted duration
walkSchema.virtual('formattedDuration').get(function() {
    if (!this.duration) return '0 min';
    const minutes = Math.floor(this.duration / 60);
    const seconds = this.duration % 60;
    return `${minutes}m ${seconds}s`;
});

// Static methods
walkSchema.statics.getRecent = function(deviceId, limit = 10) {
    return this.find({ deviceId, status: 'completed' })
        .sort({ startTime: -1 })
        .limit(limit);
};

walkSchema.statics.getToday = function(deviceId) {
    const startOfDay = new Date();
    startOfDay.setHours(0, 0, 0, 0);

    return this.find({
        deviceId,
        startTime: { $gte: startOfDay }
    }).sort({ startTime: -1 });
};

walkSchema.statics.getStats = async function(deviceId, days = 7) {
    const since = new Date(Date.now() - days * 24 * 60 * 60 * 1000);

    const stats = await this.aggregate([
        {
            $match: {
                deviceId,
                startTime: { $gte: since },
                status: 'completed'
            }
        },
        {
            $group: {
                _id: null,
                totalWalks: { $sum: 1 },
                totalDistance: { $sum: '$distanceMeters' },
                totalDuration: { $sum: '$duration' },
                totalSteps: { $sum: '$steps' },
                avgActivePercent: { $avg: '$activePercent' },
                gradeA: { $sum: { $cond: [{ $eq: ['$grade', 'A'] }, 1, 0] } },
                gradeB: { $sum: { $cond: [{ $eq: ['$grade', 'B'] }, 1, 0] } },
                gradeC: { $sum: { $cond: [{ $eq: ['$grade', 'C'] }, 1, 0] } },
                gradeF: { $sum: { $cond: [{ $eq: ['$grade', 'F'] }, 1, 0] } },
                carriedCount: { $sum: { $cond: ['$wasCarried', 1, 0] } },
                vehicleCount: { $sum: { $cond: ['$wasInVehicle', 1, 0] } }
            }
        }
    ]);

    return stats[0] || {
        totalWalks: 0,
        totalDistance: 0,
        totalDuration: 0,
        totalSteps: 0,
        avgActivePercent: 0,
        gradeA: 0,
        gradeB: 0,
        gradeC: 0,
        gradeF: 0,
        carriedCount: 0,
        vehicleCount: 0
    };
};

// Calculate walker report card
walkSchema.statics.getWalkerReportCard = async function(deviceId, walkerName) {
    const walks = await this.find({
        deviceId,
        'walker.name': walkerName,
        status: 'completed'
    }).sort({ startTime: -1 }).limit(20);

    if (walks.length === 0) {
        return { message: 'No walks found for this walker' };
    }

    const gradeValues = { 'A': 4, 'B': 3, 'C': 2, 'F': 0 };
    const gradeSum = walks.reduce((sum, walk) => sum + (gradeValues[walk.grade] || 0), 0);
    const avgGradeValue = gradeSum / walks.length;

    let avgGrade;
    if (avgGradeValue >= 3.5) avgGrade = 'A';
    else if (avgGradeValue >= 2.5) avgGrade = 'B';
    else if (avgGradeValue >= 1.5) avgGrade = 'C';
    else avgGrade = 'F';

    return {
        walkerName,
        totalWalks: walks.length,
        averageGrade: avgGrade,
        averageActivePercent: walks.reduce((sum, w) => sum + (w.activePercent || 0), 0) / walks.length,
        averageDuration: walks.reduce((sum, w) => sum + (w.duration || 0), 0) / walks.length,
        averageDistance: walks.reduce((sum, w) => sum + (w.distanceMeters || 0), 0) / walks.length,
        cheatingIncidents: walks.filter(w => w.wasCarried || w.wasInVehicle).length,
        recentWalks: walks.slice(0, 5).map(w => ({
            date: w.startTime,
            grade: w.grade,
            duration: w.duration,
            distance: w.distanceMeters
        }))
    };
};

module.exports = mongoose.model('Walk', walkSchema);
