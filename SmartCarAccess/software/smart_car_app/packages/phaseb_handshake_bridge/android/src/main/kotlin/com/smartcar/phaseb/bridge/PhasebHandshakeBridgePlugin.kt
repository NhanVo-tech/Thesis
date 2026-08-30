package com.smartcar.phaseb.bridge

import android.security.keystore.KeyProperties
import android.util.Log
import io.flutter.embedding.engine.plugins.FlutterPlugin
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import java.math.BigInteger
import java.security.KeyFactory
import java.security.KeyPair
import java.security.KeyPairGenerator
import java.security.KeyStore
import java.security.PublicKey
import java.security.SecureRandom
import java.security.Signature
import java.security.interfaces.ECPublicKey
import java.security.spec.ECGenParameterSpec
import java.security.spec.ECPoint
import java.security.spec.ECPublicKeySpec
import javax.crypto.KeyAgreement
import javax.crypto.Mac
import javax.crypto.spec.SecretKeySpec

class PhasebHandshakeBridgePlugin : FlutterPlugin, MethodChannel.MethodCallHandler {
    companion object {
        private const val TAG = "PhasebHandshakePlugin"
        private const val CHANNEL_NAME = "smartcar/phaseb/handshake"
        private const val KEYSTORE_PROVIDER = "AndroidKeyStore"
        private const val PHASE_A_KEY_ALIAS = "smart_car_phone_identity_p256"
        private var ephemeralKeyPair: KeyPair? = null
    }

    private lateinit var channel: MethodChannel

    override fun onAttachedToEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        channel = MethodChannel(binding.binaryMessenger, CHANNEL_NAME)
        channel.setMethodCallHandler(this)
        Log.i(TAG, "Attached and registered $CHANNEL_NAME")
    }

    override fun onDetachedFromEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        channel.setMethodCallHandler(null)
    }

    override fun onMethodCall(call: MethodCall, result: MethodChannel.Result) {
        try {
            when (call.method) {
                "generateEphemeralKeypair" -> {
                    val keypair = generateEphemeralKeypair()
                    if (keypair == null) {
                        result.error("KEYGEN_FAILED", "Failed to generate ephemeral keypair", null)
                    } else {
                        result.success(mapOf("publicKey" to keypair.first, "privateKey" to keypair.second))
                    }
                }
                "getEphemeralPublicKey" -> {
                    val pub = getEphemeralPublicKey()
                    if (pub == null) {
                        result.error("NO_KEYPAIR", "No ephemeral keypair available", null)
                    } else {
                        result.success(pub)
                    }
                }
                "signEphemeralWithIdentity" -> {
                    val data = call.arguments as? ByteArray
                    if (data == null) {
                        result.error("INVALID_ARGS", "Ephemeral public key required", null)
                        return
                    }
                    val sig = signWithIdentityKey(data)
                    if (sig == null) {
                        result.error("SIGN_FAILED", "Failed to sign ephemeral key", null)
                    } else {
                        result.success(sig)
                    }
                }
                "computeECDH" -> {
                    @Suppress("UNCHECKED_CAST")
                    val args = call.arguments as? Map<String, ByteArray>
                    val ecuPublicKey = args?.get("ecuPublicKey")
                    if (ecuPublicKey == null) {
                        result.error("INVALID_ARGS", "ECU public key required", null)
                        return
                    }
                    val shared = computeECDH(ecuPublicKey)
                    if (shared == null) {
                        result.error("ECDH_FAILED", "Failed to compute shared secret", null)
                    } else {
                        result.success(shared)
                    }
                }
                "deriveSessionKeys" -> {
                    @Suppress("UNCHECKED_CAST")
                    val args = call.arguments as? Map<String, Any>
                    val sharedSecret = args?.get("sharedSecret") as? ByteArray
                    val phoneEphemeralPub = args?.get("phoneEphemeralPub") as? ByteArray
                    val ecuEphemeralPub = args?.get("ecuEphemeralPub") as? ByteArray
                    if (sharedSecret == null || phoneEphemeralPub == null || ecuEphemeralPub == null) {
                        result.error("INVALID_ARGS", "All key material required", null)
                        return
                    }
                    val keys = deriveSessionKeys(sharedSecret, phoneEphemeralPub, ecuEphemeralPub)
                    if (keys == null) {
                        result.error("HKDF_FAILED", "Failed to derive session keys", null)
                    } else {
                        result.success(mapOf("encKey" to keys.first, "macKey" to keys.second))
                    }
                }
                "signChallenge" -> {
                    val challenge = call.arguments as? ByteArray
                    if (challenge == null) {
                        result.error("INVALID_ARGS", "Challenge data required", null)
                        return
                    }
                    val sig = signWithIdentityKey(challenge)
                    if (sig == null) {
                        result.error("SIGN_FAILED", "Failed to sign challenge", null)
                    } else {
                        result.success(sig)
                    }
                }
                "resetEphemeralKeys" -> {
                    ephemeralKeyPair = null
                    result.success(true)
                }
                else -> result.notImplemented()
            }
        } catch (e: Throwable) {
            Log.e(TAG, "Channel error in ${call.method}", e)
            result.error("CHANNEL_ERROR", e.message, null)
        }
    }

    private fun generateEphemeralKeypair(): Pair<ByteArray, ByteArray>? {
        return try {
            val keyPairGenerator = KeyPairGenerator.getInstance("EC")
            keyPairGenerator.initialize(ECGenParameterSpec("secp256r1"), SecureRandom())
            ephemeralKeyPair = keyPairGenerator.generateKeyPair()
            val publicKey = ephemeralKeyPair!!.public as ECPublicKey
            Pair(ecPublicKeyToUncompressed(publicKey), ephemeralKeyPair!!.private.encoded)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to generate ephemeral keypair", e)
            null
        }
    }

    private fun getEphemeralPublicKey(): ByteArray? {
        val pair = ephemeralKeyPair ?: return null
        return try {
            ecPublicKeyToUncompressed(pair.public as ECPublicKey)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to get ephemeral public key", e)
            null
        }
    }

    private fun signWithIdentityKey(data: ByteArray): ByteArray? {
        return try {
            val keyStore = KeyStore.getInstance(KEYSTORE_PROVIDER).apply { load(null) }
            val entry = keyStore.getEntry(PHASE_A_KEY_ALIAS, null) as? KeyStore.PrivateKeyEntry
            if (entry == null) {
                Log.e(TAG, "Phase A key alias not found: $PHASE_A_KEY_ALIAS")
                return null
            }
            val signature = Signature.getInstance("SHA256withECDSA")
            signature.initSign(entry.privateKey)
            signature.update(data)
            signature.sign()
        } catch (e: Exception) {
            Log.e(TAG, "Failed to sign with identity key", e)
            null
        }
    }

    private fun computeECDH(ecuPublicKey65: ByteArray): ByteArray? {
        return try {
            val pair = ephemeralKeyPair ?: return null
            if (ecuPublicKey65.size != 65 || ecuPublicKey65[0] != 0x04.toByte()) {
                return null
            }
            val ecuPublicKey = uncompressedToECPublicKey(ecuPublicKey65)
            val keyAgreement = KeyAgreement.getInstance("ECDH")
            keyAgreement.init(pair.private)
            keyAgreement.doPhase(ecuPublicKey, true)
            val sharedSecret = keyAgreement.generateSecret()
            when {
                sharedSecret.size == 32 -> sharedSecret
                sharedSecret.size > 32 -> sharedSecret.copyOfRange(sharedSecret.size - 32, sharedSecret.size)
                else -> ByteArray(32).also { System.arraycopy(sharedSecret, 0, it, 32 - sharedSecret.size, sharedSecret.size) }
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to compute ECDH", e)
            null
        }
    }

    private fun deriveSessionKeys(
        sharedSecret: ByteArray,
        phoneEphemeralPub: ByteArray,
        ecuEphemeralPub: ByteArray
    ): Pair<ByteArray, ByteArray>? {
        return try {
            val labelEnc = ByteArray(16)
            "SmartCarv1|ENC".toByteArray().copyInto(labelEnc, 0, 0, 14)
            val infoEnc = labelEnc + ecuEphemeralPub + phoneEphemeralPub
            val encKey = hkdfSha256(sharedSecret, ByteArray(0), infoEnc, 32)

            val labelMac = ByteArray(16)
            "SmartCarv1|MAC".toByteArray().copyInto(labelMac, 0, 0, 14)
            val infoMac = labelMac + ecuEphemeralPub + phoneEphemeralPub
            val macKey = hkdfSha256(sharedSecret, ByteArray(0), infoMac, 32)

            Pair(encKey, macKey)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to derive session keys", e)
            null
        }
    }

    private fun ecPublicKeyToUncompressed(publicKey: ECPublicKey): ByteArray {
        val point = publicKey.w
        val x = point.affineX.toByteArray()
        val y = point.affineY.toByteArray()
        val xBytes = if (x.size > 32) x.copyOfRange(x.size - 32, x.size) else ByteArray(32).also {
            System.arraycopy(x, 0, it, 32 - x.size, x.size)
        }
        val yBytes = if (y.size > 32) y.copyOfRange(y.size - 32, y.size) else ByteArray(32).also {
            System.arraycopy(y, 0, it, 32 - y.size, y.size)
        }
        val out = ByteArray(65)
        out[0] = 0x04
        System.arraycopy(xBytes, 0, out, 1, 32)
        System.arraycopy(yBytes, 0, out, 33, 32)
        return out
    }

    private fun uncompressedToECPublicKey(bytes: ByteArray): PublicKey {
        val x = BigInteger(1, bytes.copyOfRange(1, 33))
        val y = BigInteger(1, bytes.copyOfRange(33, 65))
        val point = ECPoint(x, y)

        val keyFactory = KeyFactory.getInstance("EC")
        val params = (KeyPairGenerator.getInstance("EC").apply {
            initialize(ECGenParameterSpec("secp256r1"))
        }.generateKeyPair().public as ECPublicKey).params

        return keyFactory.generatePublic(ECPublicKeySpec(point, params))
    }

    private fun hkdfSha256(
        ikm: ByteArray,
        salt: ByteArray,
        info: ByteArray,
        length: Int
    ): ByteArray {
        val actualSalt = if (salt.isEmpty()) ByteArray(32) else salt
        val prk = hmacSha256(actualSalt, ikm)

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

    private fun hmacSha256(key: ByteArray, data: ByteArray): ByteArray {
        val mac = Mac.getInstance("HmacSHA256")
        mac.init(SecretKeySpec(key, "HmacSHA256"))
        return mac.doFinal(data)
    }
}
