package com.example.smart_car_app

import android.Manifest
import android.app.Activity
import android.content.pm.PackageManager
import android.os.Build
import android.ranging.RangingData
import android.ranging.RangingDevice
import android.ranging.RangingManager
import android.ranging.RangingPreference
import android.ranging.RangingSession
import android.ranging.SessionConfig
import android.ranging.raw.RawInitiatorRangingConfig
import android.ranging.raw.RawRangingDevice
import android.ranging.uwb.UwbAddress
import android.ranging.uwb.UwbComplexChannel
import android.ranging.uwb.UwbRangingParams
import android.util.Log
import androidx.annotation.RequiresApi
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import io.flutter.plugin.common.BinaryMessenger
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import java.util.UUID
import java.util.concurrent.Executors

/**
 * Multicast DS-TWR ranging bridge (Stage 2). Uses the android.ranging framework
 * API (API 36+) so the phone controller address can be pinned to 0x06C1, matching
 * the fixed anchors (addresses 0/1/2). Ported from the reference app Uwb.kt.
 *
 * This is ADDITIVE: the Stage 1 unicast bridge (UwbRangingBridge) is untouched.
 * Emits over EventChannel:
 *   { type:"range",  d0, d1, d2, mask }     // mask bit i => d(i) valid
 *   { type:"status", status, message }      // ACTIVE | STOPPED | ERROR
 */
@Suppress("NewApi") // all framework calls are guarded by isSupported() (SDK_INT >= MIN_API)
class UwbMulticastBridge(
    private val activity: Activity,
    messenger: BinaryMessenger,
) : MethodChannel.MethodCallHandler, EventChannel.StreamHandler {

    companion object {
        private const val TAG = "UwbMulticastBridge"
        private const val CHANNEL = "smartcar/uwb_multi"
        private const val EVENT_CHANNEL = "smartcar/uwb_multi/events"
        private const val REQ_UWB_PERMISSION = 0x5243
        private const val MIN_API = 36 // android.ranging (Android 16 / Baklava)
        private const val ANCHOR_COUNT = 3
    }

    private val methodChannel = MethodChannel(messenger, CHANNEL)
    private val eventChannel = EventChannel(messenger, EVENT_CHANNEL)
    private val executor = Executors.newSingleThreadExecutor()

    private var eventSink: EventChannel.EventSink? = null
    private var pendingPermissionResult: MethodChannel.Result? = null
    private var session: RangingSession? = null
    private var active = false
    private val distances = DoubleArray(ANCHOR_COUNT) { Double.NaN }

    init {
        methodChannel.setMethodCallHandler(this)
        eventChannel.setStreamHandler(this)
    }

    fun dispose() {
        stopInternal("STOPPED")
        methodChannel.setMethodCallHandler(null)
        eventChannel.setStreamHandler(null)
        eventSink = null
        pendingPermissionResult = null
    }

    fun onRequestPermissionsResult(requestCode: Int, grantResults: IntArray): Boolean {
        if (requestCode != REQ_UWB_PERMISSION) return false
        val granted = grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED
        pendingPermissionResult?.success(granted)
        pendingPermissionResult = null
        return true
    }

    override fun onListen(arguments: Any?, events: EventChannel.EventSink?) {
        eventSink = events
    }

    override fun onCancel(arguments: Any?) {
        eventSink = null
    }

    override fun onMethodCall(call: MethodCall, result: MethodChannel.Result) {
        when (call.method) {
            "isSupported" -> result.success(isSupported())
            "ensurePermission" -> ensurePermission(result)
            "isActive" -> result.success(active)
            "start" -> start(call, result)
            "stop" -> {
                stopInternal("STOPPED")
                result.success(true)
            }
            else -> result.notImplemented()
        }
    }

    private fun isSupported(): Boolean {
        if (Build.VERSION.SDK_INT < MIN_API) return false
        return activity.packageManager.hasSystemFeature(PackageManager.FEATURE_UWB)
    }

    private fun hasPermission(): Boolean {
        return ContextCompat.checkSelfPermission(
            activity, Manifest.permission.RANGING,
        ) == PackageManager.PERMISSION_GRANTED
    }

    private fun ensurePermission(result: MethodChannel.Result) {
        if (!isSupported()) { result.success(false); return }
        if (hasPermission()) { result.success(true); return }
        if (pendingPermissionResult != null) {
            result.error("UWB_PERMISSION_PENDING", "Another request in progress", null)
            return
        }
        pendingPermissionResult = result
        ActivityCompat.requestPermissions(
            activity, arrayOf(Manifest.permission.RANGING), REQ_UWB_PERMISSION,
        )
    }

    @RequiresApi(MIN_API)
    private fun start(call: MethodCall, result: MethodChannel.Result) {
        if (!isSupported()) {
            result.error("UWB_NOT_SUPPORTED", "android.ranging requires API $MIN_API", null); return
        }
        if (!hasPermission()) {
            result.error("UWB_PERMISSION_DENIED", "RANGING not granted", null); return
        }
        if (active) { result.success(true); return }

        val sessionId = ((call.argument<Any>("sessionId") as? Number) ?: 42).toInt()
        val channel = ((call.argument<Any>("channel") as? Number) ?: 9).toInt()
        val preamble = ((call.argument<Any>("preambleIndex") as? Number) ?: 9).toInt()
        val localAddress = ((call.argument<Any>("localAddress") as? Number) ?: 0x06C1).toInt()
        val anchorAddrs = (call.argument<List<Int>>("anchorAddresses") ?: listOf(0, 1, 2))
        val keyInfo = toByteArray(call.argument<Any>("sessionKeyInfo"))
            ?: byteArrayOf(0x08, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06)

        try {
            val rangingManager = activity.getSystemService(RangingManager::class.java)
                ?: throw IllegalStateException("RangingManager unavailable")

            for (i in distances.indices) distances[i] = Double.NaN

            val devices = anchorAddrs.take(ANCHOR_COUNT).mapIndexed { idx, addr ->
                RawRangingDevice.Builder()
                    .setRangingDevice(
                        RangingDevice.Builder().setUuid(UUID(0, idx.toLong())).build(),
                    )
                    .setUwbRangingParams(
                        UwbRangingParams.Builder(
                            sessionId,
                            UwbRangingParams.CONFIG_MULTICAST_DS_TWR,
                            UwbAddress.fromBytes(
                                byteArrayOf(
                                    (localAddress and 0xFF).toByte(),
                                    ((localAddress shr 8) and 0xFF).toByte(),
                                ),
                            ),
                            UwbAddress.fromBytes(byteArrayOf((addr and 0xFF).toByte(), 0)),
                        )
                            .setComplexChannel(
                                UwbComplexChannel.Builder()
                                    .setChannel(channel)
                                    .setPreambleIndex(preamble)
                                    .build(),
                            )
                            .setRangingUpdateRate(RawRangingDevice.UPDATE_RATE_FREQUENT)
                            .setSessionKeyInfo(keyInfo)
                            .build(),
                    )
                    .build()
            }

            val config = RawInitiatorRangingConfig.Builder()
                .addRawRangingDevices(devices)
                .build()

            val preference = RangingPreference.Builder(
                RangingPreference.DEVICE_ROLE_INITIATOR, config,
            ).setSessionConfig(SessionConfig.Builder().build()).build()

            val callback = object : RangingSession.Callback {
                override fun onOpened() { emitStatus("ACTIVE", "session opened") }
                override fun onOpenFailed(reason: Int) {
                    active = false; emitStatus("ERROR", "open failed: $reason")
                }
                override fun onStarted(peer: RangingDevice, technology: Int) {
                    active = true; emitStatus("ACTIVE", "started")
                }
                override fun onStopped(peer: RangingDevice, technology: Int) {}
                override fun onClosed(reason: Int) {
                    active = false; emitStatus("STOPPED", "closed: $reason")
                }
                override fun onResults(peer: RangingDevice, data: RangingData) {
                    val idx = peer.uuid.leastSignificantBits.toInt()
                    if (idx !in 0 until ANCHOR_COUNT) return
                    val d = data.distance?.measurement ?: return
                    distances[idx] = d
                    emitRange()
                }
            }

            val s = rangingManager.createRangingSession(executor, callback)
                ?: throw IllegalStateException("createRangingSession returned null")
            session = s
            s.start(preference)
            active = true
            emitStatus("ACTIVE", "starting")
            result.success(true)
        } catch (e: Exception) {
            Log.e(TAG, "start failed", e)
            active = false
            emitStatus("ERROR", e.message ?: "start failed")
            result.error("UWB_START_FAILED", e.message, null)
        }
    }

    private fun stopInternal(status: String) {
        try {
            session?.stop()
        } catch (e: Exception) {
            Log.w(TAG, "stop error", e)
        }
        session = null
        active = false
        for (i in distances.indices) distances[i] = Double.NaN
        emitStatus(status, "stopped")
    }

    private fun emitRange() {
        var mask = 0
        val out = DoubleArray(ANCHOR_COUNT)
        for (i in 0 until ANCHOR_COUNT) {
            val d = distances[i]
            if (!d.isNaN()) { mask = mask or (1 shl i); out[i] = d } else out[i] = 0.0
        }
        val payload = mapOf(
            "type" to "range",
            "d0" to out[0], "d1" to out[1], "d2" to out[2],
            "mask" to mask,
        )
        // RangingSession callbacks fire on the single-thread executor, but the
        // Flutter EventChannel sink must be called on the main thread.
        activity.runOnUiThread { eventSink?.success(payload) }
    }

    private fun emitStatus(status: String, message: String) {
        val payload = mapOf("type" to "status", "status" to status, "message" to message)
        activity.runOnUiThread { eventSink?.success(payload) }
    }

    private fun toByteArray(values: Any?): ByteArray? {
        return when (values) {
            is ByteArray -> if (values.isEmpty()) null else values
            is List<*> -> if (values.isEmpty()) null else ByteArray(values.size) {
                ((values[it] as? Number)?.toInt() ?: 0).toByte()
            }
            else -> null
        }
    }
}
