import 'dart:async';
import 'dart:io';

import 'package:flutter/services.dart';

/// Ranging status from the native multicast bridge: ACTIVE | STOPPED | ERROR.
class UwbMultiStatus {
  const UwbMultiStatus(this.status, this.message);
  final String status;
  final String message;
}

/// One multi-anchor ranging round. [mask] bit i set => d(i) is valid.
class UwbMultiRange {
  const UwbMultiRange(this.d0, this.d1, this.d2, this.mask);
  final double d0;
  final double d1;
  final double d2;
  final int mask;
  bool valid(int i) => (mask & (1 << i)) != 0;
}

/// Dart wrapper over the native android.ranging multicast DS-TWR bridge.
/// Additive: separate channels from the Stage 1 single-anchor UwbService.
class UwbMultiService {
  static const MethodChannel _method = MethodChannel('smartcar/uwb_multi');
  static const EventChannel _events = EventChannel('smartcar/uwb_multi/events');

  final StreamController<UwbMultiRange> _rangeCtrl =
      StreamController<UwbMultiRange>.broadcast();
  final StreamController<UwbMultiStatus> _statusCtrl =
      StreamController<UwbMultiStatus>.broadcast();
  final StreamController<String> _logCtrl =
      StreamController<String>.broadcast();
  StreamSubscription<dynamic>? _sub;
  bool _listening = false;

  Stream<UwbMultiRange> get ranges => _rangeCtrl.stream;
  Stream<UwbMultiStatus> get status => _statusCtrl.stream;
  Stream<String> get logs => _logCtrl.stream;

  void _log(String message) {
    _logCtrl.add('[UWB] $message');
  }

  Future<bool> isSupported() async {
    if (!Platform.isAndroid) {
      _log('isSupported=false (not Android)');
      return false;
    }
    try {
      final supported =
          (await _method.invokeMethod<bool>('isSupported')) ?? false;
      _log('isSupported=$supported');
      return supported;
    } catch (e) {
      _log('isSupported error: $e');
      return false;
    }
  }

  Future<bool> ensurePermission() async {
    if (!Platform.isAndroid) {
      _log('ensurePermission=false (not Android)');
      return false;
    }
    try {
      final granted =
          (await _method.invokeMethod<bool>('ensurePermission')) ?? false;
      _log('ensurePermission=$granted');
      return granted;
    } catch (e) {
      _log('ensurePermission error: $e');
      return false;
    }
  }

  void _ensureListening() {
    if (_listening) return;
    _listening = true;
    _sub = _events.receiveBroadcastStream().listen(
      (dynamic event) {
        final map = (event as Map?)?.cast<dynamic, dynamic>() ??
            const <dynamic, dynamic>{};
        final type = map['type']?.toString();
        if (type == 'range') {
          final range = UwbMultiRange(
            _d(map['d0']),
            _d(map['d1']),
            _d(map['d2']),
            (map['mask'] as num?)?.toInt() ?? 0,
          );
          _rangeCtrl.add(range);
          _log(
            'range d0=${_fmt(range.d0)} d1=${_fmt(range.d1)} '
            'd2=${_fmt(range.d2)} mask=0x${range.mask.toRadixString(2).padLeft(3, '0')}',
          );
        } else if (type == 'status') {
          final status = UwbMultiStatus(
            map['status']?.toString() ?? 'ERROR',
            map['message']?.toString() ?? '',
          );
          _statusCtrl.add(status);
          _log('status ${status.status}: ${status.message}');
        }
      },
      onError: (Object e, StackTrace _) {
        _statusCtrl.add(UwbMultiStatus('ERROR', '$e'));
        _log('event stream error: $e');
      },
    );
  }

  Future<bool> start({
    int sessionId = 42,
    int channel = 9,
    int preambleIndex = 9,
    int localAddress = 0x06C1,
    List<int> anchorAddresses = const <int>[0, 1, 2],
    List<int> sessionKeyInfo = const <int>[
      0x08, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    ],
  }) async {
    if (!Platform.isAndroid) {
      _log('start ignored (not Android)');
      return false;
    }
    _log(
      'start sessionId=$sessionId channel=$channel preamble=$preambleIndex '
      'local=0x${localAddress.toRadixString(16).toUpperCase()} '
      'anchors=$anchorAddresses',
    );
    _ensureListening();
    final granted = await ensurePermission();
    if (!granted) {
      _log('start failed: permission denied');
      _statusCtrl.add(const UwbMultiStatus('ERROR', 'UWB permission denied'));
      return false;
    }
    try {
      final ok = (await _method.invokeMethod<bool>('start', <String, dynamic>{
            'sessionId': sessionId,
            'channel': channel,
            'preambleIndex': preambleIndex,
            'localAddress': localAddress,
            'anchorAddresses': anchorAddresses,
            'sessionKeyInfo': sessionKeyInfo,
          })) ??
          false;
      _log('start result=$ok');
      return ok;
    } catch (e) {
      _log('start error: $e');
      _statusCtrl.add(UwbMultiStatus('ERROR', '$e'));
      return false;
    }
  }

  Future<void> stop() async {
    if (!Platform.isAndroid) return;
    _log('stop requested');
    try {
      await _method.invokeMethod('stop');
      _log('stop done');
    } catch (e) {
      _log('stop error: $e');
    }
  }

  Future<bool> isActive() async {
    if (!Platform.isAndroid) return false;
    try {
      return (await _method.invokeMethod<bool>('isActive')) ?? false;
    } catch (_) {
      return false;
    }
  }

  void dispose() {
    _sub?.cancel();
    _rangeCtrl.close();
    _statusCtrl.close();
    _logCtrl.close();
  }

  static double _d(dynamic v) => v is num ? v.toDouble() : double.nan;
  static String _fmt(double v) => v.isNaN ? '---' : v.toStringAsFixed(2);
}
