import 'package:flutter/foundation.dart';

/// This file used to implement a Dart-side HCE flow with MethodChannel.
/// It is no longer used. The provisioning protocol is fully handled by
/// the native Android service `ProvisioningHostApduService`.
///
/// We keep a minimal stub to avoid breaking existing calls and to make
/// migration safe. All methods are no-ops or return informational strings.
class NfcProvisioningService {
  NfcProvisioningService({
    required this.ownerId,
  });

  final String ownerId; // preserved for compatibility

  static bool _initialized = false;

  /// No-op initializer retained for compatibility.
  static Future<void> initialize({String ownerIdHint = 'owner'}) async {
    if (_initialized) return;
    debugPrint('[HCE] Dart NfcProvisioningService is disabled. '
        'Provisioning is handled by native Android (ProvisioningHostApduService).');
    _initialized = true;
  }

  /// Test helper retained for UI diagnostics.
  /// Returns a message explaining that native HCE handles provisioning.
  static Future<String> testHceService() async {
    return 'Provisioning is handled by native Android HCE service '
        '(ProvisioningHostApduService). Dart NfcProvisioningService is disabled.';
  }

  /// No-op retained for compatibility; previously set a MethodChannel handler.
  void registerHceHandler() {
    // intentionally empty
  }

  /// No-op retained for compatibility with older UI flows that called this
  /// to ensure a keypair existed in secure storage. Native HCE now handles
  /// keys internally and this does nothing on Flutter side.
  Future<void> ensureOwnerKeysExist() async {
    debugPrint('[HCE] ensureOwnerKeysExist() ignored in Dart. '
        'Native ProvisioningHostApduService manages keys.');
  }
}
