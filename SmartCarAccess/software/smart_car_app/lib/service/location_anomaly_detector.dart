import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:geolocator/geolocator.dart';
import 'dart:math';
import 'time_anomaly_detector.dart';

/// Detect anomalies based on geographic location
class LocationAnomalyDetector {
  static const double maxNormalDistance = 50000; // 50km
  static const int minHistoryPoints = 3;

  /// Check for unusual access location
  Future<AnomalyResult> detectLocationAnomaly(AccessEvent event) async {
    if (event.location == null) {
      return AnomalyResult(
        isAnomalous: false,
        confidence: 0.0,
        reason: 'Không có thông tin vị trí',
        severity: AnomalySeverity.low,
        carId: event.carId,
        userId: event.userId,
      );
    }

    double score = 0.0;
    String reason = '';

    // 1. Get user's access location history
    final accessHistory = await _getUserAccessHistory(event.userId);

    if (accessHistory.isEmpty) {
      // User mới, không có lịch sử
      return AnomalyResult(
        isAnomalous: false,
        confidence: 0.1,
        reason: 'New user, no access history yet',
        severity: AnomalySeverity.low,
        carId: event.carId,
        userId: event.userId,
      );
    }

    // 2. Calculate distance to familiar locations
    final distances = accessHistory.map((history) {
      return _calculateDistance(
        event.location!,
        Position(
          latitude: history['latitude'],
          longitude: history['longitude'],
          timestamp: DateTime.now(),
          accuracy: 0,
          altitude: 0,
          heading: 0,
          speed: 0,
          speedAccuracy: 0,
          altitudeAccuracy: 0,
          headingAccuracy: 0,
        ),
      );
    }).toList();

    final minDistance = distances.reduce(min);
    final avgDistance = distances.reduce((a, b) => a + b) / distances.length;

    // 3. Analyze distance
    if (minDistance > maxNormalDistance) {
      score += 0.5;
      reason += 'Vị trí cách xa tất cả vị trí quen thuộc (>50km). ';
    } else if (avgDistance > maxNormalDistance * 0.5) {
      score += 0.3;
      reason += 'Vị trí khá xa các vị trí quen thuộc. ';
    }

    // 4. Check location entropy (location diversity)
    final locationEntropy = _calculateLocationEntropy(accessHistory);
    if (locationEntropy > 2.0) { // Entropy cao = nhiều vị trí khác nhau
      score += 0.2;
      reason += 'User has many diverse access locations. ';
    }

    // 5. Check for unusual movement speed
    final speedAnomaly = await _checkSpeedAnomaly(event, accessHistory);
    if (speedAnomaly.isAnomalous) {
      score += speedAnomaly.confidence;
      reason += speedAnomaly.reason;
    }

    // 6. Check familiar geographic regions
    final regionAnomaly = await _checkRegionAnomaly(event.location!, accessHistory);
    if (regionAnomaly.isAnomalous) {
      score += regionAnomaly.confidence;
      reason += regionAnomaly.reason;
    }

    // Xác định mức độ nghiêm trọng
    AnomalySeverity severity;
    if (score >= 0.8) {
      severity = AnomalySeverity.high;
    } else if (score >= 0.5) {
      severity = AnomalySeverity.medium;
    } else if (score >= 0.2) {
      severity = AnomalySeverity.low;
    } else {
      severity = AnomalySeverity.low;
    }

    return AnomalyResult(
      isAnomalous: score > 0.4,
      confidence: score,
      reason: reason.trim(),
      severity: severity,
      carId: event.carId,
      userId: event.userId,
    );
  }

  /// Get user's location history (last 30 days)
  Future<List<Map<String, dynamic>>> _getUserAccessHistory(String userId) async {
    final now = DateTime.now();
    final thirtyDaysAgo = now.subtract(const Duration(days: 30));

    final query = await FirebaseFirestore.instance
        .collection('access_logs')
        .where('userId', isEqualTo: userId)
        .where('timestamp', isGreaterThan: thirtyDaysAgo)
        .where('location', isNotEqualTo: null)
        .limit(100) // Giới hạn 100 bản ghi gần nhất
        .get();

    return query.docs.map((doc) {
      final data = doc.data();
      return {
        'latitude': data['location']['latitude'],
        'longitude': data['location']['longitude'],
        'timestamp': (data['timestamp'] as Timestamp).toDate(),
      };
    }).toList();
  }

  /// Calculate distance between 2 locations (Haversine formula)
  double _calculateDistance(Position pos1, Position pos2) {
    const double earthRadius = 6371000; // Bán kính Trái Đất (m)

    final lat1Rad = pos1.latitude * pi / 180;
    final lat2Rad = pos2.latitude * pi / 180;
    final deltaLatRad = (pos2.latitude - pos1.latitude) * pi / 180;
    final deltaLngRad = (pos2.longitude - pos1.longitude) * pi / 180;

    final a = sin(deltaLatRad / 2) * sin(deltaLatRad / 2) +
        cos(lat1Rad) * cos(lat2Rad) *
        sin(deltaLngRad / 2) * sin(deltaLngRad / 2);

    final c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return earthRadius * c; // Khoảng cách (m)
  }

  /// Calculate location entropy (measure of location diversity)
  double _calculateLocationEntropy(List<Map<String, dynamic>> history) {
    if (history.length < 2) return 0.0;

    // Gom nhóm vị trí theo vùng (khoảng 1km)
    final regions = <String>{};
    for (var location in history) {
      final lat = (location['latitude'] * 100).round() / 100; // Làm tròn 1km
      final lng = (location['longitude'] * 100).round() / 100;
      regions.add('$lat,$lng');
    }

    final totalLocations = history.length;
    final uniqueRegions = regions.length;

    // Entropy = -∑(p_i * log2(p_i))
    double entropy = 0.0;
    final probability = uniqueRegions / totalLocations;

    if (probability > 0) {
      entropy = -probability * (log(probability) / log(2));
    }

    return entropy;
  }

  /// Check for unusual movement speed
  Future<AnomalyResult> _checkSpeedAnomaly(
    AccessEvent event,
    List<Map<String, dynamic>> history,
  ) async {
    if (history.isEmpty) {
      return AnomalyResult(
        isAnomalous: false,
        confidence: 0.0,
        reason: '',
        severity: AnomalySeverity.low,
        carId: event.carId,
        userId: event.userId,
      );
    }

    // Get most recent access
    final sortedHistory = history
        .where((h) => h['timestamp'] != null)
        .toList()
      ..sort((a, b) => (b['timestamp'] as DateTime).compareTo(a['timestamp'] as DateTime));

    if (sortedHistory.isEmpty) {
      return AnomalyResult(
        isAnomalous: false,
        confidence: 0.0,
        reason: '',
        severity: AnomalySeverity.low,
        carId: event.carId,
        userId: event.userId,
      );
    }

    final lastAccess = sortedHistory.first;
    final timeDiff = event.timestamp.difference(lastAccess['timestamp']);
    final distance = _calculateDistance(
      event.location!,
      Position(
        latitude: lastAccess['latitude'],
        longitude: lastAccess['longitude'],
        timestamp: DateTime.now(),
        accuracy: 0,
        altitude: 0,
        heading: 0,
        speed: 0,
        speedAccuracy: 0,
        altitudeAccuracy: 0,
        headingAccuracy: 0,
      ),
    );

    // Tính tốc độ (m/s)
    final speed = distance / timeDiff.inSeconds;

    // Unusual speed (> 200 km/h = 55.56 m/s)
    if (speed > 55.56) {
      return AnomalyResult(
        isAnomalous: true,
        confidence: 0.4,
        reason: 'Unusual movement speed (${(speed * 3.6).round()} km/h). ',
        severity: AnomalySeverity.medium,
        carId: event.carId,
        userId: event.userId,
      );
    }

    return AnomalyResult(
      isAnomalous: false,
      confidence: 0.0,
      reason: '',
      severity: AnomalySeverity.low,
      carId: event.carId,
      userId: event.userId,
    );
  }

  /// Check for unusual geographic regions
  Future<AnomalyResult> _checkRegionAnomaly(
    Position currentLocation,
    List<Map<String, dynamic>> history,
  ) async {
    // Analyze frequently accessed regions
    final regionCounts = <String, int>{};

    for (var location in history) {
      // Chia thành vùng 5km x 5km
      final latRegion = (location['latitude'] / 0.045).round(); // ~5km latitude
      final lngRegion = (location['longitude'] / 0.045).round(); // ~5km longitude
      final regionKey = '$latRegion,$lngRegion';

      regionCounts[regionKey] = (regionCounts[regionKey] ?? 0) + 1;
    }

    // Find most frequently accessed region
    final currentLatRegion = (currentLocation.latitude / 0.045).round();
    final currentLngRegion = (currentLocation.longitude / 0.045).round();
    final currentRegionKey = '$currentLatRegion,$currentLngRegion';

    final maxCount = regionCounts.values.reduce(max);
    final currentRegionCount = regionCounts[currentRegionKey] ?? 0;

    // If current region has less than 20% of accesses compared to most frequent region
    if (maxCount > 5 && currentRegionCount < maxCount * 0.2) {
      return AnomalyResult(
        isAnomalous: true,
        confidence: 0.3,
        reason: 'Access at unfamiliar geographic region. ',
        severity: AnomalySeverity.medium,
        carId: '', // Will be set by caller
        userId: '', // Will be set by caller
      );
    }

    return AnomalyResult(
      isAnomalous: false,
      confidence: 0.0,
      reason: '',
      severity: AnomalySeverity.low,
      carId: '', // Will be set by caller
      userId: '', // Will be set by caller
    );
  }
}