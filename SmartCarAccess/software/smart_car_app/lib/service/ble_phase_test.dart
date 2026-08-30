import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import 'gps_service.dart';
import 'pke_auth_orchestrator.dart';

export 'pke_auth_orchestrator.dart' show PhaseBResult;

typedef ProgressCallback = PkeAuthProgressCallback;

/// BLE Phase Test Service
///
/// Phase A informational helpers are retained for test screens.
/// Phase B execution now delegates to `PkeAuthOrchestrator` so UI and
/// background isolate use the same authentication path.
class BlePhaseTestService {
  final PkeAuthOrchestrator _authOrchestrator = PkeAuthOrchestrator();

  bool get hasActiveSession => _authOrchestrator.hasActiveSession;
  Uint8List? get sessionEncKey => _authOrchestrator.sessionEncKey;
  Uint8List? get sessionMacKey => _authOrchestrator.sessionMacKey;

  /// Test Phase A: NFC Provisioning
  ///
  /// This demonstrates what happens during NFC provisioning:
  /// 1. Phone (Android HCE) emulates a card with AID F0:01:02:03:04:05
  /// 2. ECU (ESP32 NFC reader) performs:
  ///    a. SELECT AID -> receives 4-byte UID + 9000
  ///    b. GET_CHALLENGE (Lc=0) -> receives [keyId(1) + phonePub(65) + certLen(2)=0 + 9000]
  ///    c. Generates challenge (vehicleId[8] + nonce[16] = 24 bytes)
  ///    d. GET_CHALLENGE (Lc=24, data=challenge) -> receives [sigLen(2,BE) + DER_sig + 9000]
  /// 3. ECU verifies signature and stores phone public key
  ///
  /// Android implementation: ProvisioningHostApduService.kt
  /// ESP32 implementation: provisioning_phase.cpp
  Future<PhaseAResult> testPhaseAInfo() async {
    debugPrint('=== Phase A: NFC Provisioning Test ===');

    debugPrint('Phase A is handled by native Android HCE service:');
    debugPrint('  Service: ProvisioningHostApduService');
    debugPrint('  AID: F0:01:02:03:04:05');
    debugPrint('');
    debugPrint('Provisioning Flow:');
    debugPrint('  1. ECU sends: 00 A4 04 00 06 F0 01 02 03 04 05 (SELECT AID)');
    debugPrint('     Phone responds: [UID(4)] 90 00');
    debugPrint('');
    debugPrint('  2. ECU sends: 00 CA 00 00 00 (GET_CHALLENGE, Lc=0)');
    debugPrint(
      '     Phone responds: [keyId(1) + phonePub(65) + certLen(2)=0000] 90 00',
    );
    debugPrint('');
    debugPrint('  3. ECU generates challenge: vehicleId(8) || nonce(16)');
    debugPrint(
      '     ECU sends: 00 CA 00 00 18 [challenge(24)] (GET_CHALLENGE, Lc=24)',
    );
    debugPrint('     Phone responds: [sigLen(2,BE) + DER_signature] 90 00');
    debugPrint('');
    debugPrint(
      '  4. ECU verifies signature with phone public key and stores it',
    );
    debugPrint('');
    debugPrint('Result: ECU is now provisioned with phone\'s public key');
    debugPrint('        Ready for Phase B (BLE authentication)');

    return PhaseAResult(
      success: true,
      message:
          'Phase A is handled by native HCE. See ProvisioningHostApduService.kt',
      phonePublicKeyStored: true,
    );
  }

  Future<PhaseBResult> testPhaseB({
    required String deviceAddress,
    BluetoothDevice? device,
    Duration timeout = const Duration(seconds: 30),
    ProgressCallback? onProgress,
  }) {
    return _authOrchestrator.authenticate(
      deviceAddress: deviceAddress,
      device: device,
      timeout: timeout,
      onProgress: onProgress,
    );
  }

  Future<void> disconnect() {
    return _authOrchestrator.disconnect();
  }

  Future<bool> sendGpsLocation() {
    return _authOrchestrator.sendGpsLocation();
  }

  Future<bool> sendGpsPacket(GpsDataPacket gpsPacket) {
    return _authOrchestrator.sendGpsPacket(gpsPacket);
  }
}

class PhaseAResult {
  final bool success;
  final String message;
  final bool phonePublicKeyStored;

  PhaseAResult({
    required this.success,
    required this.message,
    required this.phonePublicKeyStored,
  });

  @override
  String toString() {
    return 'PhaseA_Result{success: $success, message: $message, phonePublicKeyStored: $phonePublicKeyStored}';
  }
}
