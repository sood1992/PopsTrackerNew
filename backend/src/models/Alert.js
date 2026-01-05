/**
 * Alert Model - Stores alerts and notifications
 */

const mongoose = require('mongoose');

const alertSchema = new mongoose.Schema({
    deviceId: {
        type: String,
        required: true,
        index: true
    },
    dogName: String,

    // Alert details
    alertType: {
        type: String,
        required: true,
        enum: [
            'LOW_BATTERY',
            'BAD_WALK',
            'SCRATCHING',
            'GEOFENCE_EXIT',
            'GEOFENCE_ENTER',
            'DEVICE_OFFLINE',
            'CARRIED',
            'VEHICLE',
            'SOS',
            'CUSTOM'
        ]
    },
    severity: {
        type: String,
        enum: ['info', 'warning', 'critical'],
        default: 'warning'
    },
    message: {
        type: String,
        required: true
    },

    // Associated data
    location: {
        type: {
            type: String,
            enum: ['Point'],
            default: 'Point'
        },
        coordinates: [Number]
    },
    walkId: {
        type: mongoose.Schema.Types.ObjectId,
        ref: 'Walk'
    },

    // Notification status
    notified: {
        type: Boolean,
        default: false
    },
    notifiedAt: Date,
    notificationChannels: [String],  // 'push', 'email', 'sms'

    // Acknowledgment
    acknowledged: {
        type: Boolean,
        default: false
    },
    acknowledgedAt: Date,
    acknowledgedBy: String,

    timestamp: {
        type: Date,
        default: Date.now,
        index: true
    }
}, {
    timestamps: true
});

// Index for efficient queries
alertSchema.index({ deviceId: 1, timestamp: -1 });
alertSchema.index({ acknowledged: 1, timestamp: -1 });

// TTL - auto-delete old acknowledged alerts after 90 days
alertSchema.index(
    { acknowledgedAt: 1 },
    { expireAfterSeconds: 7776000, partialFilterExpression: { acknowledged: true } }
);

// Static methods
alertSchema.statics.getUnacknowledged = function(deviceId) {
    return this.find({
        deviceId,
        acknowledged: false
    }).sort({ timestamp: -1 });
};

alertSchema.statics.getRecent = function(deviceId, limit = 50) {
    return this.find({ deviceId })
        .sort({ timestamp: -1 })
        .limit(limit);
};

module.exports = mongoose.model('Alert', alertSchema);
