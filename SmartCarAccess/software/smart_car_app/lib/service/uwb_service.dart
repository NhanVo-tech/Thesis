import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

class UwbOobPayload {
  static const int carRoleControlee = 0;
  static const int carRoleController = 1;

  const UwbOobPayload({
    required this.version,
    required this.role,
    required this.sessionId,
    required this.phoneMac,
    required this.carMac,
    required this.channel,
    required this.preambleIdx,
    required this.sfdId,
    required this.stsConfig,
    required this.hoppingMode,
    required this.rframeConfig,
    required this.resultReportConfig,
    required this.aoaResultReq,
    required this.scheduleMode,
    required this.multiNodeMode,
    required this.rangingRoundUsage,
    required this.rssiReporting,
    required this.slotDuration,
    required this.rangingInterval,
    required this.slotsPerRr,
    required this.vendorId,
    required this.staticStsIv,
  });

  final int version;
  final int role;
  final int sessionId;
  final int phoneMac;
  final int carMac;
  final int channel;
  final int preambleIdx;
  final int sfdId;
  final int stsConfig;
  final int hoppingMode;
  final int rframeConfig;
  final int resultReportConfig;
  final int aoaResultReq;
  final int scheduleMode;
  final int multiNodeMode;
  final int rangingRoundUsage;
  final int rssiReporting;
  final int slotDuration;
  final int rangingInterval;
  final int slotsPerRr;
  final int vendorId;
  final Uint8List staticStsIv;

  bool get carIsController => role == carRoleController;
  bool get carIsControlee => role == carRoleControlee;
  bool get phoneIsController => true;
  bool get phoneIsControlee => false;
  @Deprecated('Use carIsController/carIsControlee/phoneIsController instead.')
  bool get controller => phoneIsController;
  String get phoneMacString => UwbService.formatShortMac(phoneMac);
  String get carMacString => UwbService.formatShortMac(carMac);
  String get remoteAddress => carMacString;
  Uint8List get sessionKeyInfo {
    return Uint8List.fromList(<int>[
      vendorId & 0xFF,
      (vendorId >> 8) & 0xFF,
      ...staticStsIv,
    ]);
  }
  Map<String, Object> toMap() {
    return <String, Object>{
      'version': version,
      'role': role,
      'phoneRole': 'controller',
      'carRole': carIsControlee ? 'controlee' : 'controller',
      'sessionId': sessionId,
      'phoneMac': phoneMac,
      'phoneMacString': phoneMacString,
      'carMac': carMac,
      'carMacString': carMacString,
      'remoteAddress': remoteAddress,
      'channel': channel,
      'preambleIdx': preambleIdx,
      'sfdId': sfdId,
      'stsConfig': stsConfig,
      'hoppingMode': hoppingMode,
      'rframeConfig': rframeConfig,
      'resultReportConfig': resultReportConfig,
      'aoaResultReq': aoaResultReq,
      'scheduleMode': scheduleMode,
      'multiNodeMode': multiNodeMode,
      'rangingRoundUsage': rangingRoundUsage,
      'rssiReporting': rssiReporting,
      'slotDuration': slotDuration,
      'rangingInterval': rangingInterval,
      'slotsPerRr': slotsPerRr,
      'vendorId': vendorId,
      'staticStsIv': staticStsIv.toList(),
      'staticStsIvHex': UwbService.toHex(staticStsIv),
      'sessionKeyInfo': sessionKeyInfo.toList(),
      'sessionKeyInfoHex': UwbService.toHex(sessionKeyInfo),
    };
  }
}

class UwbRangingEvent {
  const UwbRangingEvent({
    required this.type,
    required this.raw,
    this.distanceM,
    this.azimuthDeg,
    this.elevationDeg,
    this.elapsedRealtimeNanos,
    this.elapsedMs,
    this.positionSeen,
  });

  final String type;
  final Map<dynamic, dynamic> raw;
  final double? distanceM;
  final double? azimuthDeg;
  final double? elevationDeg;
  final int? elapsedRealtimeNanos;
  final int? elapsedMs;
  final bool? positionSeen;

  bool get hasPosition => type == 'position';

  factory UwbRangingEvent.fromDynamic(dynamic event) {
    final map = (event as Map?)?.cast<dynamic, dynamic>() ?? const <dynamic, dynamic>{};
    return UwbRangingEvent(
      type: map['type']?.toString() ?? 'unknown',
      raw: map,
      distanceM: UwbService._asDouble(map['distanceM']),
      azimuthDeg: UwbService._asDouble(map['azimuthDeg']),
      elevationDeg: UwbService._asDouble(map['elevationDeg']),
      elapsedRealtimeNanos: UwbService._asInt(map['elapsedRealtimeNanos']),
      elapsedMs: UwbService._asInt(map['elapsedMs']),
      positionSeen: map['positionSeen'] as bool?,
    );
  }
}

class UwbService {
  static const String adminServiceUuid = '9a9b9c9d-0000-4000-8000-9a9b9c9d0000';
  static const String adminInfoCharUuid = '9a9b9c9d-0003-4000-8000-9a9b9c9d0003';
  static const String adminCmdCharUuid = '9a9b9c9d-0002-4000-8000-9a9b9c9d0002';
  static const String adminUwbOobCharUuid = '9a9b9c9d-0005-4000-8000-9a9b9c9d0005';
  static const int adminCmdStartUwb = 0x50;
  static const int adminCmdStopUwb = 0x51;
  static const MethodChannel _uwbMethodChannel = MethodChannel('smartcar/uwb');
  static const EventChannel _uwbEventChannel = EventChannel('smartcar/uwb/events');

  BluetoothDevice? _device;
  BluetoothCharacteristic? _infoChar;
  BluetoothCharacteristic? _cmdChar;
  BluetoothCharacteristic? _oobChar;
  Uint8List? _preparedPayload;
  UwbOobPayload? _preparedOob;
  Map<dynamic, dynamic>? _preparedSessionInfo;

  StreamSubscription<List<int>>? _infoSub;
  StreamSubscription<BluetoothConnectionState>? _connSub;
  StreamSubscription<dynamic>? _uwbEventSub;
  
  Timer? _pollingTimer;
  final bool _enableEventChannel;
  final bool _logToConsole;
  bool _pollingErrorLogged = false;

  final StreamController<String> _logController = StreamController<String>.broadcast();
  final StreamController<Uint8List> _infoController = StreamController<Uint8List>.broadcast();
  final StreamController<UwbOobPayload> _oobController = StreamController<UwbOobPayload>.broadcast();
  final StreamController<UwbRangingEvent> _rangingController = StreamController<UwbRangingEvent>.broadcast();

  Stream<String> get logs => _logController.stream;
  Stream<Uint8List> get infoNotifications => _infoController.stream;
  Stream<UwbOobPayload> get oobPayloads => _oobController.stream;
  Stream<UwbRangingEvent> get rangingEvents => _rangingController.stream;

  BluetoothDevice? get connectedDevice => _device;
  bool get isConnected => _device != null && _device!.isConnected;

  UwbService({bool enableEventChannel = true, bool logToConsole = false})
      : _enableEventChannel = enableEventChannel,
        _logToConsole = logToConsole {
    if (!Platform.isAndroid) {
      return;
    }
    if (!_enableEventChannel) {
      _startPollingFallback();
      return;
    }
    _activateEventChannelWithFallback();
  }

  void _activateEventChannelWithFallback() {
    try {
      _uwbEventSub = _uwbEventChannel.receiveBroadcastStream().listen(
        (dynamic event) {
          final parsed = UwbRangingEvent.fromDynamic(event);
          _rangingController.add(parsed);
          if (parsed.hasPosition) {
            _log(
              'UWB <= ${parsed.type} d=${_fmt(parsed.distanceM)}m az=${_fmt(parsed.azimuthDeg)} el=${_fmt(parsed.elevationDeg)}',
            );
          } else {
            _log('UWB <= ${parsed.type}');
          }
        },
        onError: (Object error, StackTrace stackTrace) {
          _log('UWB event stream error: $error');
          _startPollingFallback();
        },
      );
    } catch (e) {
      _log('EventChannel unavailable (background isolate?): $e');
      _startPollingFallback();
    }
  }

  void _startPollingFallback() {
    if (_pollingTimer != null) {
      return;
    }
    _log('Starting UWB ranging polling fallback (200ms interval)');
    _pollingTimer = Timer.periodic(const Duration(milliseconds: 200), (_) async {
      try {
        final result = await _uwbMethodChannel.invokeMethod<Map<dynamic, dynamic>?>('getLatestRanging');
        if (result != null) {
          final parsed = UwbRangingEvent.fromDynamic(result);
          _rangingController.add(parsed);
        }
      } catch (e) {
        if (!_pollingErrorLogged) {
          _pollingErrorLogged = true;
          _log('Polling getLatestRanging failed: $e');
        }
      }
    });
  }

  Future<void> ensureBluetoothReady() async {
    final supported = await FlutterBluePlus.isSupported;
    if (!supported) {
      throw Exception('Bluetooth is not supported on this device');
    }

    final state = await FlutterBluePlus.adapterState.first;
    if (state != BluetoothAdapterState.on) {
      throw Exception('Bluetooth adapter is not ON');
    }
  }

  Future<List<ScanResult>> scan({
    Duration timeout = const Duration(seconds: 10),
    String? nameContains,
  }) async {
    await ensureBluetoothReady();
    _log('Scanning BLE devices for ${timeout.inSeconds}s');

    final Map<String, ScanResult> seen = <String, ScanResult>{};
    final scanSub = FlutterBluePlus.scanResults.listen((results) {
      for (final r in results) {
        final key = r.device.remoteId.str.toUpperCase();
        final existing = seen[key];
        if (existing == null || r.rssi >= existing.rssi) {
          seen[key] = r;
        }
      }
    });

    try {
      await FlutterBluePlus.startScan(timeout: timeout);
      await Future<void>.delayed(timeout + const Duration(milliseconds: 200));
    } finally {
      await FlutterBluePlus.stopScan();
      await scanSub.cancel();
    }

    final filtered = seen.values.where((r) {
      if (nameContains == null || nameContains.trim().isEmpty) {
        return true;
      }
      final name = r.device.platformName.trim();
      return name.toLowerCase().contains(nameContains.toLowerCase());
    }).toList();

    filtered.sort((a, b) => b.rssi.compareTo(a.rssi));
    _log('Scan done: ${filtered.length} device(s)');
    return filtered;
  }

  Future<void> connect(BluetoothDevice device) async {
    if (_device != null && _device!.remoteId == device.remoteId && _device!.isConnected) {
      _log('Already connected to ${device.remoteId.str}');
      return;
    }

    await disconnect();

    _device = device;
    _connSub = device.connectionState.listen((state) {
      _log('Connection state: $state');
      if (state == BluetoothConnectionState.disconnected) {
        _clearCharacteristicsOnly();
      }
    });

    if (device.isDisconnected) {
      _log('Connecting to ${device.platformName.isEmpty ? device.remoteId.str : device.platformName}');
      await device.connect(timeout: const Duration(seconds: 12));
      await Future<void>.delayed(const Duration(milliseconds: 250));
    }

    final int mtu = await device.requestMtu(247).catchError((_) => 23);
    _log('MTU negotiated: $mtu');

    final services = await device.discoverServices();
    final adminService = services.where((s) {
      return s.uuid.toString().toLowerCase() == adminServiceUuid.toLowerCase();
    }).toList();

    if (adminService.isEmpty) {
      throw Exception('Admin service not found: $adminServiceUuid');
    }

    final chars = adminService.first.characteristics;
    _infoChar = _findCharacteristic(chars, adminInfoCharUuid);
    _cmdChar = _findCharacteristic(chars, adminCmdCharUuid);
    _oobChar = _findCharacteristic(chars, adminUwbOobCharUuid);

    if (_infoChar == null || _oobChar == null) {
      throw Exception('Required characteristic(s) missing on Admin service');
    }

    await _infoChar!.setNotifyValue(true);
    _infoSub?.cancel();
    _infoSub = _infoChar!.lastValueStream.listen((value) {
      final bytes = Uint8List.fromList(value);
      _infoController.add(bytes);
      _log('INFO <= ${_asAsciiOrHex(bytes)}');
      _tryEmitOob(bytes);
    });

    _log('Connected and ready for UWB OOB write');
  }

  Future<void> sendAdminCmd(int cmd) async {
    if (_cmdChar == null) {
      throw Exception('Admin command characteristic is not ready');
    }

    final payload = Uint8List.fromList(<int>[cmd & 0xFF]);
    await _cmdChar!.write(payload, withoutResponse: false);
    _log('CMD => 0x${cmd.toRadixString(16).padLeft(2, '0').toUpperCase()}');
  }

  Future<void> sendOobPayload(Uint8List payload) async {
    if (_oobChar == null) {
      throw Exception('Admin OOB characteristic is not ready');
    }
    if (payload.length != 37) {
      throw Exception('OOB payload must be exactly 37 bytes, got ${payload.length}');
    }

    final useWithoutResponse = _oobChar!.properties.writeWithoutResponse;
    await _oobChar!.write(payload, withoutResponse: useWithoutResponse);
    _log('OOB => ${payload.length} bytes sent');
    _log('OOB payload hex: ${UwbService.toHex(payload)}');
  }

  Future<void> requestRemoteUwbStart() async {
    await sendAdminCmd(adminCmdStartUwb);
    _log('Requested ECU to start cached UWB session');
  }

  Future<void> requestRemoteUwbStop() async {
    await sendAdminCmd(adminCmdStopUwb);
    _log('Requested ECU to stop active UWB session');
  }

  Future<void> disconnect() async {
    await _infoSub?.cancel();
    _infoSub = null;

    await _connSub?.cancel();
    _connSub = null;

    final d = _device;
    _clearCharacteristicsOnly();

    if (d != null && d.isConnected) {
      await d.disconnect();
      _log('Disconnected');
    }

    _device = null;
    clearPreparedContext();
  }

  Uint8List buildDefaultOobPayloadV1({
    int role = UwbOobPayload.carRoleControlee,
    int sessionId = 42,
    int phoneMac = 0x0001,
    int carMac = 0x0002,
    int channel = 9,
    int preambleIdx = 9,
    int sfdId = 2,
    int stsConfig = 0,
    int hoppingMode = 1,
    int rframeConfig = 3,
    int resultReportConfig = 0x0B,
    int aoaResultReq = 1,
    int scheduleMode = 1,
    int multiNodeMode = 0,
    int rangingRoundUsage = 2,
    int rssiReporting = 1,
    int slotDuration = 2400,
    int rangingInterval = 120,
    int slotsPerRr = 6,
    int vendorId = 0x0708,
    Uint8List? staticStsIv,
  }) {
    final iv = staticStsIv ?? Uint8List.fromList(const <int>[1, 2, 3, 4, 5, 6]);
    if (iv.length != 6) {
      throw Exception('staticStsIv must be 6 bytes');
    }

    final out = BytesBuilder();
    out.addByte(1);
    out.addByte(role & 0xFF);
    _addLe32(out, sessionId);
    _addLe16(out, phoneMac);
    _addLe16(out, carMac);
    out.addByte(channel & 0xFF);
    out.addByte(preambleIdx & 0xFF);
    out.addByte(sfdId & 0xFF);
    out.addByte(stsConfig & 0xFF);
    out.addByte(hoppingMode & 0xFF);
    out.addByte(rframeConfig & 0xFF);
    out.addByte(resultReportConfig & 0xFF);
    out.addByte(aoaResultReq & 0xFF);
    out.addByte(scheduleMode & 0xFF);
    out.addByte(multiNodeMode & 0xFF);
    out.addByte(rangingRoundUsage & 0xFF);
    out.addByte(rssiReporting & 0xFF);
    _addLe16(out, slotDuration);
    _addLe32(out, rangingInterval);
    out.addByte(slotsPerRr & 0xFF);
    _addLe16(out, vendorId);
    out.add(iv);

    final bytes = out.toBytes();
    if (bytes.length != 37) {
      throw Exception('Internal build error: payload length ${bytes.length}');
    }
    return bytes;
  }

  UwbOobPayload parseOobPayload(Uint8List payload) {
    if (payload.length != 37) {
      throw Exception('OOB payload must be exactly 37 bytes, got ${payload.length}');
    }
    if (payload[0] != 1) {
      throw Exception('Unsupported OOB version ${payload[0]}');
    }
    final data = ByteData.sublistView(payload);
    return UwbOobPayload(
      version: payload[0],
      role: payload[1],
      sessionId: data.getUint32(2, Endian.little),
      phoneMac: data.getUint16(6, Endian.little),
      carMac: data.getUint16(8, Endian.little),
      channel: payload[10],
      preambleIdx: payload[11],
      sfdId: payload[12],
      stsConfig: payload[13],
      hoppingMode: payload[14],
      rframeConfig: payload[15],
      resultReportConfig: payload[16],
      aoaResultReq: payload[17],
      scheduleMode: payload[18],
      multiNodeMode: payload[19],
      rangingRoundUsage: payload[20],
      rssiReporting: payload[21],
      slotDuration: data.getUint16(22, Endian.little),
      rangingInterval: data.getUint32(24, Endian.little),
      slotsPerRr: payload[28],
      vendorId: data.getUint16(29, Endian.little),
      staticStsIv: Uint8List.sublistView(payload, 31, 37),
    );
  }

  Future<bool> isUwbSupported() async {
    if (!Platform.isAndroid) {
      return false;
    }
    return await _uwbMethodChannel.invokeMethod<bool>('isSupported') ?? false;
  }

  Future<bool> ensureUwbPermission() async {
    if (!Platform.isAndroid) {
      return false;
    }
    try {
      return await _uwbMethodChannel.invokeMethod<bool>('ensurePermission') ?? false;
    } catch (e) {
      // In background isolate contexts, platform channel may not be registered.
      // Assume permission is already granted if we're in background service.
      _log('ensureUwbPermission: platform channel unavailable (background context?), assuming granted');
      return true;
    }
  }

  Future<Map<dynamic, dynamic>?> prepareSession({required bool controller}) async {
    if (!Platform.isAndroid) {
      throw UnsupportedError('UWB ranging is only available on Android');
    }
    try {
      final result = await _uwbMethodChannel.invokeMethod<dynamic>('prepareSession', <String, dynamic>{
        'controller': controller,
      });
      return (result as Map?)?.cast<dynamic, dynamic>();
    } catch (e) {
      // In background isolate contexts, platform channel may not be registered.
      // Return minimal session info to allow the handoff to proceed.
      _log('prepareSession: platform channel unavailable (background context?), using fallback');
      return <dynamic, dynamic>{
        'localAddress': 'bg_fallback',
        'rawAddress': [0, 0],
      };
    }
  }

  Future<Map<String, Object?>> preparePayloadForOob(Uint8List payload) async {
    final normalizedPayload = updateRoleInPayload(payload, UwbOobPayload.carRoleControlee);
    final cached = getPreparedContext(normalizedPayload);
    if (cached != null && _preparedOob != null) {
      _log(
        'Reusing prepared local UWB ${_preparedSessionInfo?['localAddress'] ?? '-'} with phoneMac=${_preparedOob!.phoneMacString}',
      );
      return cached;
    }

    try {
      final supported = await isUwbSupported();
      if (!supported) {
        throw Exception('UWB is not supported on this device');
      }
    } catch (e) {
      _log('UWB support check failed (may be in background context): $e; proceeding anyway');
    }

    await ensureUwbPermission();

    final session = await prepareSession(controller: true);
    int localShortAddress;
    try {
      localShortAddress = _shortAddressFromRawAddress(session?['rawAddress']);
      if (localShortAddress == 0) {
        throw Exception('platform returned zero local UWB address');
      }
    } catch (e) {
      _log('preparePayloadForOob: could not get local UWB address from platform ($e); using payload phoneMac as fallback');
      final tmp = ByteData.sublistView(normalizedPayload);
      localShortAddress = tmp.getUint16(6, Endian.little);
    }
    final patchedPayload = updatePhoneMacInPayload(normalizedPayload, localShortAddress);
    final patchedOob = parseOobPayload(patchedPayload);
    _preparedPayload = patchedPayload;
    _preparedOob = patchedOob;
    _preparedSessionInfo = session == null ? null : Map<dynamic, dynamic>.from(session);

    _log(
      'Prepared local UWB ${session?['localAddress'] ?? '-'} with phone=controller car=${patchedOob.carIsControlee ? 'controlee' : 'controller'} phoneMac=${patchedOob.phoneMacString}',
    );

    return <String, Object?>{
      'payload': patchedPayload,
      'oob': patchedOob.toMap(),
      'session': session == null ? null : Map<dynamic, dynamic>.from(session),
    };
  }

  Map<String, Object?>? getPreparedContext(Uint8List payload) {
    final preparedPayload = _preparedPayload;
    final preparedOob = _preparedOob;
    if (preparedPayload == null || preparedOob == null) {
      return null;
    }
    if (!_byteListEquals(preparedPayload, payload)) {
      return null;
    }
    return <String, Object?>{
      'payload': preparedPayload,
      'oob': preparedOob.toMap(),
      'session': _preparedSessionInfo == null ? null : Map<dynamic, dynamic>.from(_preparedSessionInfo!),
    };
  }

  Uint8List updatePhoneMacInPayload(Uint8List payload, int phoneMac) {
    if (payload.length != 37) {
      throw Exception('OOB payload must be exactly 37 bytes, got ${payload.length}');
    }
    final out = Uint8List.fromList(payload);
    out[6] = phoneMac & 0xFF;
    out[7] = (phoneMac >> 8) & 0xFF;
    return out;
  }

  Uint8List updateRoleInPayload(Uint8List payload, int role) {
    if (payload.length != 37) {
      throw Exception('OOB payload must be exactly 37 bytes, got ${payload.length}');
    }
    final out = Uint8List.fromList(payload);
    out[1] = role & 0xFF;
    return out;
  }

  Future<bool> startRangingFromOob(UwbOobPayload oob, {String? remoteAddress}) {
    return startRanging(
      remoteAddress: remoteAddress ?? oob.remoteAddress,
      sessionId: oob.sessionId,
      channel: oob.channel,
      preambleIndex: oob.preambleIdx,
      sessionKeyInfo: oob.sessionKeyInfo,
    );
  }

  Future<bool> startRanging({
    required String remoteAddress,
    required int sessionId,
    required int channel,
    required int preambleIndex,
    required List<int> sessionKeyInfo,
    int subSessionId = 0,
    String updateRate = 'frequent',
  }) async {
    if (!Platform.isAndroid) {
      throw UnsupportedError('UWB ranging is only available on Android');
    }
    try {
      return await _uwbMethodChannel.invokeMethod<bool>('startRanging', <String, dynamic>{
            'remoteAddress': remoteAddress,
            'sessionId': sessionId,
            'subSessionId': subSessionId,
            'channel': channel,
            'preambleIndex': preambleIndex,
            'sessionKeyInfo': sessionKeyInfo,
            'updateRate': updateRate,
          }) ??
          false;
    } catch (e) {
      // In background isolate, platform channel may not be available.
      // Return false to indicate we couldn't start, but don't crash.
      _log('startRanging failed (may be in background context): $e');
      return false;
    }
  }

  Future<Map<String, Object?>> startPreparedRanging(Uint8List payload) async {
    final prepared = getPreparedContext(payload);
    final preparedOob = _preparedOob;
    if (prepared == null || preparedOob == null) {
      throw Exception('No matching prepared UWB session. Send OOB first or prepare again.');
    }
    await requestRemoteUwbStart();
    final started = await startRangingFromOob(preparedOob);
    if (!started) {
      _log('UWB startRangingFromOob returned false');
      throw Exception('UWB startRangingFromOob failed');
    }
    _log(
      'UWB session started: phone=controller car=${preparedOob.carIsControlee ? 'controlee' : 'controller'} remote=${preparedOob.remoteAddress}',
    );
    return prepared;
  }

  Future<bool> stopRanging() async {
    if (isConnected) {
      try {
        await requestRemoteUwbStop();
      } catch (e) {
        _log('ECU stop request failed: $e');
      }
    }
    if (!Platform.isAndroid) {
      return false;
    }
    try {
      final stopped = await _uwbMethodChannel.invokeMethod<bool>('stopRanging') ?? false;
      clearPreparedContext();
      return stopped;
    } catch (e) {
      _log('stopRanging failed (may be in background context): $e');
      clearPreparedContext();
      return false;
    }
  }

  Future<Map<String, Object?>> joinSessionFromOob(Uint8List payload) async {
    final prepared = getPreparedContext(payload);
    if (prepared != null) {
      return await startPreparedRanging(payload);
    }
    throw Exception(
      'Current payload is not the prepared OOB for the active phone UWB session. Press Send OOB again, then Join without regenerating payload.',
    );
  }

  Future<Map<String, Object?>> sendOobThenJoinFromPayload(
    Uint8List payload, {
    Duration settleDelay = const Duration(milliseconds: 500),
    int joinAttempts = 3,
    Duration retryDelay = const Duration(milliseconds: 350),
  }) async {
    final prepared = await preparePayloadForOob(payload);
    final preparedPayload = prepared['payload'] as Uint8List;

    await sendOobPayload(preparedPayload);
    if (settleDelay > Duration.zero) {
      await Future<void>.delayed(settleDelay);
    }

    Object? lastError;
    for (int attempt = 1; attempt <= joinAttempts; attempt++) {
      try {
        return await joinSessionFromOob(preparedPayload);
      } catch (e) {
        lastError = e;
        if (attempt >= joinAttempts) {
          break;
        }
        await Future<void>.delayed(
          Duration(milliseconds: retryDelay.inMilliseconds * attempt),
        );
      }
    }

    throw Exception('UWB join failed after OOB send: $lastError');
  }

  void dispose() {
    final d = _device;
    _infoSub?.cancel();
    _infoSub = null;
    _connSub?.cancel();
    _connSub = null;
    _uwbEventSub?.cancel();
    _uwbEventSub = null;
    _pollingTimer?.cancel();
    _pollingTimer = null;
    _clearCharacteristicsOnly();
    clearPreparedContext();
    _device = null;
    if (d != null && d.isConnected) {
      d.disconnect();
    }
    _oobController.close();
    _rangingController.close();
    _logController.close();
    _infoController.close();
  }

  void _clearCharacteristicsOnly() {
    _infoChar = null;
    _cmdChar = null;
    _oobChar = null;
  }

  void clearPreparedContext() {
    _preparedPayload = null;
    _preparedOob = null;
    _preparedSessionInfo = null;
  }

  void _log(String msg) {
    _logController.add('[${DateTime.now().toIso8601String().substring(11, 19)}] $msg');
    if (_logToConsole) {
      debugPrint('[UWB] $msg');
    }
  }

  static void _addLe16(BytesBuilder out, int value) {
    out.addByte(value & 0xFF);
    out.addByte((value >> 8) & 0xFF);
  }

  static void _addLe32(BytesBuilder out, int value) {
    out.addByte(value & 0xFF);
    out.addByte((value >> 8) & 0xFF);
    out.addByte((value >> 16) & 0xFF);
    out.addByte((value >> 24) & 0xFF);
  }

  static Uint8List parseHex(String input) {
    final cleaned = input
        .replaceAll(RegExp(r'0x', caseSensitive: false), '')
        .replaceAll(RegExp(r'[^0-9A-Fa-f]'), '');

    if (cleaned.isEmpty || cleaned.length.isOdd) {
      throw Exception('Hex string must contain an even number of hex chars');
    }

    final out = Uint8List(cleaned.length ~/ 2);
    for (int i = 0; i < cleaned.length; i += 2) {
      out[i ~/ 2] = int.parse(cleaned.substring(i, i + 2), radix: 16);
    }
    return out;
  }

  static String toHex(Uint8List bytes, {String separator = ' '}) {
    return bytes.map((b) => b.toRadixString(16).padLeft(2, '0').toUpperCase()).join(separator);
  }

  static String formatShortMac(int value) {
    final first = value & 0xFF;
    final second = (value >> 8) & 0xFF;
    return '${first.toRadixString(16).padLeft(2, '0').toUpperCase()}:${second.toRadixString(16).padLeft(2, '0').toUpperCase()}';
  }

  static String _asAsciiOrHex(Uint8List bytes) {
    final isPrintableAscii = bytes.every((b) => b == 0x0A || b == 0x0D || (b >= 0x20 && b <= 0x7E));
    if (isPrintableAscii) {
      return String.fromCharCodes(bytes).trim();
    }
    return toHex(bytes);
  }

  static BluetoothCharacteristic? _findCharacteristic(
    List<BluetoothCharacteristic> chars,
    String uuid,
  ) {
    for (final c in chars) {
      if (c.uuid.toString().toLowerCase() == uuid.toLowerCase()) {
        return c;
      }
    }
    return null;
  }

  void _tryEmitOob(Uint8List bytes) {
    try {
      final oob = parseOobPayload(bytes);
      _oobController.add(oob);
      _log(
        'Parsed OOB: phone=controller car=${oob.carIsControlee ? 'controlee' : 'controller'} session=${oob.sessionId} remote=${oob.remoteAddress}',
      );
    } catch (_) {
      // Ignore non-OOB notifications on the same info characteristic.
    }
  }

  static double? _asDouble(dynamic value) {
    if (value is num) {
      return value.toDouble();
    }
    return null;
  }

  static int? _asInt(dynamic value) {
    if (value is num) {
      return value.toInt();
    }
    return null;
  }

  static String _fmt(double? value) {
    if (value == null) {
      return '-';
    }
    return value.toStringAsFixed(2);
  }

  static bool _byteListEquals(Uint8List a, Uint8List b) {
    if (identical(a, b)) {
      return true;
    }
    if (a.length != b.length) {
      return false;
    }
    for (int i = 0; i < a.length; i++) {
      if (a[i] != b[i]) {
        return false;
      }
    }
    return true;
  }

  static int _shortAddressFromRawAddress(dynamic rawAddress) {
    final bytes = (rawAddress as List?)?.cast<dynamic>() ?? const <dynamic>[];
    if (bytes.length != 2) {
      throw Exception('Expected 2-byte local UWB address, got ${bytes.length}');
    }
    final first = (bytes[0] as num).toInt() & 0xFF;
    final second = (bytes[1] as num).toInt() & 0xFF;
    return first | (second << 8);
  }
}
