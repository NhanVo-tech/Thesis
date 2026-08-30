package com.example.smart_car_app

import android.Manifest
import android.app.Activity
import android.content.pm.PackageManager
import android.os.Build
import android.util.Log
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.core.uwb.RangingParameters
import androidx.core.uwb.RangingResult
import androidx.core.uwb.UwbAddress
import androidx.core.uwb.UwbComplexChannel
import androidx.core.uwb.UwbDevice
import androidx.core.uwb.UwbManager
import androidx.core.uwb.UwbClientSessionScope
import io.flutter.plugin.common.BinaryMessenger
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.launch

class UwbRangingBridge(
    private val activity: Activity,
    messenger: BinaryMessenger,
) : MethodChannel.MethodCallHandler, EventChannel.StreamHandler {

    companion object {
        private const val TAG = "UwbRangingBridge"
        private const val CHANNEL = "smartcar/uwb"
        private const val EVENT_CHANNEL = "smartcar/uwb/events"
        private const val REQ_UWB_PERMISSION = 0x5242
    }

    private val methodChannel = MethodChannel(messenger, CHANNEL)
    private val eventChannel = EventChannel(messenger, EVENT_CHANNEL)
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)

    private var eventSink: EventChannel.EventSink? = null
    private var pendingPermissionResult: MethodChannel.Result? = null

    private var uwbManager: UwbManager? = null
    private var sessionScope: UwbClientSessionScope? = null
    private var rangingJob: Job? = null
    private var rangingStartedAtMs: Long = 0L
    private var positionSeenInCurrentRun: Boolean = false
    private var latestRangingEvent: Map<String, Any?>? = null

    init {
        methodChannel.setMethodCallHandler(this)
        eventChannel.setStreamHandler(this)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            try {
                uwbManager = UwbManager.createInstance(activity)
            } catch (e: Exception) {
                Log.w(TAG, "Failed to create UwbManager", e)
            }
        }
    }

    fun dispose() {
        stopRangingInternal()
        scope.cancel()
        methodChannel.setMethodCallHandler(null)
        eventChannel.setStreamHandler(null)
        eventSink = null
        pendingPermissionResult = null
    }

    fun onRequestPermissionsResult(requestCode: Int, grantResults: IntArray): Boolean {
        if (requestCode != REQ_UWB_PERMISSION) {
            return false
        }
        val granted = grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED
        pendingPermissionResult?.success(granted)
        pendingPermissionResult = null
        return true
    }

    override fun onMethodCall(call: MethodCall, result: MethodChannel.Result) {
        when (call.method) {
            "isSupported" -> result.success(isUwbSupported())
            "ensurePermission" -> ensurePermission(result)
            "prepareSession" -> prepareSession(call, result)
            "startRanging" -> startRanging(call, result)
            "stopRanging" -> {
                stopRangingInternal()
                result.success(true)
            }
            "isRangingActive" -> result.success(rangingJob?.isActive == true)
            "getLatestRanging" -> {
                result.success(latestRangingEvent)
            }
            else -> result.notImplemented()
        }
    }

    override fun onListen(arguments: Any?, events: EventChannel.EventSink?) {
        eventSink = events
    }

    override fun onCancel(arguments: Any?) {
        eventSink = null
    }

    private fun isUwbSupported(): Boolean {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
            return false
        }
        val pm = activity.packageManager
        val hasFeature = pm.hasSystemFeature(PackageManager.FEATURE_UWB)
        return hasFeature && uwbManager != null
    }

    private fun hasUwbPermission(): Boolean {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
            return false
        }
        return ContextCompat.checkSelfPermission(
            activity,
            Manifest.permission.UWB_RANGING,
        ) == PackageManager.PERMISSION_GRANTED
    }

    private fun ensurePermission(result: MethodChannel.Result) {
        if (!isUwbSupported()) {
            result.success(false)
            return
        }
        if (hasUwbPermission()) {
            result.success(true)
            return
        }
        if (pendingPermissionResult != null) {
            result.error("UWB_PERMISSION_PENDING", "Another permission request is in progress", null)
            return
        }
        pendingPermissionResult = result
        ActivityCompat.requestPermissions(
            activity,
            arrayOf(Manifest.permission.UWB_RANGING),
            REQ_UWB_PERMISSION,
        )
    }

    private fun prepareSession(call: MethodCall, result: MethodChannel.Result) {
        if (!isUwbSupported()) {
            result.error("UWB_NOT_SUPPORTED", "UWB is not supported on this device", null)
            return
        }
        if (!hasUwbPermission()) {
            result.error("UWB_PERMISSION_DENIED", "UWB_RANGING permission is not granted", null)
            return
        }

        val controller = call.argument<Boolean>("controller") ?: true

        stopRangingInternal()
        scope.launch {
            try {
                sessionScope = if (controller) {
                    uwbManager?.controllerSessionScope()
                } else {
                    uwbManager?.controleeSessionScope()
                }

                val localAddress = sessionScope?.localAddress?.address
                    ?: throw IllegalStateException("Unable to obtain UWB local address")

                result.success(
                    mapOf(
                        "localAddress" to formatAddress(localAddress),
                        "rawAddress" to localAddress.toList(),
                    ),
                )
            } catch (e: Exception) {
                Log.e(TAG, "prepareSession failed", e)
                result.error("UWB_PREPARE_FAILED", e.message, null)
            }
        }
    }

    private fun startRanging(call: MethodCall, result: MethodChannel.Result) {
        val activeSession = sessionScope
        if (activeSession == null) {
            result.error("UWB_SESSION_NOT_READY", "Call prepareSession first", null)
            return
        }

        val remoteAddress = call.argument<String>("remoteAddress") ?: ""
        val sessionId = ((call.argument<Any>("sessionId") as? Number) ?: 42).toInt()
        val subSessionId = ((call.argument<Any>("subSessionId") as? Number) ?: 0).toInt()
        val channel = ((call.argument<Any>("channel") as? Number) ?: 9).toInt()
        val preambleIndex = ((call.argument<Any>("preambleIndex") as? Number) ?: 9).toInt()
        val updateRate = call.argument<String>("updateRate") ?: "frequent"
        val sessionKeyInfo = toByteArray(call.argument<Any>("sessionKeyInfo"))
            ?: byteArrayOf(0x08, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06)

        val remote = parseAddress(remoteAddress)
        if (remote == null) {
            result.error("UWB_INVALID_ADDRESS", "Address must be in format XX:XX", null)
            return
        }

        val rateType = if (updateRate.equals("infrequent", ignoreCase = true)) {
            RangingParameters.RANGING_UPDATE_RATE_INFREQUENT
        } else {
            RangingParameters.RANGING_UPDATE_RATE_FREQUENT
        }

        val params = RangingParameters(
            uwbConfigType = RangingParameters.CONFIG_UNICAST_DS_TWR,
            sessionId = sessionId,
            subSessionId = subSessionId,
            sessionKeyInfo = sessionKeyInfo,
            subSessionKeyInfo = null,
            complexChannel = UwbComplexChannel(channel, preambleIndex),
            peerDevices = listOf(UwbDevice(UwbAddress(remote))),
            updateRateType = rateType,
        )

        val previousJob = rangingJob
        rangingJob = scope.launch {
            try {
                previousJob?.cancelAndJoin()
                rangingStartedAtMs = System.currentTimeMillis()
                positionSeenInCurrentRun = false

                activeSession.prepareSession(params).collect { rangingResult ->
                    when (rangingResult) {
                        is RangingResult.RangingResultPosition -> {
                            positionSeenInCurrentRun = true
                            val position = rangingResult.position
                            val event = mapOf(
                                "type" to "position",
                                "distanceM" to position.distance?.value,
                                "azimuthDeg" to position.azimuth?.value,
                                "elevationDeg" to position.elevation?.value,
                                "elapsedRealtimeNanos" to position.elapsedRealtimeNanos,
                            )
                            latestRangingEvent = event
                            eventSink?.success(event)
                        }

                        is RangingResult.RangingResultPeerDisconnected -> {
                            val elapsedMs = System.currentTimeMillis() - rangingStartedAtMs
                            val event = mapOf(
                                "type" to "peer_disconnected",
                                "elapsedMs" to elapsedMs,
                                "positionSeen" to positionSeenInCurrentRun,
                            )
                            latestRangingEvent = event
                            eventSink?.success(event)
                        }
                    }
                }
            } catch (e: CancellationException) {
                // Expected when stopping ranging after a peer_disconnected event.
                Log.d(TAG, "Ranging coroutine cancelled: ${e.message}")
            } catch (e: Exception) {
                if (isAlreadyStartedError(e.message)) {
                    Log.w(TAG, "Ranging already started — recreating scope and retrying")
                    sessionScope = null
                    val freshScope = uwbManager?.controllerSessionScope()
                    if (freshScope == null) {
                        Log.e(TAG, "Failed to recreate UWB session scope")
                        eventSink?.error("UWB_RANGING_FAILED", "Cannot create fresh session scope", null)
                        return@launch
                    }
                    sessionScope = freshScope
                    try {
                        rangingStartedAtMs = System.currentTimeMillis()
                        positionSeenInCurrentRun = false
                        freshScope.prepareSession(params).collect { rangingResult ->
                            when (rangingResult) {
                                is RangingResult.RangingResultPosition -> {
                                    positionSeenInCurrentRun = true
                                    val position = rangingResult.position
                                    val event = mapOf(
                                        "type" to "position",
                                        "distanceM" to position.distance?.value,
                                        "azimuthDeg" to position.azimuth?.value,
                                        "elevationDeg" to position.elevation?.value,
                                        "elapsedRealtimeNanos" to position.elapsedRealtimeNanos,
                                    )
                                    latestRangingEvent = event
                                    eventSink?.success(event)
                                }
                                is RangingResult.RangingResultPeerDisconnected -> {
                                    val elapsedMs = System.currentTimeMillis() - rangingStartedAtMs
                                    val event = mapOf(
                                        "type" to "peer_disconnected",
                                        "elapsedMs" to elapsedMs,
                                        "positionSeen" to positionSeenInCurrentRun,
                                    )
                                    latestRangingEvent = event
                                    eventSink?.success(event)
                                }
                            }
                        }
                    } catch (e2: CancellationException) {
                        Log.d(TAG, "Ranging coroutine cancelled: ${e2.message}")
                    } catch (e2: Exception) {
                        Log.e(TAG, "Ranging retry failed", e2)
                        eventSink?.error("UWB_RANGING_FAILED", e2.message, null)
                    }
                    return@launch
                }
                Log.e(TAG, "Ranging failed", e)
                eventSink?.error("UWB_RANGING_FAILED", e.message, null)
                stopRangingInternal(keepSession = true)
            }
        }

        result.success(true)
    }

    private fun stopRangingInternal(keepSession: Boolean = false) {
        rangingJob?.cancel()
        rangingJob = null
        rangingStartedAtMs = 0L
        positionSeenInCurrentRun = false
        if (!keepSession) {
            sessionScope = null
            latestRangingEvent = null
        }
    }

    private fun isAlreadyStartedError(message: String?): Boolean {
        if (message.isNullOrBlank()) {
            return false
        }
        return message.contains("already started", ignoreCase = true) &&
            message.contains("clientSessionScope", ignoreCase = true)
    }

    private fun parseAddress(value: String): ByteArray? {
        val parts = value.trim().uppercase().split(":")
        if (parts.size != 2) {
            return null
        }
        return try {
            byteArrayOf(parts[0].toInt(16).toByte(), parts[1].toInt(16).toByte())
        } catch (_: Exception) {
            null
        }
    }

    private fun formatAddress(address: ByteArray): String {
        return address.joinToString(":") { byte -> "%02X".format(byte.toInt() and 0xFF) }
    }

    private fun toByteArray(values: Any?): ByteArray? {
        if (values == null) {
            return null
        }
        return when (values) {
            is ByteArray -> if (values.isEmpty()) null else values
            is List<*> -> {
                if (values.isEmpty()) {
                    null
                } else {
                    ByteArray(values.size) { idx ->
                        ((values[idx] as? Number)?.toInt() ?: 0 and 0xFF).toByte()
                    }
                }
            }
            else -> null
        }
    }
}
