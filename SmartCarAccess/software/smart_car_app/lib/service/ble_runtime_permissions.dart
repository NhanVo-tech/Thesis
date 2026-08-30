import 'dart:io';

import 'package:flutter/services.dart';
import 'package:permission_handler/permission_handler.dart';

class BleRuntimePermissionStatus {
  BleRuntimePermissionStatus({
    required this.androidSdkInt,
    required this.ready,
    required this.missing,
  });

  final int androidSdkInt;
  final bool ready;
  final List<String> missing;

  bool get isDegraded => !ready;

  String toUserMessage() {
    if (missing.isEmpty) {
      return 'Background BLE prerequisites are granted.';
    }
    return 'Background BLE is in degraded mode. Missing: ${missing.join(', ')}.';
  }
}

class BleRuntimePermissionService {
  static const MethodChannel _deviceInfoChannel = MethodChannel(
    'smartcar/device_info',
  );

  Future<BleRuntimePermissionStatus> ensureReady({
    bool requestIfNeeded = false,
  }) async {
    if (!Platform.isAndroid) {
      return BleRuntimePermissionStatus(
        androidSdkInt: 0,
        ready: true,
        missing: const <String>[],
      );
    }

    final sdkInt = await _getAndroidSdkInt();
    final required = _requiredPermissionsForSdk(sdkInt);

    final missingBefore = await _findMissing(required);
    if (requestIfNeeded && missingBefore.isNotEmpty) {
      for (final permission in missingBefore) {
        await permission.request();
      }
    }

    final missingAfter = await _findMissing(required);
    return BleRuntimePermissionStatus(
      androidSdkInt: sdkInt,
      ready: missingAfter.isEmpty,
      missing: missingAfter.map(_permissionLabel).toList(),
    );
  }

  Future<void> openSettings() async {
    await openAppSettings();
  }

  Future<int> _getAndroidSdkInt() async {
    try {
      final result = await _deviceInfoChannel.invokeMethod<int>(
        'getAndroidSdkInt',
      );
      return result ?? 0;
    } catch (_) {
      return 0;
    }
  }

  List<Permission> _requiredPermissionsForSdk(int sdkInt) {
    final permissions = <Permission>[];

    if (sdkInt >= 31) {
      permissions.addAll(const <Permission>[
        Permission.bluetoothScan,
        Permission.bluetoothConnect,
        Permission.bluetoothAdvertise,
      ]);
    } else {
      permissions.add(Permission.locationWhenInUse);
    }

    if (sdkInt >= 33) {
      permissions.add(Permission.notification);
    }

    return permissions;
  }

  Future<List<Permission>> _findMissing(List<Permission> permissions) async {
    final missing = <Permission>[];
    for (final permission in permissions) {
      final status = await permission.status;
      if (!status.isGranted) {
        missing.add(permission);
      }
    }
    return missing;
  }

  String _permissionLabel(Permission permission) {
    if (permission == Permission.bluetoothScan) {
      return 'bluetooth_scan';
    }
    if (permission == Permission.bluetoothConnect) {
      return 'bluetooth_connect';
    }
    if (permission == Permission.bluetoothAdvertise) {
      return 'bluetooth_advertise';
    }
    if (permission == Permission.locationWhenInUse) {
      return 'location_when_in_use';
    }
    if (permission == Permission.notification) {
      return 'post_notifications';
    }
    return permission.toString();
  }
}
