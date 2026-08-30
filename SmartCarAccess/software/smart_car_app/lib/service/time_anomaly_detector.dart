import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:geolocator/geolocator.dart';

/// Model for anomaly detection results
class AnomalyResult {
  final bool isAnomalous;
  final double confidence;
  final String reason;
  final AnomalySeverity severity;
  final String carId;
  final String userId;

  AnomalyResult({
    required this.isAnomalous,
    required this.confidence,
    required this.reason,
    required this.severity,
    required this.carId,
    required this.userId,
  });
}

/// Severity level of anomaly
enum AnomalySeverity {
  low,
  medium,
  high,
}

/// Access event for analysis
class AccessEvent {
  final String userId;
  final String carId;
  final DateTime timestamp;
  final Position? location;
  final Map<String, dynamic>? deviceInfo;

  AccessEvent({
    required this.userId,
    required this.carId,
    required this.timestamp,
    this.location,
    this.deviceInfo,
  });
}

/// Detect anomalies based on time
class TimeAnomalyDetector {
  static const List<int> normalHours = [6, 7, 8, 9, 10, 11, 12, 17, 18, 19, 20, 21, 22];
  static const List<int> suspiciousHours = [1, 2, 3, 4, 5, 23, 0];

  /// Check for unusual access time
  Future<AnomalyResult> detectTimeAnomaly(AccessEvent event) async {
    final hour = event.timestamp.hour;
    final weekday = event.timestamp.weekday; // 1 = Monday, 7 = Sunday

    double score = 0.0;
    String reason = '';

    // 1. Check for unusual hours
    if (suspiciousHours.contains(hour)) {
      score += 0.4;
      reason += 'Truy cập vào giờ bất thường (${hour}h). ';
    } else if (!normalHours.contains(hour)) {
      score += 0.2;
      reason += 'Truy cập ngoài giờ thông thường (${hour}h). ';
    }

    // 2. Check for weekend days
    if (weekday == DateTime.saturday || weekday == DateTime.sunday) {
      // Get user's weekend access history
      final weekendHistory = await _getWeekendAccessHistory(event.userId);
      if (weekendHistory < 3) { // Less than 3 weekend accesses
        score += 0.3;
        reason += 'Truy cập cuối tuần bất thường. ';
      }
    }

    // 3. Check for consecutive access times
    final recentAccesses = await _getRecentAccessCount(event.userId, event.carId);
    if (recentAccesses > 5) { // Nhiều hơn 5 lần trong 1 giờ
      score += 0.3;
      reason += 'Tần suất truy cập cao bất thường. ';
    }

    // Xác định mức độ nghiêm trọng
    AnomalySeverity severity;
    if (score >= 0.7) {
      severity = AnomalySeverity.high;
    } else if (score >= 0.4) {
      severity = AnomalySeverity.medium;
    } else if (score >= 0.2) {
      severity = AnomalySeverity.low;
    } else {
      severity = AnomalySeverity.low;
    }

    return AnomalyResult(
      isAnomalous: score > 0.3,
      confidence: score,
      reason: reason.trim(),
      severity: severity,
      carId: event.carId,
      userId: event.userId,
    );
  }

  /// Get user's weekend access count
  Future<int> _getWeekendAccessHistory(String userId) async {
    final now = DateTime.now();
    final thirtyDaysAgo = now.subtract(const Duration(days: 30));

    final query = await FirebaseFirestore.instance
        .collection('access_logs')
        .where('userId', isEqualTo: userId)
        .where('timestamp', isGreaterThan: thirtyDaysAgo)
        .get();

    int weekendCount = 0;
    for (var doc in query.docs) {
      final timestamp = (doc['timestamp'] as Timestamp).toDate();
      if (timestamp.weekday == DateTime.saturday || timestamp.weekday == DateTime.sunday) {
        weekendCount++;
      }
    }

    return weekendCount;
  }

  /// Get recent access count (within 1 hour)
  Future<int> _getRecentAccessCount(String userId, String carId) async {
    final now = DateTime.now();
    final oneHourAgo = now.subtract(const Duration(hours: 1));

    final query = await FirebaseFirestore.instance
        .collection('access_logs')
        .where('userId', isEqualTo: userId)
        .where('carId', isEqualTo: carId)
        .where('timestamp', isGreaterThan: oneHourAgo)
        .get();

    return query.docs.length;
  }
}