package com.example.smart_car_app

object ByteArrayHexUtil {
    fun ByteArray.toHex(): String = joinToString("") { String.format("%02X", it) }
    fun hexToBytes(hex: String): ByteArray {
        val clean = hex.replace("[^0-9A-Fa-f]".toRegex(), "")
        require(clean.length % 2 == 0) { "Hex string must have even length" }
        return ByteArray(clean.length / 2) { i -> clean.substring(i * 2, i * 2 + 2).toInt(16).toByte() }
    }
}