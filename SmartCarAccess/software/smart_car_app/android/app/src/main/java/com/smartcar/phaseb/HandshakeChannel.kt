package com.smartcar.phaseb

import android.util.Log
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.BinaryMessenger
import io.flutter.plugin.common.MethodChannel

/**
 * HandshakeChannel for Phase B (BLE Authentication)
 * 
 * Provides Flutter-to-Kotlin bridge for Phase B handshake operations:
 * - Generate ephemeral keypair
 * - Sign ephemeral public key with identity key
 * - Compute ECDH shared secret
 * - Derive session keys with HKDF
 * - Sign challenge for final verification
 */
object HandshakeChannel {
    private const val TAG = "HandshakeChannel"
    private const val CHANNEL_NAME = "smartcar/phaseb/handshake"
    
    /**
     * Register Phase B handshake channel with Flutter engine.
     */
    fun register(flutterEngine: FlutterEngine) {
        register(flutterEngine.dartExecutor.binaryMessenger)
    }

    /**
     * Register Phase B handshake channel with a binary messenger.
     * This supports both the UI engine and the headless background engine.
     */
    fun register(messenger: BinaryMessenger) {
        MethodChannel(
            messenger,
            CHANNEL_NAME
        ).setMethodCallHandler { call, result ->
            try {
                when (call.method) {
                    "generateEphemeralKeypair" -> {
                        // Generate ephemeral P-256 keypair for this session
                        val keypair = PhaseBCrypto.generateEphemeralKeypair()
                        if (keypair != null) {
                            result.success(mapOf(
                                "publicKey" to keypair.first,
                                "privateKey" to keypair.second
                            ))
                        } else {
                            result.error("KEYGEN_FAILED", "Failed to generate ephemeral keypair", null)
                        }
                    }
                    
                    "getEphemeralPublicKey" -> {
                        // Get current ephemeral public key (65 bytes uncompressed)
                        val pubKey = PhaseBCrypto.getEphemeralPublicKey()
                        if (pubKey != null) {
                            result.success(pubKey)
                        } else {
                            result.error("NO_KEYPAIR", "No ephemeral keypair available", null)
                        }
                    }
                    
                    "signEphemeralWithIdentity" -> {
                        // Sign ephemeral public key with Android Keystore identity key
                        val ephemeralPubKey = call.arguments as? ByteArray
                        if (ephemeralPubKey == null) {
                            result.error("INVALID_ARGS", "Ephemeral public key required", null)
                            return@setMethodCallHandler
                        }
                        
                        val signature = PhaseBCrypto.signWithIdentityKey(ephemeralPubKey)
                        if (signature != null) {
                            result.success(signature)
                        } else {
                            result.error("SIGN_FAILED", "Failed to sign ephemeral key", null)
                        }
                    }
                    
                    "computeECDH" -> {
                        // Compute ECDH shared secret
                        @Suppress("UNCHECKED_CAST")
                        val args = call.arguments as? Map<String, ByteArray>
                        val ecuPublicKey = args?.get("ecuPublicKey")
                        
                        if (ecuPublicKey == null) {
                            result.error("INVALID_ARGS", "ECU public key required", null)
                            return@setMethodCallHandler
                        }
                        
                        val sharedSecret = PhaseBCrypto.computeECDH(ecuPublicKey)
                        if (sharedSecret != null) {
                            result.success(sharedSecret)
                        } else {
                            result.error("ECDH_FAILED", "Failed to compute shared secret", null)
                        }
                    }
                    
                    "deriveSessionKeys" -> {
                        // Derive session keys using HKDF-SHA256
                        @Suppress("UNCHECKED_CAST")
                        val args = call.arguments as? Map<String, Any>
                        val sharedSecret = args?.get("sharedSecret") as? ByteArray
                        val phoneEphemeralPub = args?.get("phoneEphemeralPub") as? ByteArray
                        val ecuEphemeralPub = args?.get("ecuEphemeralPub") as? ByteArray
                        
                        if (sharedSecret == null || phoneEphemeralPub == null || ecuEphemeralPub == null) {
                            result.error("INVALID_ARGS", "All key material required", null)
                            return@setMethodCallHandler
                        }
                        
                        val keys = PhaseBCrypto.deriveSessionKeys(
                            sharedSecret,
                            phoneEphemeralPub,
                            ecuEphemeralPub
                        )
                        
                        if (keys != null) {
                            result.success(mapOf(
                                "encKey" to keys.first,
                                "macKey" to keys.second
                            ))
                        } else {
                            result.error("HKDF_FAILED", "Failed to derive session keys", null)
                        }
                    }
                    
                    "signChallenge" -> {
                        // Sign challenge with identity key for final verification
                        val challenge = call.arguments as? ByteArray
                        if (challenge == null) {
                            result.error("INVALID_ARGS", "Challenge data required", null)
                            return@setMethodCallHandler
                        }
                        
                        val signature = PhaseBCrypto.signChallenge(challenge)
                        if (signature != null) {
                            result.success(signature)
                        } else {
                            result.error("SIGN_FAILED", "Failed to sign challenge", null)
                        }
                    }
                    
                    "resetEphemeralKeys" -> {
                        // Clear ephemeral keys (on disconnect)
                        PhaseBCrypto.resetEphemeralKeys()
                        result.success(true)
                    }
                    
                    else -> {
                        result.notImplemented()
                    }
                }
            } catch (e: Exception) {
                Log.e(TAG, "HandshakeChannel error: ${call.method}", e)
                result.error("CHANNEL_ERROR", e.message, e.stackTraceToString())
            }
        }
        
        Log.i(TAG, "Phase B handshake channel registered: $CHANNEL_NAME")
    }
}
