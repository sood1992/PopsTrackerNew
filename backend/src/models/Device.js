/**
 * Device Model - Stores device information and configuration
 */

const mongoose = require('mongoose');

const deviceSchema = new mongoose.Schema({
    deviceId: {
        type: String,
        required: true,
        unique: true,
        index: true
    },
    dogName: {
        type: String,
        default: 'Unknown'
    },
    ownerName: String,
    ownerEmail: String,
    ownerPhone: String,

    // Home location for geofencing
    homeLocation: {
        type: {
            type: String,
            enum: ['Point'],
            default: 'Point'
        },
        coordinates: {
            type: [Number],  // [longitude, latitude]
            default: [0, 0]
        }
    },
    homeRadius: {
        type: Number,
        default: 50  // meters
    },

    // Device status
    lastSeen: Date,
    batteryLevel: Number,
    firmwareVersion: String,
    isOnline: {
        type: Boolean,
        default: false
    },

    // Current location (cached for quick access)
    currentLocation: {
        lat: Number,
        lon: Number,
        speed: Number,
        timestamp: Date
    },

    // Walker information
    walkers: [{
        name: String,
        phone: String,
        email: String,
        averageGrade: String,
        totalWalks: { type: Number, default: 0 }
    }],

    // Notification settings
    notifications: {
        pushEnabled: { type: Boolean, default: true },
        emailEnabled: { type: Boolean, default: false },
        smsEnabled: { type: Boolean, default: false },
        lowBatteryAlert: { type: Boolean, default: true },
        walkCompleteAlert: { type: Boolean, default: true },
        badWalkAlert: { type: Boolean, default: true },
        geofenceAlert: { type: Boolean, default: true }
    },

    // Stats
    stats: {
        totalWalks: { type: Number, default: 0 },
        totalDistance: { type: Number, default: 0 },
        totalActiveMinutes: { type: Number, default: 0 },
        averageWalkGrade: String
    }
}, {
    timestamps: true
});

// Geospatial index for location queries
deviceSchema.index({ homeLocation: '2dsphere' });

// Methods
deviceSchema.methods.updateLocation = function(lat, lon, speed) {
    this.currentLocation = {
        lat,
        lon,
        speed,
        timestamp: new Date()
    };
    this.lastSeen = new Date();
    this.isOnline = true;
    return this.save();
};

deviceSchema.methods.setOffline = function() {
    this.isOnline = false;
    return this.save();
};

module.exports = mongoose.model('Device', deviceSchema);
