/**
 * Location Model - Stores GPS tracking data
 */

const mongoose = require('mongoose');

const locationSchema = new mongoose.Schema({
    deviceId: {
        type: String,
        required: true,
        index: true
    },
    dogName: String,

    // GPS data
    location: {
        type: {
            type: String,
            enum: ['Point'],
            default: 'Point'
        },
        coordinates: {
            type: [Number],  // [longitude, latitude]
            required: true
        }
    },
    altitude: Number,
    speed: Number,       // km/h
    course: Number,      // degrees
    satellites: Number,
    hdop: Number,

    // Activity data (captured at same time)
    activity: {
        steps: Number,
        activeMinutes: Number,
        level: {
            type: Number,
            enum: [0, 1, 2, 3]  // Resting, Light, Active, Very Active
        },
        isMoving: Boolean,
        isSleeping: Boolean,
        isScratching: Boolean
    },

    // Battery status
    batteryVoltage: Number,
    batteryPercent: Number,

    // Walk association (if during a walk)
    walkId: {
        type: mongoose.Schema.Types.ObjectId,
        ref: 'Walk',
        index: true
    },

    // Metadata
    timestamp: {
        type: Date,
        default: Date.now,
        index: true
    }
}, {
    timestamps: true
});

// Geospatial index
locationSchema.index({ location: '2dsphere' });

// Compound index for efficient queries
locationSchema.index({ deviceId: 1, timestamp: -1 });

// TTL index - auto-delete location data older than 30 days
locationSchema.index({ timestamp: 1 }, { expireAfterSeconds: 2592000 });

// Static methods
locationSchema.statics.getLatest = function(deviceId) {
    return this.findOne({ deviceId })
        .sort({ timestamp: -1 })
        .limit(1);
};

locationSchema.statics.getHistory = function(deviceId, hours = 24) {
    const since = new Date(Date.now() - hours * 60 * 60 * 1000);
    return this.find({
        deviceId,
        timestamp: { $gte: since }
    }).sort({ timestamp: 1 });
};

locationSchema.statics.getWalkRoute = function(walkId) {
    return this.find({ walkId })
        .sort({ timestamp: 1 })
        .select('location timestamp speed');
};

module.exports = mongoose.model('Location', locationSchema);
