package com.smartcar.phaseb

import android.util.Log
import com.smartcaraccess.KeystoreBridge
import java.security.*
import java.security.interfaces.ECPrivateKey
import java.security.interfaces.ECPublicKey
import java.security.spec.ECGenParameterSpec
import java.security.spec.ECPoint
import java.security.spec.ECPublicKeySpec
import javax.crypto.KeyAgreement
import javax.crypto.Mac
import javax.crypto.spec.SecretKeySpec
import java.math.BigInteger

/**
 * PhaseBCrypto - Cryptographic operations for Phase B Authentication
 * 
 * Handles:
 * - Ephemeral keypair generation (session-based, not stored)
 * - ECDSA signing with Android Keystore identity key
 * - ECDH shared secret computation
 * - HKDF-SHA256 key derivation
 */
object PhaseBCrypto {
    private const val TAG = "PhaseBCrypto"
    
    // Ephemeral keypair for current session (not stored in Keystore)
    private var ephemeralKeyPair: KeyPair? = null
    
    /**
     * Generate ephemeral P-256 keypair for Phase B session.
     * Returns (publicKey65, privateKeyEncoded) where publicKey65 is uncompressed format.
     */
    fun generateEphemeralKeypair(): Pair<ByteArray, ByteArray>? {
        return try {
            Log.d(TAG, "Generating ephemeral P-256 keypair for Phase B session")
            
            val keyPairGenerator = KeyPairGenerator.getInstance("EC")
            keyPairGenerator.initialize(ECGenParameterSpec("secp256r1"), SecureRandom())
            
            ephemeralKeyPair = keyPairGenerator.generateKeyPair()
            
            val publicKey = ephemeralKeyPair!!.public as ECPublicKey
            val publicKey65 = ecPublicKeyToUncompressed(publicKey)
            val privateKeyEncoded = ephemeralKeyPair!!.private.encoded
            
            Log.i(TAG, "✓ Ephemeral keypair generated")
            Log.d(TAG, "   Public key (65 bytes): ${publicKey65.toHex()}")
            
            Pair(publicKey65, privateKeyEncoded)
            
        } catch (e: Exception) {
            Log.e(TAG, "Failed to generate ephemeral keypair", e)
            null
        }
    }
    
    /**
     * Get current ephemeral public key (65 bytes uncompressed).
     */
    fun getEphemeralPublicKey(): ByteArray? {
        return try {
            if (ephemeralKeyPair == null) {
                Log.w(TAG, "No ephemeral keypair available")
                return null
            }
            
            val publicKey = ephemeralKeyPair!!.public as ECPublicKey
            ecPublicKeyToUncompressed(publicKey)
            
        } catch (e: Exception) {
            Log.e(TAG, "Failed to get ephemeral public key", e)
            null
        }
    }
    
    /**
     * Sign data with Android Keystore identity key (Phase A key).
     * Returns DER-encoded ECDSA signature.
     */
    fun signWithIdentityKey(data: ByteArray): ByteArray? {
        return try {
            Log.d(TAG, "Signing with identity key (Android Keystore)")
            
            // Use Phase A key from Keystore
            val signature = KeystoreBridge.signPhaseA(data)
            
            if (signature != null) {
                Log.i(TAG, "✓ Data signed successfully: ${signature.size} bytes")
            }
            
            signature
            
        } catch (e: Exception) {
            Log.e(TAG, "Failed to sign with identity key", e)
            null
        }
    }
    
    /**
     * Compute ECDH shared secret using ephemeral private key and ECU public key.
     * Returns 32-byte shared secret.
     */
    fun computeECDH(ecuPublicKey65: ByteArray): ByteArray? {
        return try {
            if (ephemeralKeyPair == null) {
                Log.e(TAG, "No ephemeral keypair for ECDH")
                return null
            }
            
            if (ecuPublicKey65.size != 65 || ecuPublicKey65[0] != 0x04.toByte()) {
                Log.e(TAG, "Invalid ECU public key format")
                return null
            }
            
            Log.d(TAG, "Computing ECDH shared secret")
            
            // Parse ECU public key
            val ecuPublicKey = uncompressedToECPublicKey(ecuPublicKey65)
            
            // Compute ECDH
            val keyAgreement = KeyAgreement.getInstance("ECDH")
            keyAgreement.init(ephemeralKeyPair!!.private)
            keyAgreement.doPhase(ecuPublicKey, true)
            
            val sharedSecret = keyAgreement.generateSecret()
            
            // Ensure 32 bytes (pad or truncate if needed)
            val result = when {
                sharedSecret.size == 32 -> sharedSecret
                sharedSecret.size > 32 -> sharedSecret.copyOfRange(sharedSecret.size - 32, sharedSecret.size)
                else -> ByteArray(32).also { System.arraycopy(sharedSecret, 0, it, 32 - sharedSecret.size, sharedSecret.size) }
            }
            
            Log.i(TAG, "✓ ECDH shared secret computed: 32 bytes")
            Log.d(TAG, "   Shared secret: ${result.toHex()}")
            
            result
            
        } catch (e: Exception) {
            Log.e(TAG, "Failed to compute ECDH", e)
            null
        }
    }
    
    /**
     * Derive session keys using HKDF-SHA256.
     * Returns (encKey32, macKey32).
     */
    fun deriveSessionKeys(
        sharedSecret: ByteArray,
        phoneEphemeralPub: ByteArray,
        ecuEphemeralPub: ByteArray
    ): Pair<ByteArray, ByteArray>? {
        return try {
            Log.d(TAG, "Deriving session keys with HKDF-SHA256")
            
            // Derive encryption key
            // Label is 14 bytes, pad to 16 with null bytes to match ESP32
            val labelEnc = ByteArray(16)
            "SmartCarv1|ENC".toByteArray().copyInto(labelEnc, 0, 0, 14)
            val infoEnc = labelEnc + ecuEphemeralPub + phoneEphemeralPub
            val encKey = hkdfSha256(
                ikm = sharedSecret,
                salt = ByteArray(0),
                info = infoEnc,
                length = 32
            )
            
            // Derive MAC key
            // Label is 14 bytes, pad to 16 with null bytes to match ESP32
            val labelMac = ByteArray(16)
            "SmartCarv1|MAC".toByteArray().copyInto(labelMac, 0, 0, 14)
            val infoMac = labelMac + ecuEphemeralPub + phoneEphemeralPub
            val macKey = hkdfSha256(
                ikm = sharedSecret,
                salt = ByteArray(0),
                info = infoMac,
                length = 32
            )
            
            Log.i(TAG, "✓ Session keys derived")
            Log.d(TAG, "   Encryption key: ${encKey.toHex()}")
            Log.d(TAG, "   MAC key: ${macKey.toHex()}")
            
            Pair(encKey, macKey)
            
        } catch (e: Exception) {
            Log.e(TAG, "Failed to derive session keys", e)
            null
        }
    }
    
    /**
     * Sign challenge with identity key for final Phase B verification.
     */
    fun signChallenge(challenge: ByteArray): ByteArray? {
        return try {
            Log.d(TAG, "Signing challenge (${challenge.size} bytes)")
            
            val signature = KeystoreBridge.signPhaseA(challenge)
            
            if (signature != null) {
                Log.i(TAG, "✓ Challenge signed: ${signature.size} bytes")
            }
            
            signature
            
        } catch (e: Exception) {
            Log.e(TAG, "Failed to sign challenge", e)
            null
        }
    }
    
    /**
     * Reset ephemeral keys (on disconnect or session end).
     */
    fun resetEphemeralKeys() {
        Log.d(TAG, "Resetting ephemeral keys")
        ephemeralKeyPair = null
    }
    
    // ========== Helper Functions ==========
    
    /**
     * Convert ECPublicKey to uncompressed format (65 bytes: 0x04 + X + Y).
     */
    private fun ecPublicKeyToUncompressed(publicKey: ECPublicKey): ByteArray {
        val point = publicKey.w
        val x = point.affineX.toByteArray()
        val y = point.affineY.toByteArray()
        
        // Ensure 32 bytes for each coordinate
        val xBytes = x.let { if (it.size > 32) it.copyOfRange(it.size - 32, it.size) else it.copyOf(32) }
        val yBytes = y.let { if (it.size > 32) it.copyOfRange(it.size - 32, it.size) else it.copyOf(32) }
        
        val result = ByteArray(65)
        result[0] = 0x04 // Uncompressed marker
        System.arraycopy(xBytes, 0, result, 1, 32)
        System.arraycopy(yBytes, 0, result, 33, 32)
        
        return result
    }
    
    /**
     * Convert uncompressed bytes (65) to ECPublicKey.
     */
    private fun uncompressedToECPublicKey(bytes: ByteArray): PublicKey {
        if (bytes.size != 65 || bytes[0] != 0x04.toByte()) {
            throw IllegalArgumentException("Invalid uncompressed EC public key format")
        }
        
        val x = BigInteger(1, bytes.copyOfRange(1, 33))
        val y = BigInteger(1, bytes.copyOfRange(33, 65))
        
        val point = ECPoint(x, y)
        
        // Get P-256 parameters
        val keyFactory = KeyFactory.getInstance("EC")
        val params = (KeyPairGenerator.getInstance("EC").apply {
            initialize(ECGenParameterSpec("secp256r1"))
        }.generateKeyPair().public as ECPublicKey).params
        
        val keySpec = ECPublicKeySpec(point, params)
        return keyFactory.generatePublic(keySpec)
    }
    
    /**
     * HKDF-SHA256 implementation.
     * 
     * HKDF-Extract: PRK = HMAC-SHA256(salt, IKM)
     * HKDF-Expand: OKM = HMAC-SHA256(PRK, info || 0x01)
     */
    private fun hkdfSha256(
        ikm: ByteArray,
        salt: ByteArray,
        info: ByteArray,
        length: Int
    ): ByteArray {
        // HKDF-Extract
        val actualSalt = if (salt.isEmpty()) ByteArray(32) else salt
        val prk = hmacSha256(actualSalt, ikm)
        
        // HKDF-Expand
        val mac = Mac.getInstance("HmacSHA256")
        mac.init(SecretKeySpec(prk, "HmacSHA256"))
        
        val okm = ByteArray(length)
        var offset = 0
        var counter = 1
        var previousT = ByteArray(0)
        
        while (offset < length) {
            mac.update(previousT)
            mac.update(info)
            mac.update(counter.toByte())
            previousT = mac.doFinal()
            
            val copyLength = minOf(previousT.size, length - offset)
            System.arraycopy(previousT, 0, okm, offset, copyLength)
            offset += copyLength
            counter++
        }
        
        return okm
    }
    
    /**
     * HMAC-SHA256.
     */
    private fun hmacSha256(key: ByteArray, data: ByteArray): ByteArray {
        val mac = Mac.getInstance("HmacSHA256")
        mac.init(SecretKeySpec(key, "HmacSHA256"))
        return mac.doFinal(data)
    }
}

// Extension function for hex encoding
private fun ByteArray.toHex(): String = joinToString("") { "%02X".format(it) }
