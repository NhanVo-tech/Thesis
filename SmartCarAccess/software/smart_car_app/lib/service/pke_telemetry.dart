import 'dart:convert';
import 'dart:math';
import 'package:flutter/foundation.dart';

class PkeTelemetrySchema {
  static const String version = 'pke.v1';
}

class PkeTelemetryEvent {
  static const String scanWake = 'scan_wake';
  static const String connect = 'connect';
  static const String auth0Received = 'auth0_received';
  static const String auth1Sent = 'auth1_sent';
  static const String authVerified = 'auth_verified';
  static const String controlFlowAck = 'control_flow_ack';
  static const String rssiSnapshot = 'rssi_snapshot';
  static const String unlockDecision = 'unlock_decision';
}

class PkeTelemetry {
  PkeTelemetry({this.source = 'app'});

  final String source;
  final Random _random = Random();
  int _attemptId = 0;

  int startAttempt({String? vehicleId}) {
    _attemptId = DateTime.now().millisecondsSinceEpoch * 1000 + _random.nextInt(1000);
    emit(
      event: PkeTelemetryEvent.scanWake,
      vehicleId: vehicleId,
      details: 'manual_test_trigger',
    );
    return _attemptId;
  }

  int get attemptId => _attemptId;

  void emit({
    required String event,
    String? vehicleId,
    int? rssiDbm,
    String? unlockDecision,
    String? details,
  }) {
    final payload = <String, dynamic>{
      'schema': PkeTelemetrySchema.version,
      'source': source,
      'attempt_id': _attemptId,
      'event': event,
      'ts_ms': DateTime.now().millisecondsSinceEpoch,
      'vehicle_id': vehicleId,
      'rssi_dbm': rssiDbm,
      'unlock_decision': unlockDecision,
      'details': details,
    };
    debugPrint('[PKE-TLM] ${jsonEncode(payload)}');
  }
}
