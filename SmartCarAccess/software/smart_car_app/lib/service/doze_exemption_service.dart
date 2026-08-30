import 'dart:io';

import 'package:flutter/services.dart';

class DozeExemptionStatus {
  DozeExemptionStatus({
    required this.androidSdkInt,
    required this.supported,
    required this.ignored,
  });

  final int androidSdkInt;
  final bool supported;
  final bool ignored;

  bool get needsExemption => supported && !ignored;

  String toUserMessage() {
    if (!supported) {
      return 'Doze optimization controls are not required on this Android version.';
    }
    if (ignored) {
      return 'Battery optimization exemption is enabled for background reliability.';
    }
    return 'Battery optimization is enabled for this app and may delay background BLE detection.';
  }
}

class DozeExemptionService {
  static const MethodChannel _deviceInfoChannel = MethodChannel(
    'smartcar/device_info',
  );

  Future<DozeExemptionStatus> getStatus() async {
    if (!Platform.isAndroid) {
      return DozeExemptionStatus(
        androidSdkInt: 0,
        supported: false,
        ignored: true,
      );
    }

    try {
      final raw = await _deviceInfoChannel.invokeMethod<dynamic>(
        'getBatteryOptimizationStatus',
      );
      if (raw is Map) {
        final sdkInt = _asInt(raw['sdkInt']) ?? 0;
        final supported = _asBool(raw['supported']) ?? (sdkInt >= 23);
        final ignored = _asBool(raw['ignored']) ?? !supported;
        return DozeExemptionStatus(
          androidSdkInt: sdkInt,
          supported: supported,
          ignored: ignored,
        );
      }
    } catch (_) {
      // Fall through to conservative default.
    }

    return DozeExemptionStatus(
      androidSdkInt: 0,
      supported: true,
      ignored: false,
    );
  }

  Future<bool> requestExemption() async {
    if (!Platform.isAndroid) return false;
    try {
      final ok = await _deviceInfoChannel.invokeMethod<bool>(
        'requestIgnoreBatteryOptimizations',
      );
      return ok == true;
    } catch (_) {
      return false;
    }
  }

  Future<bool> openBatteryOptimizationSettings() async {
    if (!Platform.isAndroid) return false;
    try {
      final ok = await _deviceInfoChannel.invokeMethod<bool>(
        'openBatteryOptimizationSettings',
      );
      return ok == true;
    } catch (_) {
      return false;
    }
  }

  int? _asInt(dynamic value) {
    if (value is int) return value;
    if (value is num) return value.toInt();
    return null;
  }

  bool? _asBool(dynamic value) {
    if (value is bool) return value;
    return null;
  }
}
