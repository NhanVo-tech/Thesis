package com.smartcaraccess

import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import android.util.Log
import java.security.*
import java.security.interfaces.ECPublicKey
import java.security.spec.ECGenParameterSpec

/**
 * KeystoreBridge for Phase A (NFC Provisioning)
 * 
 * Manages EC P-256 keypair in Android Keystore for secure identity.
 * Used during NFC provisioning to sign challenges from ECU.
 */
object KeystoreBridge {
    private const val TAG = "KeystoreBridge"
    private const val KEYSTORE_PROVIDER = "AndroidKeyStore"
    private const val PHASE_A_KEY_ALIAS = "smart_car_phone_identity_p256"
    
    private val keyStore: KeyStore by lazy {
        KeyStore.getInstance(KEYSTORE_PROVIDER).apply { load(null) }
    }

    /**
     * Ensure Phase A identity key exists in Android Keystore.
     * Generates a new P-256 keypair if not present.
     * 
     * @return true if key exists or was created successfully
     */
    fun ensurePhaseAKey(): Boolean {
        return try {
            if (keyStore.containsAlias(PHASE_A_KEY_ALIAS)) {
                Log.d(TAG, "Phase A key already exists: $PHASE_A_KEY_ALIAS")
                return true
            }
            
            Log.i(TAG, "Generating new Phase A identity key in Keystore")
            
            val keyPairGenerator = KeyPairGenerator.getInstance(
                KeyProperties.KEY_ALGORITHM_EC,
                KEYSTORE_PROVIDER
            )
            
            val parameterSpec = KeyGenParameterSpec.Builder(
                PHASE_A_KEY_ALIAS,
                KeyProperties.PURPOSE_SIGN or KeyProperties.PURPOSE_VERIFY
            ).apply {
                setAlgorithmParameterSpec(ECGenParameterSpec("secp256r1"))
                setDigests(KeyProperties.DIGEST_SHA256)
                setUserAuthenticationRequired(false) // Allow background HCE service to access
                setInvalidatedByBiometricEnrollment(false)
            }.build()
            
            keyPairGenerator.initialize(parameterSpec)
            val keyPair = keyPairGenerator.generateKeyPair()
            
            Log.i(TAG, "✓ Phase A key generated successfully")
            Log.d(TAG, "   Algorithm: ${keyPair.public.algorithm}")
            Log.d(TAG, "   Format: ${keyPair.public.format}")
            
            true
        } catch (e: Exception) {
            Log.e(TAG, "Failed to ensure Phase A key", e)
            false
        }
    }

    /**
     * Get Phase A public key in uncompressed format (65 bytes: 0x04 + X + Y).
     * 
     * @return ByteArray of 65 bytes, or null if key doesn't exist
     */
    fun getPhaseAPublicKey65(): ByteArray? {
        return try {
            if (!keyStore.containsAlias(PHASE_A_KEY_ALIAS)) {
                Log.w(TAG, "Phase A key not found, generating...")
                if (!ensurePhaseAKey()) {
                    return null
                }
            }
            
            val entry = keyStore.getEntry(PHASE_A_KEY_ALIAS, null) as? KeyStore.PrivateKeyEntry
            if (entry == null) {
                Log.e(TAG, "Failed to get Phase A key entry")
                return null
            }
            
            val publicKey = entry.certificate.publicKey as? ECPublicKey
            if (publicKey == null) {
                Log.e(TAG, "Public key is not an EC key")
                return null
            }
            
            // Extract uncompressed point (0x04 + X + Y)
            val point = publicKey.w
            val x = point.affineX.toByteArray()
            val y = point.affineY.toByteArray()
            
            // Ensure 32 bytes for each coordinate (remove sign byte if present)
            val xBytes = x.let { if (it.size > 32) it.copyOfRange(it.size - 32, it.size) else it.copyOf(32) }
            val yBytes = y.let { if (it.size > 32) it.copyOfRange(it.size - 32, it.size) else it.copyOf(32) }
            
            val result = ByteArray(65)
            result[0] = 0x04 // Uncompressed point marker
            System.arraycopy(xBytes, 0, result, 1, 32)
            System.arraycopy(yBytes, 0, result, 33, 32)
            
            Log.d(TAG, "Phase A public key (65 bytes): ${result.toHex()}")
            result
            
        } catch (e: Exception) {
            Log.e(TAG, "Failed to get Phase A public key", e)
            null
        }
    }

    /**
     * Sign data with Phase A identity key using ECDSA-SHA256.
     * Returns DER-encoded signature.
     * 
     * @param data Data to sign
     * @return DER-encoded signature, or null on failure
     */
    fun signPhaseA(data: ByteArray): ByteArray? {
        return try {
            if (!keyStore.containsAlias(PHASE_A_KEY_ALIAS)) {
                Log.e(TAG, "Phase A key not found for signing")
                return null
            }
            
            val entry = keyStore.getEntry(PHASE_A_KEY_ALIAS, null) as? KeyStore.PrivateKeyEntry
            if (entry == null) {
                Log.e(TAG, "Failed to get Phase A key entry for signing")
                return null
            }
            
            val signature = Signature.getInstance("SHA256withECDSA")
            signature.initSign(entry.privateKey)
            signature.update(data)
            val sig = signature.sign()
            
            Log.d(TAG, "Phase A signature generated: ${sig.size} bytes (DER)")
            Log.d(TAG, "   Signature: ${sig.toHex()}")
            
            sig
            
        } catch (e: Exception) {
            Log.e(TAG, "Failed to sign with Phase A key", e)
            null
        }
    }

    /**
     * Sign raw data (for Phase A challenge response).
     * Alias for signPhaseA() for consistency.
     */
    fun signPhaseAData(data: ByteArray): ByteArray? = signPhaseA(data)

    /**
     * Delete Phase A key from Keystore (for testing/reset).
     */
    fun deletePhaseAKey(): Boolean {
        return try {
            if (keyStore.containsAlias(PHASE_A_KEY_ALIAS)) {
                keyStore.deleteEntry(PHASE_A_KEY_ALIAS)
                Log.i(TAG, "Phase A key deleted")
                true
            } else {
                Log.d(TAG, "Phase A key not found, nothing to delete")
                false
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to delete Phase A key", e)
            false
        }
    }

    /**
     * Check if Phase A key exists.
     */
    fun hasPhaseAKey(): Boolean {
        return try {
            keyStore.containsAlias(PHASE_A_KEY_ALIAS)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to check Phase A key existence", e)
            false
        }
    }
}

// Extension function for hex encoding
private fun ByteArray.toHex(): String = joinToString("") { "%02X".format(it) }
