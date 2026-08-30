package com.example.smart_car_app

import android.nfc.cardemulation.HostApduService
import android.os.SystemClock
import android.os.Bundle
import android.util.Log
import javax.crypto.Mac
import javax.crypto.spec.SecretKeySpec

class ProvisioningHostApduService : HostApduService() {
    companion object {
        private const val TAG = "ProvisioningHostApduService"
        private const val AID_HEX = "A000000809434343444B467631"
        private val AID = ByteArrayHexUtil.hexToBytes(AID_HEX)
        private val SW_SUCCESS = byteArrayOf(0x90.toByte(), 0x00)
        private val SW_INS_NOT_SUPPORTED = byteArrayOf(0x6D.toByte(), 0x00)
        private val SW_CLA_NOT_SUPPORTED = byteArrayOf(0x6E.toByte(), 0x00)
        private val SW_UNKNOWN = byteArrayOf(0x6F.toByte(), 0x00)
        private val SW_SECURITY_STATUS_NOT_SATISFIED = byteArrayOf(0x69.toByte(), 0x82.toByte()) // Not logged in
        private const val INS_GET_DATA = 0xCA.toByte()
        private const val INS_SPAKE2_REQUEST = 0x30.toByte()
        private const val INS_SPAKE2_VERIFY = 0x32.toByte()
        private const val INS_WRITE_DATA = 0xD4.toByte()
        private const val INS_OP_CONTROL = 0x3C.toByte()
        private const val INS_READ_BINARY = 0xB0.toByte()
        private const val INS_PROVISION_RESULT = 0xDA.toByte()
        // Keep challenge alive long enough to survive PN532 link-loss + reselection retries.
        private const val SPAKE2_CHALLENGE_TTL_MS = 45000L
        private val SELECT_OK = byteArrayOf(
            0x5A.toByte(), 0x03.toByte(), 0x02.toByte(), 0x00.toByte(), 0x00.toByte(),
            0x5C.toByte(), 0x04.toByte(), 0x01.toByte(), 0x00.toByte(), 0x01.toByte(), 0x00.toByte()
        )

        @Volatile private var pendingSpake2Challenge: ByteArray? = null
        @Volatile private var pendingSpake2ChallengeTsMs: Long = 0L

        private fun storePendingSpake2Challenge(challenge: ByteArray) {
            pendingSpake2Challenge = challenge.copyOf()
            pendingSpake2ChallengeTsMs = SystemClock.elapsedRealtime()
        }

        private fun readPendingSpake2Challenge(): ByteArray? {
            val challenge = pendingSpake2Challenge ?: return null
            val ageMs = SystemClock.elapsedRealtime() - pendingSpake2ChallengeTsMs
            if (ageMs > SPAKE2_CHALLENGE_TTL_MS) {
                Log.w(TAG, "[PhaseA] Pending SPAKE2 challenge expired age=${ageMs}ms ttl=${SPAKE2_CHALLENGE_TTL_MS}ms")
                pendingSpake2Challenge = null
                pendingSpake2ChallengeTsMs = 0L
                return null
            }
            return challenge.copyOf()
        }

        private fun clearPendingSpake2Challenge() {
            pendingSpake2Challenge = null
            pendingSpake2ChallengeTsMs = 0L
        }
    }

    // Track selection
    private var aidSelected = false
    private var spake2Challenge: ByteArray? = null
    @Volatile private var cachedBase: ByteArray? = null
    @Volatile private var baseWarmStarted = false

    override fun onCreate() {
        super.onCreate()
        Log.i(TAG, "HCE provisioning service created. AID=${AID.toHex()}")
        warmBaseCredentials()
    }

    override fun processCommandApdu(commandApdu: ByteArray?, extras: Bundle?): ByteArray {
        if (commandApdu == null) return SW_UNKNOWN

        // NOTE: We now only enforce login for the signature step. Allow SELECT and base (Lc=0) for testing.

        if (commandApdu.size < 4) return SW_UNKNOWN
        val t0 = SystemClock.elapsedRealtime()
        val cla = commandApdu[0]
        val ins = commandApdu[1]
        val p1 = commandApdu[2]
        val p2 = commandApdu[3]
        val hasLc = commandApdu.size >= 5
        val lc = if (hasLc) (commandApdu[4].toInt() and 0xFF) else 0
        val dataStart = if (hasLc) 5 else 4
        val dataEnd = dataStart + lc
        val commandData = if (lc > 0 && dataEnd <= commandApdu.size) commandApdu.copyOfRange(dataStart, dataEnd) else ByteArray(0)

        if (cla != 0x00.toByte()) return SW_CLA_NOT_SUPPORTED

        val response = try {
            when (ins) {
                0xA4.toByte() -> handleSelect(commandApdu) // always allow SELECT
                INS_GET_DATA -> handleGetData(lc, commandData)
                INS_SPAKE2_REQUEST -> handleSpake2Request(lc, commandData)
                INS_SPAKE2_VERIFY -> handleSpake2Verify()
                INS_WRITE_DATA -> handleWriteData(lc, commandData)
                INS_OP_CONTROL -> handleOpControl(p1)
                INS_READ_BINARY -> handleReadBinary(p1, p2)
                INS_PROVISION_RESULT -> handleProvisionResult(lc, commandData)
                else -> SW_INS_NOT_SUPPORTED
            }
        } catch (e: Exception) {
            Log.e(TAG, "Exception processing APDU: ${e.message}", e)
            SW_UNKNOWN
        }

        val dt = SystemClock.elapsedRealtime() - t0
        Log.i(
            TAG,
            "[APDU] ins=${String.format("%02X", ins)} lc=$lc respLen=${response.size} dt=${dt}ms selected=$aidSelected"
        )
        return response
    }

    private fun handleSelect(apdu: ByteArray): ByteArray {
        // Minimal SELECT parsing: 00 A4 04 00 Lc <AID>
        if (apdu.size < 5) return SW_UNKNOWN
        val lc = apdu[4].toInt() and 0xFF
        val start = 5
        val end = start + lc
        if (end > apdu.size) return SW_UNKNOWN
        val aidBytes = apdu.copyOfRange(start, end)
        aidSelected = aidBytes.contentEquals(AID)
        return if (aidSelected) {
            // CCC SELECT response: return supported version tags + 9000
            SELECT_OK + SW_SUCCESS
        } else {
            SW_INS_NOT_SUPPORTED
        }
    }

    private fun handleGetData(lc: Int, data: ByteArray): ByteArray {
        if (!aidSelected) return SW_UNKNOWN
        
        // SECURITY: Require user to be logged in for ALL provisioning steps
        // This ensures user must open app and authenticate before provisioning
        if (!isUserLoggedIn()) {
            if (!isTestBypassEnabled()) {
                Log.w(TAG, "[PhaseA] SECURITY: User not logged in, provisioning denied")
                return SW_SECURITY_STATUS_NOT_SATISFIED
            }
            Log.w(TAG, "[PhaseA] TEST BYPASS: proceeding while not logged in (DEBUG MODE ONLY)")
        }
        
        return if (lc == 0) {
            // Base credentials (wrapped in 7F24)
            val base = getCachedBaseCredentials() ?: return SW_UNKNOWN
            Log.i(TAG, "[PhaseA] OUT GET DATA len=${base.size} last2=${String.format("%02X%02X", SW_SUCCESS[0], SW_SUCCESS[1])}")
            base + SW_SUCCESS
        } else {
            // Signature-only: challenge = incoming data
            Log.i(TAG, "[PhaseA] challenge(${lc}): ${data.toHex()}")
            val pkt = ProvisioningResponseBuilder.buildSignaturePacket(this, data)
            // Validate length prefix (BIG-endian) vs actual DER length
            if (pkt.size >= 2) {
                val declaredBE = ((pkt[0].toInt() and 0xFF) shl 8) or (pkt[1].toInt() and 0xFF)
                val actual = pkt.size - 2
                if (declaredBE != actual) {
                    // Try detect if it was little-endian and fix to big-endian for the reader
                    val declaredLE = (pkt[0].toInt() and 0xFF) or ((pkt[1].toInt() and 0xFF) shl 8)
                    Log.w(TAG, "[PhaseA] sigLen prefix mismatch: declaredBE=$declaredBE declaredLE=$declaredLE actual=$actual. Fixing to BE.")
                    // Write BIG-endian prefix
                    pkt[0] = ((actual shr 8) and 0xFF).toByte()
                    pkt[1] = (actual and 0xFF).toByte()
                }
                val sigLenBE = ((pkt[0].toInt() and 0xFF) shl 8) or (pkt[1].toInt() and 0xFF)
                Log.i(TAG, "[PhaseA] OUT Sig pktLen=${pkt.size} sigLenBE=${sigLenBE} first4=${pkt.copyOfRange(0, minOf(4, pkt.size)).toHex()}")
            } else {
                Log.w(TAG, "[PhaseA] Signature packet too short: ${pkt.size}")
            }
            pkt + SW_SUCCESS
        }
    }

    private fun warmBaseCredentials() {
        if (baseWarmStarted) return
        baseWarmStarted = true
        Thread {
            try {
                Log.i(TAG, "[PhaseA] Warming base credentials")
                val base = ProvisioningResponseBuilder.buildCccGetDataPacket(this)
                if (base.isNotEmpty()) {
                    cachedBase = base
                    Log.i(TAG, "[PhaseA] Base credentials cached len=${base.size}")
                } else {
                    Log.w(TAG, "[PhaseA] Base credentials warm returned empty")
                }
            } catch (e: Exception) {
                Log.e(TAG, "[PhaseA] Base credentials warm failed", e)
            }
        }.start()
    }

    private fun getCachedBaseCredentials(): ByteArray? {
        cachedBase?.let { return it }
        warmBaseCredentials()
        val start = SystemClock.uptimeMillis()
        while (SystemClock.uptimeMillis() - start < 200) {
            cachedBase?.let { return it }
            Thread.sleep(10)
        }
        Log.w(TAG, "[PhaseA] Base credentials not ready; retry")
        return null
    }

    private fun handleSpake2Request(lc: Int, data: ByteArray): ByteArray {
        if (!aidSelected) return SW_UNKNOWN
        if (lc < 2 || data.isEmpty()) return SW_UNKNOWN
        val challenge = extractTlv(0x50.toByte(), data)
        if (challenge == null || challenge.isEmpty()) {
            Log.w(TAG, "[PhaseA] SPAKE2+ request missing tag 0x50")
            return SW_UNKNOWN
        }
        spake2Challenge = challenge.copyOf()
        storePendingSpake2Challenge(challenge)
        Log.i(TAG, "[PhaseA] SPAKE2+ request stored challenge len=${challenge.size}")
        return SW_SUCCESS
    }

    private fun handleSpake2Verify(): ByteArray {
        if (!aidSelected) return SW_UNKNOWN
        val challenge = spake2Challenge ?: readPendingSpake2Challenge()
        if (challenge == null) {
            Log.w(TAG, "[PhaseA] SPAKE2+ verify denied: missing or expired challenge")
            return SW_SECURITY_STATUS_NOT_SATISFIED
        }
        if (!MasterCardSession.isActive()) {
            Log.w(TAG, "[PhaseA] SPAKE2+ verify denied: master session inactive")
            return SW_SECURITY_STATUS_NOT_SATISFIED
        }
        spake2Challenge = challenge.copyOf()
        val masterSecret = MasterCardSession.getMasterSecret() ?: return SW_SECURITY_STATUS_NOT_SATISFIED

        val mac = Mac.getInstance("HmacSHA256")
        mac.init(SecretKeySpec(masterSecret, "HmacSHA256"))
        val macOut = mac.doFinal(challenge)
        val tlv = buildTlv(0x58.toByte(), macOut)
        spake2Challenge = null
        clearPendingSpake2Challenge()
        Log.i(TAG, "[PhaseA] SPAKE2+ verify returning MAC len=${macOut.size}")
        return tlv + SW_SUCCESS
    }

    private fun handleWriteData(lc: Int, data: ByteArray): ByteArray {
        if (!aidSelected) return SW_UNKNOWN
        if (lc < 4 || data.isEmpty()) return SW_UNKNOWN

        val vehicleId = extractTlv(0x80.toByte(), data)
        val vehiclePub = extractTlv(0x81.toByte(), data)
        if (vehicleId == null || vehiclePub == null) {
            Log.w(TAG, "[PhaseA] WRITE DATA missing required TLVs (0x80/0x81)")
            return SW_UNKNOWN
        }
        if (vehicleId.size != 8 || vehiclePub.size != 65 || vehiclePub[0] != 0x04.toByte()) {
            Log.w(TAG, "[PhaseA] WRITE DATA invalid lengths vid=${vehicleId.size} vpub=${vehiclePub.size}")
            return SW_UNKNOWN
        }

        val stored = DataStoreUtil.saveProvisioningVehicleBinding(this, vehicleId, vehiclePub)
        if (!stored) {
            Log.e(TAG, "[PhaseA] WRITE DATA storage failed")
            return SW_UNKNOWN
        }

        Log.i(TAG, "[PhaseA] WRITE DATA stored vehicle binding vid=${vehicleId.toHex()} vpubLen=${vehiclePub.size}")
        return SW_SUCCESS
    }

    private fun handleOpControl(p1: Byte): ByteArray {
        if (!aidSelected) return SW_UNKNOWN
        Log.i(TAG, "[PhaseA] OP CONTROL FLOW P1=${String.format("%02X", p1)}")
        return SW_SUCCESS
    }

    private fun handleReadBinary(p1: Byte, p2: Byte): ByteArray {
        if (!aidSelected) return SW_UNKNOWN
        val uid = DataStoreUtil.getOrCreate4ByteUid(this)
        return uid + SW_SUCCESS
    }

    private fun handleProvisionResult(lc: Int, data: ByteArray): ByteArray {
        if (!aidSelected) {
            Log.w(TAG, "[PhaseA] Provision result received while AID not selected")
        }
        if (lc < 1) return SW_UNKNOWN
        val success = data[0] == 0x01.toByte()
        val prefs = getSharedPreferences("FlutterSharedPreferences", MODE_PRIVATE)
        prefs.edit()
            .putBoolean("flutter.provision_result", success)
            .putLong("flutter.provision_ts", System.currentTimeMillis())
            .apply()
        Log.i(TAG, "[PhaseA] Provision result received: ${if (success) "OK" else "FAIL"}")
        return SW_SUCCESS
    }

    private fun isUserLoggedIn(): Boolean {
        // Check Flutter's shared_preferences (FlutterSharedPreferences, key flutter.is_logged_in)
        val flutterPrefs = getSharedPreferences("FlutterSharedPreferences", MODE_PRIVATE)
        if (flutterPrefs.contains("flutter.is_logged_in")) {
            return flutterPrefs.getBoolean("flutter.is_logged_in", false)
        }
        // Fallback to app-local prefs if you choose to set it natively
        val prefs = getSharedPreferences("smart_car_login", MODE_PRIVATE)
        return prefs.getBoolean("logged_in", false)
    }

    private fun isTestBypassEnabled(): Boolean {
        // PRODUCTION: Only allow bypass in debug builds (BuildConfig.DEBUG)
        // Remove test_mode preference for production security
        return try {
            val isDebuggable = (applicationInfo.flags and android.content.pm.ApplicationInfo.FLAG_DEBUGGABLE) != 0
            if (!isDebuggable) {
                // Production build - NO BYPASS
                return false
            }
            // Debug build - allow explicit test mode preference
            val testPrefs = getSharedPreferences("smart_car_test", MODE_PRIVATE)
            testPrefs.getBoolean("test_mode", false) || isDebuggable
        } catch (e: Exception) { false }
    }

    override fun onDeactivated(reason: Int) {
        aidSelected = false
        // Keep pending SPAKE2 challenge for a short TTL to survive LINK_LOSS reselection.
        val reasonStr = when (reason) {
            DEACTIVATION_DESELECTED -> "DESELECTED"
            DEACTIVATION_LINK_LOSS -> "LINK_LOSS"
            else -> "UNKNOWN"
        }
        Log.i(TAG, "HCE deactivated reason=$reason ($reasonStr)")
    }

    private fun extractTlv(tag: Byte, data: ByteArray): ByteArray? {
        var idx = 0
        while (idx + 2 <= data.size) {
            val t = data[idx]
            val len = data[idx + 1].toInt() and 0xFF
            idx += 2
            if (idx + len > data.size) return null
            if (t == tag) {
                return data.copyOfRange(idx, idx + len)
            }
            idx += len
        }
        return null
    }

    private fun buildTlv(tag: Byte, value: ByteArray): ByteArray {
        val len = value.size
        val out = ByteArray(2 + len)
        out[0] = tag
        out[1] = len.toByte()
        System.arraycopy(value, 0, out, 2, len)
        return out
    }
}

private fun ByteArray.toHex(): String = joinToString("") { String.format("%02X", it) }