import 'dart:math';
import 'package:cloud_firestore/cloud_firestore.dart';
import 'time_anomaly_detector.dart';
import 'location_anomaly_detector.dart';
import 'anomaly_scorer.dart';
import 'notification_service.dart';

/// Main service for detecting anomalies in car access
class AnomalyDetectionService {
  static final AnomalyDetectionService _instance = AnomalyDetectionService._internal();
  static AnomalyDetectionService get instance => _instance;

  AnomalyDetectionService._internal();

  final TimeAnomalyDetector _timeDetector = TimeAnomalyDetector();
  final LocationAnomalyDetector _locationDetector = LocationAnomalyDetector();

  /// Analyze access event and detect anomalies
  Future<AnomalyAnalysisResult> analyzeAccessEvent(AccessEvent event) async {
    final timeResult = await _timeDetector.detectTimeAnomaly(event);
    final locationResult = await _locationDetector.detectLocationAnomaly(event);

    // Calculate total anomaly score
    final totalScore = timeResult.confidence + locationResult.confidence;
    final isAnomalous = timeResult.isAnomalous || locationResult.isAnomalous;
    AnomalySeverity overallSeverity;
    if (totalScore >= 1.0) {
      overallSeverity = AnomalySeverity.high;
    } else if (totalScore >= 0.6) {
      overallSeverity = AnomalySeverity.medium;
    } else if (totalScore >= 0.3) {
      overallSeverity = AnomalySeverity.low;
    } else {
      overallSeverity = AnomalySeverity.low;
    }

    // Log analysis results
    await _logAnalysis(event, timeResult, locationResult, totalScore, overallSeverity);

    return AnomalyAnalysisResult(
      isAnomalous: isAnomalous,
      confidence: totalScore,
      severity: overallSeverity,
      timeAnalysis: timeResult,
      locationAnalysis: locationResult,
      timestamp: DateTime.now(),
    );
  }

  /// Analyze access event with AI scoring and return enriched decision
  Future<AnomalyEnrichedDecision> analyzeAccessEventWithAI(AccessEvent event) async {
    final preprocessedInput = await _computePreprocessedFeatures(event);
    final aiResult = await AnomalyScorer.evaluateAnomaly(preprocessedInput);
    final enrichedDecision = AnomalyEnrichedDecision(
      isAnomalous: aiResult.isAnomalous,
      confidenceScore: aiResult.confidenceScore,
      reason: aiResult.reason,
      severity: aiResult.severity,
      action: aiResult.action,
      shouldNotify: aiResult.shouldNotify,
      timestamp: DateTime.now(),
    );
    await _logEnrichedDecision(event, aiResult, enrichedDecision);
    return enrichedDecision;
  }

  /// Compute preprocessed features for AI scorer
  Future<AnomalyInput> _computePreprocessedFeatures(AccessEvent event) async {
    final distanceFromUsual = await _calculateDistanceFromUsual(event);
    final accessCountLastHour = await _getAccessCountLastHour(event.userId, event.carId);

    return AnomalyInput(
      timestamp: event.timestamp.toIso8601String(),
      hour: event.timestamp.hour,
      weekday: event.timestamp.weekday,
      location: Location(
        lat: event.location?.latitude ?? 0.0,
        lng: event.location?.longitude ?? 0.0,
      ),
      distanceFromUsual: distanceFromUsual,
      accessCountLastHour: accessCountLastHour,
    );
  }

  /// Calculate distance from usual locations
  Future<double> _calculateDistanceFromUsual(AccessEvent event) async {
    if (event.location == null) return 0.0;

    try {
      final history = await FirebaseFirestore.instance
          .collection('anomaly_analysis')
          .where('userId', isEqualTo: event.userId)
          .orderBy('timestamp', descending: true)
          .limit(10)
          .get();

      if (history.docs.isEmpty) return 0.0;

      double minDistance = double.infinity;
      for (final doc in history.docs) {
        final loc = doc.data()['location'];
        if (loc != null) {
          final distance = _calculateHaversineDistance(
            event.location!.latitude,
            event.location!.longitude,
            loc['latitude'],
            loc['longitude'],
          );
          if (distance < minDistance) minDistance = distance;
        }
      }

      return minDistance.isInfinite ? 0.0 : minDistance;
    } catch (e) {
      print('Error calculating distance from usual: $e');
      return 0.0;
    }
  }

  /// Count number of accesses in the last hour
  Future<int> _getAccessCountLastHour(String userId, String carId) async {
    try {
      final oneHourAgo = DateTime.now().subtract(const Duration(hours: 1));
      final history = await FirebaseFirestore.instance
          .collection('anomaly_analysis')
          .where('userId', isEqualTo: userId)
          .where('carId', isEqualTo: carId)
          .where('timestamp', isGreaterThan: oneHourAgo)
          .get();

      return history.docs.length;
    } catch (e) {
      print('Error getting access count last hour: $e');
      return 0;
    }
  }

  /// Calculate Haversine distance between two points
  double _calculateHaversineDistance(double lat1, double lon1, double lat2, double lon2) {
    const double earthRadius = 6371; // km
    final double dLat = _degreesToRadians(lat2 - lat1);
    final double dLon = _degreesToRadians(lon2 - lon1);

    final double a = sin(dLat / 2) * sin(dLat / 2) +
        cos(_degreesToRadians(lat1)) * cos(_degreesToRadians(lat2)) * sin(dLon / 2) * sin(dLon / 2);
    final double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return earthRadius * c;
  }

  double _degreesToRadians(double degrees) => degrees * 3.141592653589793 / 180;

  /// Log analysis results to Firestore
  Future<void> _logAnalysis(
    AccessEvent event,
    AnomalyResult timeResult,
    AnomalyResult locationResult,
    double totalScore,
    AnomalySeverity severity,
  ) async {
    try {
      await FirebaseFirestore.instance.collection('anomaly_analysis').add({
        'userId': event.userId,
        'carId': event.carId,
        'timestamp': FieldValue.serverTimestamp(),
        'eventTimestamp': event.timestamp,
        'location': event.location != null ? {
          'latitude': event.location!.latitude,
          'longitude': event.location!.longitude,
        } : null,
        'deviceInfo': event.deviceInfo,
        'timeAnalysis': {
          'isAnomalous': timeResult.isAnomalous,
          'confidence': timeResult.confidence,
          'reason': timeResult.reason,
          'severity': timeResult.severity.toString(),
        },
        'locationAnalysis': {
          'isAnomalous': locationResult.isAnomalous,
          'confidence': locationResult.confidence,
          'reason': locationResult.reason,
          'severity': locationResult.severity.toString(),
        },
        'overallScore': totalScore,
        'overallSeverity': severity.toString(),
        'isAnomalous': timeResult.isAnomalous || locationResult.isAnomalous,
      });
    } catch (e) {
      print('Error logging anomaly analysis: $e');
    }
  }

  /// Get user's anomaly analysis history
  Future<List<AnomalyAnalysisResult>> getUserAnomalyHistory(
    String userId, {
    int limit = 50,
  }) async {
    try {
      final query = await FirebaseFirestore.instance
          .collection('anomaly_analysis')
          .where('userId', isEqualTo: userId)
          .orderBy('timestamp', descending: true)
          .limit(limit)
          .get();

      return query.docs.map((doc) {
        final data = doc.data();
        return AnomalyAnalysisResult(
          isAnomalous: data['isAnomalous'] ?? false,
          confidence: (data['overallScore'] ?? 0.0).toDouble(),
          severity: _parseSeverity(data['overallSeverity']),
          timeAnalysis: AnomalyResult(
            isAnomalous: data['timeAnalysis']['isAnomalous'] ?? false,
            confidence: (data['timeAnalysis']['confidence'] ?? 0.0).toDouble(),
            reason: data['timeAnalysis']['reason'] ?? '',
            severity: _parseSeverity(data['timeAnalysis']['severity']),
            carId: data['carId'] ?? '',
            userId: data['userId'] ?? '',
          ),
          locationAnalysis: AnomalyResult(
            isAnomalous: data['locationAnalysis']['isAnomalous'] ?? false,
            confidence: (data['locationAnalysis']['confidence'] ?? 0.0).toDouble(),
            reason: data['locationAnalysis']['reason'] ?? '',
            severity: _parseSeverity(data['locationAnalysis']['severity']),
            carId: data['carId'] ?? '',
            userId: data['userId'] ?? '',
          ),
          timestamp: (data['timestamp'] as Timestamp?)?.toDate() ?? DateTime.now(),
        );
      }).toList();
    } catch (e) {
      print('Error getting user anomaly history: $e');
      return [];
    }
  }

  /// Parse severity from string
  AnomalySeverity _parseSeverity(String? severity) {
    switch (severity) {
      case 'AnomalySeverity.high':
        return AnomalySeverity.high;
      case 'AnomalySeverity.medium':
        return AnomalySeverity.medium;
      case 'AnomalySeverity.low':
      default:
        return AnomalySeverity.low;
    }
  }

  /// Log enriched AI decision with notifications to Firestore
  Future<void> _logEnrichedDecision(
    AccessEvent event,
    AnomalyOutput aiResult,
    AnomalyEnrichedDecision decision,
  ) async {
    try {
      await FirebaseFirestore.instance.collection('anomaly_decisions').add({
        'userId': event.userId,
        'carId': event.carId,
        'timestamp': FieldValue.serverTimestamp(),
        'eventTimestamp': event.timestamp,
        'location': event.location != null ? {
          'latitude': event.location!.latitude,
          'longitude': event.location!.longitude,
        } : null,
        'aiResult': aiResult.toJson(),
        'decision': decision.toJson(),
      });
    } catch (e) {
      print('Error logging enriched decision: $e');
    }
  }
}

/// Overall anomaly analysis result
class AnomalyAnalysisResult {
  final bool isAnomalous;
  final double confidence;
  final AnomalySeverity severity;
  final AnomalyResult timeAnalysis;
  final AnomalyResult locationAnalysis;
  final DateTime timestamp;

  AnomalyAnalysisResult({
    required this.isAnomalous,
    required this.confidence,
    required this.severity,
    required this.timeAnalysis,
    required this.locationAnalysis,
    required this.timestamp,
  });
}

/// Enriched decision with hard-coded notifications based on severity
class AnomalyEnrichedDecision {
  final bool isAnomalous;
  final double confidenceScore;
  final String reason;
  final String severity;
  final String action;
  final bool shouldNotify;
  final DateTime timestamp;

  AnomalyEnrichedDecision({
    required this.isAnomalous,
    required this.confidenceScore,
    required this.reason,
    required this.severity,
    required this.action,
    required this.shouldNotify,
    required this.timestamp,
  });

  /// Whether to allow the access
  bool get isAllowed => action == 'ALLOW';

  /// Whether to require confirmation
  bool get requiresConfirmation => action == 'CONFIRM';

  /// Whether to block the access
  bool get isBlocked => action == 'BLOCK';

  /// Get hard-coded notification title based on severity (respects language setting)
  String get notificationTitle {
    if (!shouldNotify) return '';
    return NotificationService.instance.getNotificationTitle(severity);
  }

  /// Get hard-coded notification body based on severity (respects language setting)
  String get notificationBody {
    if (!shouldNotify) return '';
    return NotificationService.instance.getNotificationBody(severity);
  }

  Map<String, dynamic> toJson() {
    return {
      'is_anomalous': isAnomalous,
      'confidence_score': confidenceScore,
      'reason': reason,
      'severity': severity,
      'action': action,
      'should_notify': shouldNotify,
      'notification_title': notificationTitle,
      'notification_body': notificationBody,
      'timestamp': timestamp.toIso8601String(),
    };
  }
}