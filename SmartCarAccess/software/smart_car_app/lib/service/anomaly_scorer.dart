import 'dart:convert';
import 'ai_service.dart';

class Location {
  final double lat;
  final double lng;

  Location({required this.lat, required this.lng});

  factory Location.fromJson(Map<String, dynamic> json) {
    return Location(
      lat: json['lat'] as double,
      lng: json['lng'] as double,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'lat': lat,
      'lng': lng,
    };
  }
}

class AnomalyInput {
  final String timestamp;
  final int hour;
  final int weekday;
  final Location location;
  final double distanceFromUsual;
  final int accessCountLastHour;

  AnomalyInput({
    required this.timestamp,
    required this.hour,
    required this.weekday,
    required this.location,
    required this.distanceFromUsual,
    required this.accessCountLastHour,
  });

  factory AnomalyInput.fromJson(Map<String, dynamic> json) {
    return AnomalyInput(
      timestamp: json['timestamp'] as String,
      hour: json['hour'] as int,
      weekday: json['weekday'] as int,
      location: Location.fromJson(json['location'] as Map<String, dynamic>),
      distanceFromUsual: json['distance_from_usual'] as double,
      accessCountLastHour: json['access_count_last_hour'] as int,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'timestamp': timestamp,
      'hour': hour,
      'weekday': weekday,
      'location': location.toJson(),
      'distance_from_usual': distanceFromUsual,
      'access_count_last_hour': accessCountLastHour,
    };
  }
}

class AnomalyOutput {
  final bool isAnomalous;
  final double confidenceScore;
  final String reason;
  final String severity;
  final String action;
  final bool shouldNotify;

  AnomalyOutput({
    required this.isAnomalous,
    required this.confidenceScore,
    required this.reason,
    required this.severity,
    required this.action,
    required this.shouldNotify,
  });

  Map<String, dynamic> toJson() {
    return {
      'is_anomalous': isAnomalous,
      'confidence_score': confidenceScore,
      'reason': reason,
      'severity': severity,
      'action': action,
      'should_notify': shouldNotify,
    };
  }

  String toJsonString() {
    return jsonEncode(toJson());
  }
}

class AnomalyScorer {
  static Future<AnomalyOutput> evaluateAnomaly(AnomalyInput input) async {
    // Use AI service for anomaly detection
    return await AIService.detectAnomalyWithAI(input);
  }
}