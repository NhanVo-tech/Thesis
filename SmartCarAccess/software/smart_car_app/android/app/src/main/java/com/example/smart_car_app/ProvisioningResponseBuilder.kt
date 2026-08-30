package com.example.smart_car_app

import android.content.Context
import android.util.Log
import com.smartcaraccess.KeystoreBridge
import java.nio.ByteBuffer
import java.nio.ByteOrder
import javax.crypto.Mac
import javax.crypto.spec.SecretKeySpec

/**
 * ProvisioningResponseBuilder - Build APDU response packets for Phase A (NFC Provisioning)
 * 
 * Handles two types of responses:
 * 1. Base credentials: keyId + publicKey + certificate
 * 2. Signature: DER-encoded ECDSA signature over challenge
 */
object ProvisioningResponseBuilder {
    private const val TAG = "ProvisioningResponseBuilder"
    private const val MAX_DER_SIGNATURE_LEN = 70
    private const val MAX_SIGNATURE_RETRY_ATTEMPTS = 32
    
    /**
     * Build base credentials packet for Phase A.
        * Format: [keyIdLen(1) + keyId(keyIdLen) + publicKey(65) + certLen(2) + certificate + optional TLVs]
        * Optional TLVs:
        * - 0x90 len=1  : fast artifact version
        * - 0x91 len=32 : fast artifact key bytes
     * 
     * For now, we don't include certificate (certLen = 0).
    * Returns variable length. Legacy readers can ignore trailing bytes.
     */
    fun buildBaseCredentialsPacket(context: Context): ByteArray {
        Log.d(TAG, "Building base credentials packet")
        
        // Ensure key exists in Keystore
        KeystoreBridge.ensurePhaseAKey()
        
        // Get public key
        val publicKey65 = KeystoreBridge.getPhaseAPublicKey65()
        if (publicKey65 == null) {
            Log.e(TAG, "Failed to get public key from Keystore")
            return ByteArray(0)
        }
        
        val fastVersion = DataStoreUtil.getOrCreateFastArtifactVersion(context)
        val fastKey = DataStoreUtil.getOrCreateFastArtifactKey(context)
        if (fastKey.size != 32) {
            Log.e(TAG, "Invalid fast artifact key size: ${fastKey.size}")
            return ByteArray(0)
        }

        val fastVersionTlv = byteArrayOf(0x90.toByte(), 0x01.toByte(), (fastVersion and 0xFF).toByte())
        val fastKeyTlv = byteArrayOf(0x91.toByte(), 0x20.toByte()) + fastKey

        // Build packet
        val packet = ByteArray(69 + fastVersionTlv.size + fastKeyTlv.size)
        var offset = 0
        
        // KeyId length (1 byte)
        packet[offset++] = 0x01 // keyId is 1 byte
        
        // KeyId (1 byte) - arbitrary identifier
        packet[offset++] = 0x01 // actual keyId value
        
        // Public key (65 bytes uncompressed)
        System.arraycopy(publicKey65, 0, packet, offset, 65)
        offset += 65
        
        // Certificate length (2 bytes, big-endian) - 0 for now
        packet[offset++] = 0x00
        packet[offset++] = 0x00

        // Append fast-path artifact TLVs (versioned contract extension).
        System.arraycopy(fastVersionTlv, 0, packet, offset, fastVersionTlv.size)
        offset += fastVersionTlv.size
        System.arraycopy(fastKeyTlv, 0, packet, offset, fastKeyTlv.size)
        
        Log.i(TAG, "✓ Base credentials packet built: ${packet.size} bytes")
        Log.d(TAG, "   KeyIdLen: 1, KeyId: 0x01")
        Log.d(TAG, "   Public key: ${publicKey65.toHex()}")
        Log.d(TAG, "   Cert length: 0")
        Log.d(TAG, "   Fast artifact version: ${fastVersion and 0xFF}")
        Log.d(TAG, "   Fast artifact key len: ${fastKey.size}")
        
        return packet
    }

    /**
     * Wrap base credentials in CCC GET DATA tag 7F24.
     */
    fun buildCccGetDataPacket(context: Context): ByteArray {
        val inner = buildBaseCredentialsPacket(context)
        if (inner.isEmpty()) return ByteArray(0)
        return wrapWith7F24(inner)
    }
    
    /**
     * Build signature packet for Phase A challenge-response.
     * Format: [sigLen(2, big-endian) + DER_signature]
     * 
     * @param context Android context
     * @param challenge Challenge from ECU (vehicleId(8) + nonce(16))
     * @return Signature packet
     */
    fun buildSignaturePacket(context: Context, challenge: ByteArray): ByteArray {
        Log.d(TAG, "Building signature packet for challenge: ${challenge.toHex()}")
        
        if (challenge.size != 24) {
            Log.w(TAG, "Challenge size is ${challenge.size}, expected 24 bytes")
        }
        
        val signature = buildCompactSignature(challenge)

        if (signature == null) {
            Log.e(TAG, "Failed to sign challenge")
            return ByteArray(0)
        }
        
        // Build packet: [sigLen(2, BE) + signature]
        val packet = ByteArray(2 + signature.size)
        
        // Signature length (big-endian)
        packet[0] = ((signature.size shr 8) and 0xFF).toByte()
        packet[1] = (signature.size and 0xFF).toByte()
        
        // Signature bytes
        System.arraycopy(signature, 0, packet, 2, signature.size)
        
        Log.i(TAG, "✓ Signature packet built: ${packet.size} bytes")
        Log.d(TAG, "   Signature length (BE): ${signature.size}")
        Log.d(TAG, "   Signature: ${signature.toHex()}")
        
        return packet
    }

    private fun wrapWith7F24(inner: ByteArray): ByteArray {
        val len = inner.size
        return if (len <= 0x7F) {
            byteArrayOf(0x7F.toByte(), 0x24.toByte(), len.toByte()) + inner
        } else {
            byteArrayOf(0x7F.toByte(), 0x24.toByte(), 0x81.toByte(), len.toByte()) + inner
        }
    }

    /**
     * Build signature packet with HMAC for master-card provisioning.
     * Format: [mac(32) + sigLen(2, big-endian) + DER_signature]
     * MAC = HMAC_SHA256(masterSecret, challenge || phonePub65)
     */
    fun buildSignaturePacketWithMac(
        context: Context,
        challenge: ByteArray,
        masterSecret: ByteArray
    ): ByteArray {
        Log.d(TAG, "Building signature+MAC packet for master-card provisioning")

        // Ensure key exists in Keystore and fetch phone public key for binding
        KeystoreBridge.ensurePhaseAKey()
        val phonePub65 = KeystoreBridge.getPhaseAPublicKey65()
        if (phonePub65 == null) {
            Log.e(TAG, "Failed to get phone public key for MAC binding")
            return ByteArray(0)
        }

        val signature = buildCompactSignature(challenge)
        if (signature == null) {
            Log.e(TAG, "Failed to sign challenge")
            return ByteArray(0)
        }

        val macInput = ByteArray(challenge.size + phonePub65.size)
        System.arraycopy(challenge, 0, macInput, 0, challenge.size)
        System.arraycopy(phonePub65, 0, macInput, challenge.size, phonePub65.size)

        val mac = Mac.getInstance("HmacSHA256")
        mac.init(SecretKeySpec(masterSecret, "HmacSHA256"))
        val macOut = mac.doFinal(macInput)

        val packet = ByteArray(32 + 2 + signature.size)
        var offset = 0
        System.arraycopy(macOut, 0, packet, offset, 32)
        offset += 32

        packet[offset++] = ((signature.size shr 8) and 0xFF).toByte()
        packet[offset++] = (signature.size and 0xFF).toByte()
        System.arraycopy(signature, 0, packet, offset, signature.size)

        Log.i(TAG, "✓ Signature+MAC packet built: ${packet.size} bytes")
        Log.d(TAG, "   MAC: ${macOut.toHex()}")
        Log.d(TAG, "   Signature length (BE): ${signature.size}")

        return packet
    }

    private fun buildCompactSignature(challenge: ByteArray): ByteArray? {
        // Some PN532 + phone combinations are flaky when APDU payload exceeds 72 bytes,
        // so keep retrying until we get a compact DER signature when possible.
        var bestSignature: ByteArray? = null
        for (attempt in 1..MAX_SIGNATURE_RETRY_ATTEMPTS) {
            val signature = KeystoreBridge.signPhaseA(challenge) ?: continue
            if (bestSignature == null || signature.size < bestSignature!!.size) {
                bestSignature = signature
            }
            if (signature.size <= MAX_DER_SIGNATURE_LEN) {
                if (attempt > 1) {
                    Log.i(TAG, "Using compact DER signature after retry #$attempt (len=${signature.size})")
                }
                return signature
            }
        }

        if (bestSignature != null) {
            Log.w(
                TAG,
                "Could not obtain compact DER signature after $MAX_SIGNATURE_RETRY_ATTEMPTS attempts; using shortest len=${bestSignature!!.size}",
            )
        }
        return bestSignature
    }
    
    /**
     * Verify signature length prefix is correctly encoded as big-endian.
     * This is a helper for debugging.
     */
    fun verifySigLenPrefix(packet: ByteArray): Boolean {
        if (packet.size < 2) return false
        
        val declaredBE = ((packet[0].toInt() and 0xFF) shl 8) or (packet[1].toInt() and 0xFF)
        val actualLen = packet.size - 2
        
        if (declaredBE != actualLen) {
            Log.w(TAG, "Signature length mismatch: declared(BE)=$declaredBE, actual=$actualLen")
            return false
        }
        
        return true
    }
}

// Extension function for hex encoding
private fun ByteArray.toHex(): String = joinToString("") { "%02X".format(it) }
