package com.equinox.uwb;

import android.ranging.RangingData
import android.ranging.RangingDevice
import android.ranging.RangingManager
import android.ranging.RangingPreference
import android.ranging.RangingSession
import android.ranging.SensorFusionParams
import android.ranging.SessionConfig
import android.ranging.raw.RawInitiatorRangingConfig
import android.ranging.raw.RawRangingDevice
import android.ranging.uwb.UwbAddress
import android.ranging.uwb.UwbComplexChannel
import android.ranging.uwb.UwbRangingParams
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.MutableState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import java.util.UUID
import java.util.concurrent.Executors

// Id and position of an anchor. The id must match the responder address set on the DWM3001CDK.
private data class UwbAnchor(val id: Int, val xMeters: Double, val yMeters: Double)

// A distance measurement from an anchor.
data class AnchorSample(val x: Double, val y: Double, val d: Double)

// Hardcoded list of anchors and positions.
private val uwbAnchors = listOf(
    UwbAnchor(0, 0.0, 0.0),
    UwbAnchor(1, 5.0, 0.0),
    UwbAnchor(2, 0.0, 5.0),
)

// Local (phone) address. Can be any value, but must match the peer (initiator) address set on the DWM3001CDK.
private const val LOCAL_ADDRESS = 1729

private val rawInitiatorRangingConfig = RawInitiatorRangingConfig.Builder()
    .addRawRangingDevices((0..<uwbAnchors.size).map { peerIdx ->
        RawRangingDevice.Builder()
            .setRangingDevice(
                RangingDevice.Builder()
                    .setUuid(UUID(0, peerIdx.toLong()))
                    .build()
            )
            .setUwbRangingParams(
                UwbRangingParams.Builder(
                    42,
                    UwbRangingParams.CONFIG_MULTICAST_DS_TWR,
                    UwbAddress.fromBytes(byteArrayOf(
                        (LOCAL_ADDRESS and 0xFF).toByte(),
                        ((LOCAL_ADDRESS shr 8) and 0xFF).toByte(),
                    )),
                    UwbAddress.fromBytes(byteArrayOf(peerIdx.toByte(), 0)),
                )
                    .setComplexChannel(
                        UwbComplexChannel.Builder()
                            .setChannel(UwbComplexChannel.UWB_CHANNEL_9)
                            .setPreambleIndex(UwbComplexChannel.UWB_PREAMBLE_CODE_INDEX_9)
                            .build()
                    )
                    .setRangingUpdateRate(RawRangingDevice.UPDATE_RATE_FREQUENT)
                    .setSessionKeyInfo(byteArrayOf(0x08, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06))
                    .build()
            )
            .build()
    }).build()

private val sessionConfig = SessionConfig.Builder()
    .setSensorFusionParams(SensorFusionParams.Builder().setSensorFusionEnabled(true).build())
    .setAngleOfArrivalNeeded(true) // optional: if you need the azimuth (unreliable in my experience)
    .build()

private val rangingPreference =
    RangingPreference.Builder(RangingPreference.DEVICE_ROLE_INITIATOR, rawInitiatorRangingConfig)
        .setSessionConfig(sessionConfig)
        .build()

private fun createRangingSession(
    rangingManager: RangingManager,
    positionTracker: PositionTracker,
    statusMessage: MutableState<String>,
): RangingSession? {
    return rangingManager.createRangingSession(
        Executors.newSingleThreadExecutor(),
        object : RangingSession.Callback {
            override fun onClosed(reason: Int) {
                statusMessage.value = "Session closed (reason: $reason)"
            }

            override fun onOpenFailed(reason: Int) {
                statusMessage.value = "Session failed to open (reason: $reason)"
            }

            override fun onOpened() {
                statusMessage.value = "Session opened"
            }

            override fun onResults(peer: RangingDevice, data: RangingData) {
                val peerIdx = peer.uuid.leastSignificantBits.toInt()
                if (peerIdx !in  0..<uwbAnchors.size) return

                data.distance?.let { distance ->
                    positionTracker.update(peerIdx, distance.measurement)
                }
            }

            override fun onStarted(peer: RangingDevice, technology: Int) {
                statusMessage.value = "Session started"
            }

            override fun onStopped(peer: RangingDevice, technology: Int) {
                statusMessage.value = "Session stopped"
            }
        }
    )
}

private class PositionTracker {
    val distancesMeters = MutableList(uwbAnchors.size) { Double.NaN }

    var tagPositionXYMeters by mutableStateOf(Pair(Double.NaN, Double.NaN))
        private set

    fun update(anchorIdx: Int, distanceMeters: Double) {
        distancesMeters[anchorIdx] = distanceMeters

        // Create anchor samples for each anchor with a valid measurement
        val anchorSamples = distancesMeters.mapIndexedNotNull { anchorIdx, anchorDistanceMeters ->
            if (anchorDistanceMeters.isNaN()) return@mapIndexedNotNull null
            val anchor = uwbAnchors[anchorIdx]
            AnchorSample(anchor.xMeters, anchor.yMeters, anchorDistanceMeters)
        }

        tagPositionXYMeters = estimateTagPosition(anchorSamples)
    }
}

@Composable
fun EstimateUwbLocation(rangingManager: RangingManager) {
    val statusMessage = remember { mutableStateOf("Not started") }
    val positionTracker = remember { PositionTracker() }

    LaunchedEffect(Unit) {
        createRangingSession(rangingManager, positionTracker, statusMessage)?.start(rangingPreference)
    }

    Column(modifier = Modifier.padding(start = 16.dp), verticalArrangement = Arrangement.spacedBy(16.dp)) {
        Text(text = "Status: ${statusMessage.value}")

        Column {
            positionTracker.distancesMeters.forEachIndexed { anchorIdx, distanceMeters ->
                Text(text = "Anchor %d distance: %.2f m".format(anchorIdx, distanceMeters))
            }
        }

        Text(
            text = "Estimated position: (%.2f, %.2f) m".format(
                positionTracker.tagPositionXYMeters.first,
                positionTracker.tagPositionXYMeters.second
            )
        )
    }
}
