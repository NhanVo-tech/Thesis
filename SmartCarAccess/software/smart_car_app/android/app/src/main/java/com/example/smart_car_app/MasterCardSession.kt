package com.example.smart_car_app

import android.util.Log

/**
 * In-memory holder for master card provisioning session.
 * This is intentionally not persisted to disk.
 */
object MasterCardSession {
    private const val TAG = "MasterCardSession"

    @Volatile private var masterSecret: ByteArray? = null
    @Volatile private var vehicleId: ByteArray? = null
    @Volatile private var expiresAtMs: Long = 0

    fun setSession(secret: ByteArray, vid: ByteArray, ttlMs: Long) {
        masterSecret = secret.copyOf()
        vehicleId = vid.copyOf()
        expiresAtMs = System.currentTimeMillis() + ttlMs
        Log.i(TAG, "Master session set (ttlMs=$ttlMs)")
    }

    fun clearSession() {
        masterSecret = null
        vehicleId = null
        expiresAtMs = 0
        Log.i(TAG, "Master session cleared")
    }

    fun isActive(): Boolean {
        val now = System.currentTimeMillis()
        val active = masterSecret != null && vehicleId != null && expiresAtMs > now
        if (!active && (masterSecret != null || vehicleId != null)) {
            clearSession()
        }
        return active
    }

    fun getMasterSecret(): ByteArray? {
        return if (isActive()) masterSecret else null
    }

    fun getVehicleId(): ByteArray? {
        return if (isActive()) vehicleId else null
    }
}
