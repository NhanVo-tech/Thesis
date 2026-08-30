package com.example.smart_car_app

import android.app.PendingIntent
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.nfc.NfcAdapter
import android.nfc.cardemulation.CardEmulation
import android.os.Build
import android.os.Bundle
import android.os.PowerManager
import android.provider.Settings
import android.util.Log
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel
import com.smartcaraccess.KeystoreBridge
import com.smartcar.phaseb.HandshakeChannel

class MainActivity : FlutterActivity() {
    companion object {
        private const val TAG = "MainActivity"
        private const val CHANNEL = "smartcar/keystore"
        private const val MASTER_CHANNEL = "smartcar/mastercard"
        private const val NFC_READER_CHANNEL = "smartcar/nfc_reader"
        private const val DEVICE_INFO_CHANNEL = "smartcar/device_info"
    }

    private var readerModeEnabled = false
    private var isForeground = false
    private var uwbBridge: UwbRangingBridge? = null
    private val nfcAdapter: NfcAdapter? by lazy { NfcAdapter.getDefaultAdapter(this) }
    private val pendingIntent: PendingIntent by lazy {
        val intent = Intent(this, javaClass).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP)
        PendingIntent.getActivity(this, 0, intent, PendingIntent.FLAG_IMMUTABLE)
    }

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        uwbBridge = UwbRangingBridge(this, flutterEngine.dartExecutor.binaryMessenger)
        // Register Phase B handshake channel (Android Keystore-backed ECDSA signing)
        HandshakeChannel.register(flutterEngine)
        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, CHANNEL).setMethodCallHandler { call, result ->
            try {
                when (call.method) {
                    "ensurePhaseAKey" -> {
                        val ok = KeystoreBridge.ensurePhaseAKey()
                        result.success(ok)
                    }
                    "getPhaseAPublicKey65" -> {
                        val pub = KeystoreBridge.getPhaseAPublicKey65()
                        result.success(pub)
                    }
                    "signPhaseA" -> {
                        val args = call.arguments as ByteArray
                        val sig = KeystoreBridge.signPhaseA(args)
                        result.success(sig)
                    }
                    "signPhaseAData" -> {
                        val args = call.arguments as ByteArray
                        val sig = KeystoreBridge.signPhaseAData(args)
                        result.success(sig)
                    }
                    else -> result.notImplemented()
                }
            } catch (e: Exception) {
                Log.e(TAG, "Keystore channel error", e)
                result.error("KEYSTORE_ERROR", e.message, null)
            }
        }

        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, MASTER_CHANNEL).setMethodCallHandler { call, result ->
            try {
                when (call.method) {
                    "setMasterSession" -> {
                        @Suppress("UNCHECKED_CAST")
                        val args = call.arguments as? Map<String, Any>
                        val vid = args?.get("vehicleId") as? ByteArray
                        val secret = args?.get("masterSecret") as? ByteArray
                        val ttlSeconds = args?.get("ttlSeconds") as? Int ?: 60

                        if (vid == null || secret == null) {
                            result.error("INVALID_ARGS", "vehicleId and masterSecret required", null)
                            return@setMethodCallHandler
                        }
                        if (vid.size != 8 || secret.size != 32) {
                            result.error("INVALID_ARGS", "vehicleId=8 bytes, masterSecret=32 bytes required", null)
                            return@setMethodCallHandler
                        }

                        MasterCardSession.setSession(secret, vid, ttlSeconds * 1000L)
                        // HCE needs reader mode off; disable while provisioning session is active
                        readerModeEnabled = false
                        if (isForeground) {
                            disableReaderMode()
                            disableForegroundDispatch()
                        }
                        result.success(true)
                    }
                    "clearMasterSession" -> {
                        MasterCardSession.clearSession()
                        // Restore reader mode when session ends (if foreground)
                        if (isForeground) {
                            readerModeEnabled = true
                            enableReaderMode()
                            enableForegroundDispatch()
                        }
                        result.success(true)
                    }
                    "isMasterSessionActive" -> {
                        result.success(MasterCardSession.isActive())
                    }
                    "getProvisioningVehicleBinding" -> {
                        val vid = DataStoreUtil.getProvisionedVehicleId(this)
                        val vpub = DataStoreUtil.getProvisionedVehiclePub(this)
                        val writeDataPayload = DataStoreUtil.getProvisionedWriteDataPayload(this)
                        val updatedAtMs = DataStoreUtil.getProvisionedUpdatedAtMs(this)
                        val epub = KeystoreBridge.getPhaseAPublicKey65()
                        if (vid == null || vpub == null || epub == null) {
                            result.success(null)
                            return@setMethodCallHandler
                        }

                        val map = hashMapOf<String, Any>(
                            "vehicleId" to vid,
                            "vehiclePubKey" to vpub,
                            "devicePubKey" to epub,
                        )
                        if (writeDataPayload != null) {
                            map["writeDataPayload"] = writeDataPayload
                        }
                        if (updatedAtMs != null) {
                            map["updatedAtMs"] = updatedAtMs
                        }
                        result.success(map)
                    }
                    "clearProvisioningVehicleBinding" -> {
                        val ok = DataStoreUtil.clearProvisioningVehicleBinding(this)
                        result.success(ok)
                    }
                    else -> result.notImplemented()
                }
            } catch (e: Exception) {
                Log.e(TAG, "Master card channel error", e)
                result.error("MASTER_SESSION_ERROR", e.message, null)
            }
        }

        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, NFC_READER_CHANNEL).setMethodCallHandler { call, result ->
            try {
                when (call.method) {
                    "enableReaderMode" -> {
                        readerModeEnabled = true
                        enableReaderMode()
                        result.success(true)
                    }
                    "disableReaderMode" -> {
                        readerModeEnabled = false
                        disableReaderMode()
                        result.success(true)
                    }
                    else -> result.notImplemented()
                }
            } catch (e: Exception) {
                Log.e(TAG, "NFC reader channel error", e)
                result.error("NFC_READER_ERROR", e.message, null)
            }
        }

        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, DEVICE_INFO_CHANNEL).setMethodCallHandler { call, result ->
            when (call.method) {
                "getAndroidSdkInt" -> result.success(android.os.Build.VERSION.SDK_INT)
                "getBatteryOptimizationStatus" -> {
                    val supported = Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
                    val ignoring = if (supported) isIgnoringBatteryOptimizations() else true
                    result.success(
                        hashMapOf<String, Any>(
                            "supported" to supported,
                            "ignored" to ignoring,
                            "sdkInt" to Build.VERSION.SDK_INT,
                        )
                    )
                }
                "requestIgnoreBatteryOptimizations" -> result.success(requestIgnoreBatteryOptimizations())
                "openBatteryOptimizationSettings" -> result.success(openBatteryOptimizationSettings())
                else -> result.notImplemented()
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Check for HCE support as required by Android documentation
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_NFC_HOST_CARD_EMULATION)) {
            Log.i(TAG, "✅ HCE feature is supported on this device")
            
            val nfcAdapter = NfcAdapter.getDefaultAdapter(this)
            if (nfcAdapter != null && nfcAdapter.isEnabled) {
                Log.i(TAG, "✅ NFC is enabled and ready")
                Log.i(TAG, "✅ HCE service (ProvisioningHostApduService) is active via manifest")
                Log.i(TAG, "   App can be in foreground or background - HCE works independently")
                
                // NOTE: Do NOT use setPreferredService for HCE
                // HCE services work automatically when declared in manifest
                // setPreferredService is for foreground dispatch, not HCE
            } else {
                Log.w(TAG, "⚠️ NFC is not available or disabled")
            }
        } else {
            Log.e(TAG, "❌ HCE feature not supported on this device")
        }
    }

    override fun onResume() {
        super.onResume()
        isForeground = true
        
        // CRITICAL: Keep HCE service active even when app is in foreground
        // Do NOT unset in onPause - let Android handle HCE lifecycle
        val nfcAdapter = NfcAdapter.getDefaultAdapter(this)
        if (nfcAdapter?.isEnabled == true) {
            val cardEmulation = CardEmulation.getInstance(nfcAdapter)
            val componentName = android.content.ComponentName(this, ProvisioningHostApduService::class.java)
            
            try {
                // setPreferredService only affects foreground dispatch, not HCE
                // For HCE to work in foreground, service must be in manifest with correct category
                Log.d(TAG, "Activity resumed - HCE service remains active")
            } catch (e: Exception) {
                Log.w(TAG, "Error in onResume: ${e.message}")
            }
        }

        // Suppress system NFC tag UI while app is foreground
        enableForegroundDispatch()
        if (!MasterCardSession.isActive()) {
            readerModeEnabled = true
            enableReaderMode()
        }
    }
    
    override fun onPause() {
        super.onPause()
        isForeground = false
        
        // DO NOT unset HCE service - it should remain active
        // HCE works independently of activity lifecycle
        Log.d(TAG, "Activity paused - HCE service continues running")

        if (readerModeEnabled) {
            disableReaderMode()
        }
        disableForegroundDispatch()
    }

    override fun onDestroy() {
        uwbBridge?.dispose()
        uwbBridge = null
        super.onDestroy()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        if (uwbBridge?.onRequestPermissionsResult(requestCode, grantResults) == true) {
            return
        }
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
    }

    private fun enableReaderMode() {
        val adapter = nfcAdapter ?: return
        if (!adapter.isEnabled) return
        val flags = NfcAdapter.FLAG_READER_NFC_A or
            NfcAdapter.FLAG_READER_NFC_B or
            NfcAdapter.FLAG_READER_SKIP_NDEF_CHECK
        adapter.enableReaderMode(this, { }, flags, null)
        Log.d(TAG, "Reader mode enabled")
    }

    private fun disableReaderMode() {
        val adapter = nfcAdapter ?: return
        adapter.disableReaderMode(this)
        Log.d(TAG, "Reader mode disabled")
    }

    private fun enableForegroundDispatch() {
        val adapter = nfcAdapter ?: return
        if (!adapter.isEnabled) return
        adapter.enableForegroundDispatch(this, pendingIntent, null, null)
        Log.d(TAG, "Foreground dispatch enabled")
    }

    private fun disableForegroundDispatch() {
        val adapter = nfcAdapter ?: return
        adapter.disableForegroundDispatch(this)
        Log.d(TAG, "Foreground dispatch disabled")
    }

    private fun isIgnoringBatteryOptimizations(): Boolean {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return true
        val pm = getSystemService(PowerManager::class.java) ?: return false
        return pm.isIgnoringBatteryOptimizations(packageName)
    }

    private fun requestIgnoreBatteryOptimizations(): Boolean {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return true
        return try {
            val intent = Intent(
                Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS,
                Uri.parse("package:$packageName"),
            )
            startActivity(intent)
            true
        } catch (e: Exception) {
            Log.w(TAG, "Failed to launch doze exemption request", e)
            false
        }
    }

    private fun openBatteryOptimizationSettings(): Boolean {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return true
        return try {
            val intent = Intent(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS)
            startActivity(intent)
            true
        } catch (e: Exception) {
            Log.w(TAG, "Failed to open battery optimization settings", e)
            false
        }
    }
}
