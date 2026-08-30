import 'dart:async';
import 'dart:convert';
import 'package:flutter/services.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import 'package:nfc_manager/nfc_manager.dart';

class MasterCardPayload {
  MasterCardPayload({required this.vehicleId, required this.masterSecret});

  final Uint8List vehicleId; // 8 bytes
  final Uint8List masterSecret; // 32 bytes

  String get vehicleIdHex => _bytesToHex(vehicleId);
  String get masterSecretHex => _bytesToHex(masterSecret);

  static String _bytesToHex(Uint8List bytes) {
    final sb = StringBuffer();
    for (final b in bytes) {
      sb.write(b.toRadixString(16).padLeft(2, '0').toUpperCase());
    }
    return sb.toString();
  }
}

class ProvisioningVehicleBinding {
  ProvisioningVehicleBinding({
    required this.vehicleId,
    required this.vehiclePubKey,
    required this.devicePubKey,
    this.writeDataPayload,
    this.updatedAtMs,
  });

  final Uint8List vehicleId;
  final Uint8List vehiclePubKey;
  final Uint8List devicePubKey;
  final Uint8List? writeDataPayload;
  final int? updatedAtMs;
}

/// Master card provisioning helper.
/// - Reads NDEF text payload from master card
/// - Parses JSON {"vid":"...","msk":"..."}
/// - Sends session data to Android HCE service (in-memory only)
class MasterCardProvisioningService {
  static const MethodChannel _channel = MethodChannel('smartcar/mastercard');
  static const MethodChannel _readerChannel = MethodChannel(
    'smartcar/nfc_reader',
  );
  static const String _pendingPrefix = 'pending_mastercard_';
  static final FlutterSecureStorage _secureStorage = FlutterSecureStorage();
  Completer<MasterCardPayload>? _activeCompleter;

  Future<MasterCardPayload> readMasterCard({
    Duration timeout = const Duration(seconds: 30),
  }) async {
    final completer = Completer<MasterCardPayload>();
    _activeCompleter = completer;

    // Enable reader mode to suppress system UI while scanning
    await _readerChannel.invokeMethod('enableReaderMode');

    await NfcManager.instance.startSession(
      onDiscovered: (tag) async {
        try {
          final ndef = Ndef.from(tag);
          if (ndef == null) {
            throw StateError('NDEF not supported on this tag');
          }
          final message = ndef.cachedMessage;
          if (message == null || message.records.isEmpty) {
            throw StateError('Empty NDEF message');
          }

          final text = _extractPayloadText(message.records);
          final payload = _parsePayload(text);

          completer.complete(payload);
        } catch (e) {
          completer.completeError(e);
        } finally {
          await NfcManager.instance.stopSession();
          _activeCompleter = null;
        }
      },
      onError: (error) async {
        if (!completer.isCompleted) {
          completer.completeError(error);
        }
        await NfcManager.instance.stopSession();
        _activeCompleter = null;
      },
    );

    return completer.future.timeout(
      timeout,
      onTimeout: () async {
        await NfcManager.instance.stopSession();
        _activeCompleter = null;
        throw TimeoutException('Timed out waiting for master card');
      },
    );
  }

  Future<void> cancelReadMasterCard() async {
    final completer = _activeCompleter;
    if (completer != null && !completer.isCompleted) {
      completer.completeError(StateError('Master card scan cancelled'));
    }
    _activeCompleter = null;
    await NfcManager.instance.stopSession();
  }

  Future<void> savePendingPayload(
    String carId,
    MasterCardPayload payload,
  ) async {
    final data = jsonEncode({
      'vid': payload.vehicleIdHex,
      'msk': payload.masterSecretHex,
    });
    await _secureStorage.write(key: '$_pendingPrefix$carId', value: data);
  }

  Future<MasterCardPayload?> loadPendingPayload(String carId) async {
    final raw = await _secureStorage.read(key: '$_pendingPrefix$carId');
    if (raw == null) return null;
    try {
      return _parsePayload(raw);
    } catch (_) {
      return null;
    }
  }

  Future<void> clearPendingPayload(String carId) async {
    await _secureStorage.delete(key: '$_pendingPrefix$carId');
  }

  Future<void> activateHceSession(
    MasterCardPayload payload, {
    Duration ttl = const Duration(seconds: 60),
  }) async {
    await _channel.invokeMethod('setMasterSession', {
      'vehicleId': payload.vehicleId,
      'masterSecret': payload.masterSecret,
      'ttlSeconds': ttl.inSeconds,
    });
  }

  Future<void> clearHceSession() async {
    await _channel.invokeMethod('clearMasterSession');
  }

  Future<bool> isHceSessionActive() async {
    final result = await _channel.invokeMethod('isMasterSessionActive');
    return result == true;
  }

  Future<bool> clearProvisioningVehicleBinding() async {
    final result = await _channel.invokeMethod('clearProvisioningVehicleBinding');
    return result == true;
  }

  Future<ProvisioningVehicleBinding?> getProvisioningVehicleBinding() async {
    final raw = await _channel.invokeMethod('getProvisioningVehicleBinding');
    if (raw is! Map) return null;

    final vehicleId = _asBytes(raw['vehicleId']);
    final vehiclePubKey = _asBytes(raw['vehiclePubKey']);
    final devicePubKey = _asBytes(raw['devicePubKey']);
    final writeDataPayload = _asBytes(raw['writeDataPayload']);
    final updatedAtMs = _asInt(raw['updatedAtMs']);
    if (vehicleId == null || vehiclePubKey == null || devicePubKey == null) {
      return null;
    }
    if (vehicleId.length != 8 ||
        vehiclePubKey.length != 65 ||
        devicePubKey.length != 65) {
      return null;
    }
    if (writeDataPayload != null && writeDataPayload.length != 77) {
      return null;
    }

    return ProvisioningVehicleBinding(
      vehicleId: vehicleId,
      vehiclePubKey: vehiclePubKey,
      devicePubKey: devicePubKey,
      writeDataPayload: writeDataPayload,
      updatedAtMs: updatedAtMs,
    );
  }

  bool validateBindingConsistency(
    ProvisioningVehicleBinding binding, {
    Uint8List? expectedVehicleId,
  }) {
    if (binding.vehicleId.length != 8 ||
        binding.vehiclePubKey.length != 65 ||
        binding.devicePubKey.length != 65) {
      return false;
    }

    if (expectedVehicleId != null && !_bytesEqual(binding.vehicleId, expectedVehicleId)) {
      return false;
    }

    final payload = binding.writeDataPayload;
    if (payload != null) {
      final expectedPayload = buildWriteDataPayload(
        vehicleId: binding.vehicleId,
        vehiclePubKey: binding.vehiclePubKey,
      );
      if (!_bytesEqual(payload, expectedPayload)) {
        return false;
      }
    }

    return true;
  }

  Uint8List buildWriteDataPayload({
    required Uint8List vehicleId,
    required Uint8List vehiclePubKey,
  }) {
    if (vehicleId.length != 8) {
      throw ArgumentError('vehicleId must be 8 bytes');
    }
    if (vehiclePubKey.length != 65) {
      throw ArgumentError('vehiclePubKey must be 65 bytes');
    }

    final payload = Uint8List(77);
    var offset = 0;
    payload[offset++] = 0x80;
    payload[offset++] = 0x08;
    payload.setRange(offset, offset + vehicleId.length, vehicleId);
    offset += vehicleId.length;
    payload[offset++] = 0x81;
    payload[offset++] = 0x41;
    payload.setRange(offset, offset + vehiclePubKey.length, vehiclePubKey);
    return payload;
  }

  String _extractPayloadText(List<NdefRecord> records) {
    if (records.isEmpty) {
      throw FormatException('Empty NDEF message');
    }

    // Prefer a well-known Text record when available
    for (final record in records) {
      if (record.typeNameFormat == NdefTypeNameFormat.nfcWellknown &&
          record.type.isNotEmpty &&
          String.fromCharCodes(record.type) == 'T') {
        return _decodeTextRecord(record);
      }
    }

    // Fallback: try to decode the first record payload as UTF-8
    final fallback = records.first.payload;
    if (fallback.isEmpty) {
      throw FormatException('Empty NDEF payload');
    }
    return utf8.decode(fallback);
  }

  String _decodeTextRecord(NdefRecord record) {
    final payload = record.payload;
    if (payload.isEmpty) {
      throw FormatException('Empty NDEF payload');
    }

    final status = payload.first;
    final langLen = status & 0x3F;
    final textStart = 1 + langLen;
    if (textStart > payload.length) {
      throw FormatException('Invalid NDEF text payload');
    }

    return utf8.decode(payload.sublist(textStart));
  }

  MasterCardPayload _parsePayload(String text) {
    String? vidHex;
    String? mskHex;

    try {
      final obj = jsonDecode(text);
      if (obj is Map) {
        vidHex = obj['vid']?.toString();
        mskHex = obj['msk']?.toString();
      }
    } catch (_) {
      // Fallback to CSV: vid,msk
      final parts = text.split(',');
      if (parts.length >= 2) {
        vidHex = parts[0].trim();
        mskHex = parts[1].trim();
      }
    }

    if (vidHex == null || mskHex == null) {
      throw FormatException('Master card payload missing vid/msk');
    }

    final vidBytes = _hexToBytes(vidHex);
    final mskBytes = _hexToBytes(mskHex);

    if (vidBytes.length != 8) {
      throw FormatException('Vehicle ID must be 8 bytes (16 hex chars)');
    }
    if (mskBytes.length != 32) {
      throw FormatException('Master Secret must be 32 bytes (64 hex chars)');
    }

    return MasterCardPayload(vehicleId: vidBytes, masterSecret: mskBytes);
  }

  Uint8List _hexToBytes(String hex) {
    final clean = hex.replaceAll(RegExp(r'[^0-9a-fA-F]'), '');
    if (clean.length.isOdd) {
      throw FormatException('Hex string must have even length');
    }
    final out = Uint8List(clean.length ~/ 2);
    for (var i = 0; i < clean.length; i += 2) {
      out[i ~/ 2] = int.parse(clean.substring(i, i + 2), radix: 16);
    }
    return out;
  }

  Uint8List? _asBytes(dynamic value) {
    if (value is Uint8List) return value;
    if (value is List<dynamic>) {
      try {
        return Uint8List.fromList(value.cast<int>());
      } catch (_) {
        return null;
      }
    }
    return null;
  }

  int? _asInt(dynamic value) {
    if (value is int) return value;
    if (value is num) return value.toInt();
    return null;
  }

  bool _bytesEqual(Uint8List a, Uint8List b) {
    if (a.length != b.length) return false;
    for (var i = 0; i < a.length; i++) {
      if (a[i] != b[i]) return false;
    }
    return true;
  }
}
