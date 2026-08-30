import 'dart:async';
import 'dart:io';
import 'dart:ui';
import 'dart:typed_data';

import 'package:flutter/widgets.dart';
import 'package:flutter_background_service/flutter_background_service.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'nfc_provisioning_service.dart';
import 'pke_auth_orchestrator.dart';
import 'pke_rollout_flags.dart';
import 'pke_telemetry.dart';
import 'uwb_service.dart';

class PkeBackgroundService {
  static const String _channelId = 'pke_background_runtime';
  static const String _channelName = 'PKE Background Runtime';
  static const String _channelDescription =
      'Maintains authentication orchestration in background mode.';
  static const int _notificationId = 7411;

  static final FlutterBackgroundService _service = FlutterBackgroundService();
  static bool _configured = false;
  static bool _foregroundListenerConfigured = false;
  static bool _foregroundHandoffInFlight = false;
  static UwbService? _foregroundUwb;

  static void _log(String message) {
    debugPrint('[PKE][BG] $message');
  }

  static Future<void> ensureForRollout(PkeRolloutFlags flags) async {
    if (!Platform.isAndroid) {
      _log('ensureForRollout skipped on non-Android platform');
      return;
    }

    await _ensureConfigured();

    final running = await _service.isRunning();
    _log(
      'ensureForRollout backgroundMode=${flags.backgroundMode ? 1 : 0} running=${running ? 1 : 0}',
    );

    if (!running) {
      _log('starting foreground background-service');
      await _service.startService();
    } else {
      _log('background-service already running');
    }
  }

  static void registerForegroundHandoffListener() {
    if (!Platform.isAndroid || _foregroundListenerConfigured) {
      return;
    }

    _foregroundListenerConfigured = true;
    _foregroundUwb ??= UwbService(enableEventChannel: true, logToConsole: true);

    FlutterBackgroundService()
      .on('runUwbHandoff')
      .listen((event) {
      final payload = event is Map<dynamic, dynamic> ? event : null;
      final deviceAddress = payload?['deviceAddress']?.toString().trim();
      final requestId = payload?['requestId']?.toString();

      if (deviceAddress == null || deviceAddress.isEmpty) {
        _log('foreground handoff ignored: missing deviceAddress');
        return;
      }

      if (_foregroundHandoffInFlight) {
        _log('foreground handoff ignored: another handoff in flight');
        return;
      }

      _foregroundHandoffInFlight = true;
      unawaited(_handleForegroundHandoff(deviceAddress, requestId));
    });

    FlutterBackgroundService().on('stopUwbHandoff').listen((event) {
      final payload = event is Map<dynamic, dynamic> ? event : null;
      final requestId = payload?['requestId']?.toString();
      unawaited(_handleForegroundHandoffStop(requestId));
    });
  }

  static Future<void> _handleForegroundHandoff(
    String deviceAddress,
    String? requestId,
  ) async {
    final service = FlutterBackgroundService();
    final uwb = _foregroundUwb;
    bool ok = false;

    if (uwb == null) {
      _log('foreground handoff failed: UWB service not initialized');
      service.invoke('uwbHandoffResult', {
        'requestId': requestId,
        'ok': false,
        'error': 'uwb_unavailable',
      });
      _foregroundHandoffInFlight = false;
      return;
    }

    try {
      final targetAddress = deviceAddress.trim().toUpperCase();
      _log('foreground handoff start target=$targetAddress');

      // Skip the 8-second scan. Re-use the known MAC address.
      // Because the background isolate is holding the connection open, 
      // the OS will instantly resolve this connect request.
      final targetDevice = BluetoothDevice.fromId(targetAddress);
      
      await uwb.connect(targetDevice);

      final payload = uwb.buildDefaultOobPayloadV1(
        role: UwbOobPayload.carRoleControlee,
        sessionId: DateTime.now().millisecondsSinceEpoch & 0xFFFFFFFF,
        phoneMac: 0x0001,
        carMac: 0x0002,
        channel: 9,
      );

      final prepared = await uwb.preparePayloadForOob(payload);
      final preparedPayload = prepared['payload'] as Uint8List?;
      if (preparedPayload == null) {
        throw Exception('foreground handoff: prepared payload missing');
      }

      await uwb.sendOobThenJoinFromPayload(preparedPayload);
      ok = true;
      _log('foreground handoff completed');
    } catch (e, st) {
      _log('foreground handoff failed: $e');
      debugPrint('[PKE][FG] $st');
    } finally {
      _foregroundHandoffInFlight = false;
    }

    service.invoke('uwbHandoffResult', {
      'requestId': requestId,
      'ok': ok,
    });
  }

  static Future<void> _handleForegroundHandoffStop(String? requestId) async {
    final service = FlutterBackgroundService();
    final uwb = _foregroundUwb;

    try {
      _log('foreground UWB stop requested requestId=${requestId ?? '-'}');
      if (uwb != null) {
        await uwb.stopRanging();
        await uwb.disconnect();
      }
      _log('foreground UWB stop completed requestId=${requestId ?? '-'}');
      service.invoke('uwbHandoffStopped', {
        'requestId': requestId,
        'ok': true,
      });
    } catch (e, st) {
      _log('foreground UWB stop failed: $e');
      debugPrint('[PKE][FG] $st');
      service.invoke('uwbHandoffStopped', {
        'requestId': requestId,
        'ok': false,
        'error': e.toString(),
      });
    }
  }

  static Future<bool> isRunning() {
    if (!Platform.isAndroid) {
      return Future<bool>.value(false);
    }
    return _service.isRunning();
  }

  static Future<void> setPreferredDeviceAddress(String deviceAddress) {
    return PkeAuthOrchestrator.savePreferredDeviceAddress(deviceAddress);
  }

  static Future<void> triggerAuthOnce({String? deviceAddress}) async {
    if (!Platform.isAndroid) {
      _log('triggerAuthOnce skipped on non-Android platform');
      return;
    }

    await _ensureConfigured();
    if (deviceAddress != null && deviceAddress.trim().isNotEmpty) {
      await PkeAuthOrchestrator.savePreferredDeviceAddress(deviceAddress);
    }

    final running = await _service.isRunning();
    if (!running) {
      _log('triggerAuthOnce starting service before invoking auth');
      await _service.startService();
    }

    _log('triggerAuthOnce invoked deviceAddress=${deviceAddress ?? 'auto'}');
    _service.invoke('runAuthOnce', {
      if (deviceAddress != null) 'deviceAddress': deviceAddress,
    });
  }

  static Future<void> _ensureConfigured() async {
    if (_configured) {
      _log('background service already configured');
      return;
    }

    _log('configuring background service and notification channel');
    await _createNotificationChannel();

    await _service.configure(
      androidConfiguration: AndroidConfiguration(
        onStart: onStart,
        autoStart: false,
        autoStartOnBoot: false,
        isForegroundMode: true,
        foregroundServiceTypes: <AndroidForegroundType>[
          AndroidForegroundType.location,
          AndroidForegroundType.connectedDevice,
        ],
        notificationChannelId: _channelId,
        initialNotificationTitle: 'Smart Car Background Runtime',
        initialNotificationContent: 'Preparing background orchestration',
        foregroundServiceNotificationId: _notificationId,
      ),
      iosConfiguration: IosConfiguration(
        autoStart: false,
        onForeground: onStart,
        onBackground: onIosBackground,
      ),
    );

    _configured = true;
    _log('background service configured');
  }

  static Future<void> _createNotificationChannel() async {
    const channel = AndroidNotificationChannel(
      _channelId,
      _channelName,
      description: _channelDescription,
      importance: Importance.low,
      playSound: false,
      enableVibration: false,
      showBadge: false,
    );

    final notificationsPlugin = FlutterLocalNotificationsPlugin();
    final androidImplementation = notificationsPlugin
        .resolvePlatformSpecificImplementation<
          AndroidFlutterLocalNotificationsPlugin
        >();
    await androidImplementation?.createNotificationChannel(channel);
  }
}

@pragma('vm:entry-point')
Future<bool> onIosBackground(ServiceInstance service) async {
  WidgetsFlutterBinding.ensureInitialized();
  return true;
}

@pragma('vm:entry-point')
void onStart(ServiceInstance service) async {
  WidgetsFlutterBinding.ensureInitialized();
  DartPluginRegistrant.ensureInitialized();

  debugPrint('[PKE][BG] onStart entered');

  // Check if background service is enabled; stop immediately if disabled
  final prefs = await SharedPreferences.getInstance();
  final isEnabled = prefs.getBool('pke_bg_service_enabled') ?? true;
  if (!isEnabled) {
    debugPrint('[PKE][BG] Background service is disabled, stopping immediately');
    service.stopSelf();
    return;
  }

  const Duration scanTimeout = Duration(seconds: 4);
  const Duration scanTick = Duration(seconds: 8);
  const Duration retryBase = Duration(seconds: 2);
  const Duration retryMax = Duration(seconds: 20);

  await NfcProvisioningService.initialize(ownerIdHint: 'background');

  final telemetry = PkeTelemetry(source: 'app_bg');
  final authOrchestrator = PkeAuthOrchestrator(telemetry: telemetry);
  var authInFlight = false;
  var scanInFlight = false;
  var consecutiveAuthFailures = 0;
  DateTime nextAllowedAuthAt = DateTime.fromMillisecondsSinceEpoch(0);
  StreamSubscription<List<ScanResult>>? scanSubscription;
  telemetry.startAttempt();
  telemetry.emit(
    event: PkeTelemetryEvent.scanWake,
    details: 'background_service_started',
  );
  debugPrint('[PKE][BG] background isolate initialized');
  debugPrint(
    '[PKE][BG] scanTimeout=${scanTimeout.inSeconds}s scanTick=${scanTick.inSeconds}s retryBase=${retryBase.inSeconds}s retryMax=${retryMax.inSeconds}s',
  );

  if (service is AndroidServiceInstance) {
    service.setAsForegroundService();
    service.setForegroundNotificationInfo(
      title: 'Smart Car Background Runtime',
      content: 'Background orchestration active',
    );
  }

  Timer? heartbeat;
  Timer? scanLoop;

  bool isAuthCooldownActive() {
    return DateTime.now().isBefore(nextAllowedAuthAt);
  }

  Duration computeRetryDelay(int failureCount) {
    int delayMs = retryBase.inMilliseconds;
    for (int i = 1; i < failureCount; i++) {
      delayMs *= 2;
      if (delayMs >= retryMax.inMilliseconds) {
        delayMs = retryMax.inMilliseconds;
        break;
      }
    }
    if (delayMs > retryMax.inMilliseconds) {
      delayMs = retryMax.inMilliseconds;
    }
    return Duration(milliseconds: delayMs);
  }

  void scheduleRetryBackoff() {
    consecutiveAuthFailures++;
    final delay = computeRetryDelay(consecutiveAuthFailures);
    nextAllowedAuthAt = DateTime.now().add(delay);
    telemetry.emit(
      event: PkeTelemetryEvent.scanWake,
      details:
          'background_retry_backoff_${delay.inMilliseconds}ms_failures_$consecutiveAuthFailures',
    );
  }

  void resetRetryBackoff() {
    consecutiveAuthFailures = 0;
    nextAllowedAuthAt = DateTime.fromMillisecondsSinceEpoch(0);
  }

  Future<bool> runUwbHandoff({
    String? deviceAddress,
    BluetoothDevice? discoveredDevice,
  }) async {
    try {
      final targetAddress = (discoveredDevice?.remoteId.str ?? deviceAddress)
          ?.trim()
          .toUpperCase();
      if (targetAddress == null || targetAddress.isEmpty) {
        throw Exception('No BLE target available for UWB handoff');
      }
      final requestId = DateTime.now().millisecondsSinceEpoch.toString();

      telemetry.emit(
        event: PkeTelemetryEvent.scanWake,
        details: 'background_uwb_handoff_start',
      );
      debugPrint('[PKE][BG] Delegating UWB handoff to foreground engine');
      debugPrint('[PKE][BG] UWB handoff target=$targetAddress requestId=$requestId');

      service.invoke('runUwbHandoff', {
        'deviceAddress': targetAddress,
        'requestId': requestId,
      });
      return true;
    } catch (e, st) {
      debugPrint('[PKE][BG] UWB handoff failed: $e');
      debugPrint('[PKE][BG] Stack trace: $st');
      return false;
    }
  }

  Future<bool> runAuthOnce(
    String trigger, {
    String? deviceAddress,
    BluetoothDevice? discoveredDevice,
  }) async {
    debugPrint(
      '[PKE][BG] runAuthOnce trigger=$trigger deviceAddress=${deviceAddress ?? 'auto'} discovered=${discoveredDevice != null ? 1 : 0}',
    );

    if (authInFlight) {
      debugPrint('[PKE][BG] auth already in flight, ignoring trigger=$trigger');
      return false;
    }

    if (isAuthCooldownActive()) {
      debugPrint('[PKE][BG] auth cooldown active, ignoring trigger=$trigger');
      telemetry.emit(
        event: PkeTelemetryEvent.scanWake,
        details: 'background_auth_cooldown_active_$trigger',
      );
      return false;
    }

    authInFlight = true;
    try {
      final override = deviceAddress?.trim();
      if (override != null && override.isNotEmpty) {
        await PkeAuthOrchestrator.savePreferredDeviceAddress(override);
      }

      final target = (override != null && override.isNotEmpty)
          ? override
          : await PkeAuthOrchestrator.loadPreferredDeviceAddress();

      telemetry.startAttempt();
      telemetry.emit(
        event: PkeTelemetryEvent.scanWake,
        details: 'background_auth_trigger_$trigger',
      );

      final result = await authOrchestrator.authenticate(
        deviceAddress: target ?? '',
        device: discoveredDevice,
        timeout: const Duration(seconds: 25),
      );

      debugPrint(
        '[PKE][BG] auth result trigger=$trigger success=${result.success ? 1 : 0} message=${result.message}',
      );

      if (result.success) {
        debugPrint('[PKE][BG] auth success - holding BLE session open for UWB handoff');
        
        // Trigger foreground UWB setup WHILE the background connection is still alive
        final handoffOk = await runUwbHandoff(
          deviceAddress: target,
          discoveredDevice: discoveredDevice,
        );
        
        // NOW it is safe to release the background orchestrator's hold on the connection
        await authOrchestrator.disconnect();
        
        resetRetryBackoff();
        telemetry.emit(
          event: PkeTelemetryEvent.unlockDecision,
          unlockDecision: handoffOk ? 'allow' : 'deny',
          details: handoffOk
              ? 'background_auth_success'
              : 'background_auth_success_uwb_failed',
        );
        return handoffOk;
      } else {
        scheduleRetryBackoff();
        telemetry.emit(
          event: PkeTelemetryEvent.unlockDecision,
          unlockDecision: 'deny',
          details: 'background_auth_failed',
        );
        return false;
      }
    } catch (_) {
      debugPrint('[PKE][BG] auth exception trigger=$trigger');
      scheduleRetryBackoff();
      telemetry.emit(
        event: PkeTelemetryEvent.unlockDecision,
        unlockDecision: 'deny',
        details: 'background_auth_exception',
      );
      return false;
    } finally {
      await authOrchestrator.disconnect();
      authInFlight = false;
    }
  }

  bool isCccAuthCandidate(ScanResult result) {
    for (final serviceUuid in result.advertisementData.serviceUuids) {
      if (serviceUuid.toString().toLowerCase() ==
          PkeAuthOrchestrator.authServiceUUID.toLowerCase()) {
        return true;
      }
    }
    return false;
  }

  Future<void> handleScanMatch(ScanResult result) async {
    if (authInFlight) {
      debugPrint(
        '[PKE][BG] scan match ignored because auth is already running',
      );
      return;
    }

    if (isAuthCooldownActive()) {
      debugPrint('[PKE][BG] scan match ignored because cooldown is active');
      telemetry.emit(
        event: PkeTelemetryEvent.scanWake,
        details: 'background_scan_match_cooldown_active',
      );
      return;
    }

    scanInFlight = false;
    try {
      await FlutterBluePlus.stopScan();
    } catch (_) {}

    final remoteId = result.device.remoteId.str;
    debugPrint(
      '[PKE][BG] scan match remoteId=$remoteId rssi=${result.rssi} name=${result.device.platformName}',
    );
    telemetry.emit(
      event: PkeTelemetryEvent.scanWake,
      details: 'background_scan_match_$remoteId',
      rssiDbm: result.rssi,
    );

    await runAuthOnce(
      'scan_match',
      deviceAddress: remoteId,
      discoveredDevice: result.device,
    );
  }

  Future<void> beginScanBurst() async {
    if (scanInFlight || authInFlight || isAuthCooldownActive()) {
      debugPrint(
        '[PKE][BG] beginScanBurst skipped scanInFlight=${scanInFlight ? 1 : 0} authInFlight=${authInFlight ? 1 : 0} cooldown=${isAuthCooldownActive() ? 1 : 0}',
      );
      return;
    }

    scanInFlight = true;
    try {
      await FlutterBluePlus.stopScan();
    } catch (_) {}

    telemetry.emit(
      event: PkeTelemetryEvent.scanWake,
      details: 'background_scan_start',
    );
    debugPrint(
      '[PKE][BG] starting scan burst timeout=${scanTimeout.inSeconds}s',
    );

    try {
      await FlutterBluePlus.startScan(
        timeout: scanTimeout,
        withServices: [Guid(PkeAuthOrchestrator.authServiceUUID)],
      );
    } catch (_) {
      debugPrint('[PKE][BG] startScan threw; scanInFlight reset');
    } finally {
      scanInFlight = false;
      debugPrint('[PKE][BG] scan burst finished; scanInFlight reset');
    }
  }

  void stopService() {
    debugPrint('[PKE][BG] stopService requested - beginning cleanup');
    heartbeat?.cancel();
    debugPrint('[PKE][BG] heartbeat timer cancelled');
    scanLoop?.cancel();
    debugPrint('[PKE][BG] scan loop timer cancelled');
    unawaited(scanSubscription?.cancel());
    debugPrint('[PKE][BG] scan subscription cancelled');
    unawaited(FlutterBluePlus.stopScan());
    debugPrint('[PKE][BG] BLE scan stopped');
    final stopRequestId = DateTime.now().millisecondsSinceEpoch.toString();
    debugPrint('[PKE][BG] requesting foreground UWB stop requestId=$stopRequestId');
    service.invoke('stopUwbHandoff', {'requestId': stopRequestId});
    unawaited(authOrchestrator.disconnect());
    debugPrint('[PKE][BG] auth orchestrator disconnected');
    telemetry.emit(
      event: PkeTelemetryEvent.unlockDecision,
      unlockDecision: 'deny',
      details: 'background_service_stopped',
    );
    debugPrint('[PKE][BG] calling service.stopSelf()');
    service.stopSelf();
    debugPrint('[PKE][BG] service.stopSelf() completed');
  }

  service.on('stopService').listen((_) {
    stopService();
  });

  service.on('runAuthOnce').listen((event) {
    final payload = event is Map<dynamic, dynamic> ? event : null;
    final deviceAddress = payload?['deviceAddress'] as String?;
    runAuthOnce('invoke', deviceAddress: deviceAddress);
  });

  service.on('uwbHandoffResult').listen((event) {
    final payload = event is Map<dynamic, dynamic> ? event : null;
    final requestId = payload?['requestId']?.toString() ?? '-';
    final ok = payload?['ok'] == true;
    debugPrint('[PKE][BG] foreground handoff result requestId=$requestId ok=${ok ? 1 : 0}');
  });

  service.on('uwbHandoffStopped').listen((event) {
    final payload = event is Map<dynamic, dynamic> ? event : null;
    final requestId = payload?['requestId']?.toString() ?? '-';
    final ok = payload?['ok'] == true;
    debugPrint('[PKE][BG] foreground UWB stopped requestId=$requestId ok=${ok ? 1 : 0}');
  });

  scanSubscription = FlutterBluePlus.scanResults.listen((results) {
    for (final result in results) {
      if (!isCccAuthCandidate(result)) {
        continue;
      }
      debugPrint(
        '[PKE][BG] scan candidate remoteId=${result.device.remoteId.str} rssi=${result.rssi} services=${result.advertisementData.serviceUuids.join(',')}',
      );
      unawaited(handleScanMatch(result));
      break;
    }
  });

  scanLoop = Timer.periodic(scanTick, (_) {
    unawaited(beginScanBurst());
  });

  unawaited(beginScanBurst());
  debugPrint('[PKE][BG] initial scan burst scheduled');

  heartbeat = Timer.periodic(const Duration(seconds: 20), (_) {
    final now = DateTime.now();
    if (service is AndroidServiceInstance) {
      service.setForegroundNotificationInfo(
        title: 'Smart Car Background Runtime',
        content:
            'Active at ${now.hour.toString().padLeft(2, '0')}:${now.minute.toString().padLeft(2, '0')}:${now.second.toString().padLeft(2, '0')}',
      );
    }

    telemetry.emit(
      event: PkeTelemetryEvent.rssiSnapshot,
      details: 'background_service_heartbeat',
    );
  });

  unawaited(runAuthOnce('startup'));
  debugPrint('[PKE][BG] startup auth scheduled');
}
