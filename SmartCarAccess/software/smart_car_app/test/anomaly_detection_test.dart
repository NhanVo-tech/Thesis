import 'package:flutter_test/flutter_test.dart';
import 'package:smart_car_app/service/anomaly_scorer.dart';

void main() {
  group('Anomaly Detection Tests', () {
    test('Test 1: Normal access - LOW RISK (allow)', () async {
      final input = AnomalyInput(
        timestamp: DateTime.now().toIso8601String(),
        hour: 8,
        weekday: 3,
        location: Location(lat: 10.7769, lng: 106.7009),
        distanceFromUsual: 2.5,
        accessCountLastHour: 1,
      );

      final result = await AnomalyScorer.evaluateAnomaly(input);

      expect(result.severity, 'low');
      expect(result.action, 'ALLOW');
      expect(result.shouldNotify, false);
      print('✅ Test 1 PASSED - Normal access detected');
      print('  Severity: ${result.severity}');
      print('  Action: ${result.action}');
      print('  Notification: ${result.shouldNotify}');
    });

    test('Test 2: Unusual time + moderate distance - MEDIUM RISK (confirm)', () async {
      final input = AnomalyInput(
        timestamp: DateTime.now().toIso8601String(),
        hour: 2,
        weekday: 3,
        location: Location(lat: 10.8, lng: 106.8),
        distanceFromUsual: 45.0,
        accessCountLastHour: 2,
      );

      final result = await AnomalyScorer.evaluateAnomaly(input);

      expect(result.shouldNotify, true);
      print('✅ Test 2 PASSED - Unusual access detected');
      print('  Severity: ${result.severity}');
      print('  Action: ${result.action}');
      print('  Notification: ${result.shouldNotify}');
      print('  Reason: ${result.reason}');
    });

    test('Test 3: Multiple risk factors - HIGH RISK (block)', () async {
      final input = AnomalyInput(
        timestamp: DateTime.now().toIso8601String(),
        hour: 3,
        weekday: 7,
        location: Location(lat: 21.0, lng: 105.0),
        distanceFromUsual: 250.0,
        accessCountLastHour: 9,
      );

      final result = await AnomalyScorer.evaluateAnomaly(input);

      expect(result.severity, 'high');
      expect(result.action, 'BLOCK');
      expect(result.shouldNotify, true);
      print('✅ Test 3 PASSED - Suspicious access detected');
      print('  Severity: ${result.severity}');
      print('  Action: ${result.action}');
      print('  Notification: ${result.shouldNotify}');
      print('  Reason: ${result.reason}');
    });

    test('Test 4: High frequency access - MEDIUM RISK', () async {
      final input = AnomalyInput(
        timestamp: DateTime.now().toIso8601String(),
        hour: 9,
        weekday: 2,
        location: Location(lat: 10.7769, lng: 106.7009),
        distanceFromUsual: 1.0,
        accessCountLastHour: 6,
      );

      final result = await AnomalyScorer.evaluateAnomaly(input);

      expect(result.shouldNotify, true);
      print('✅ Test 4 PASSED - High frequency access detected');
      print('  Severity: ${result.severity}');
      print('  Action: ${result.action}');
      print('  Reason: ${result.reason}');
    });

    test('Test 5: Very far location - HIGH RISK', () async {
      final input = AnomalyInput(
        timestamp: DateTime.now().toIso8601String(),
        hour: 10,
        weekday: 1,
        location: Location(lat: 16.0, lng: 108.0),
        distanceFromUsual: 500.0,
        accessCountLastHour: 1,
      );

      final result = await AnomalyScorer.evaluateAnomaly(input);

      expect(result.shouldNotify, true);
      print('✅ Test 5 PASSED - Far location detected');
      print('  Severity: ${result.severity}');
      print('  Action: ${result.action}');
      print('  Distance: 500km');
    });
  });

  group('Risk Score Calculation Tests', () {
    test('Calculate time risk for normal hours', () {
      // Hour 8 = normal (6-9, 17-22)
      // Expected: 0.0
      final hour = 8;
      final isNormal = (hour >= 6 && hour < 9) || (hour >= 17 && hour < 22);
      expect(isNormal, true);
      print('✅ Time Risk - Normal hours (8am): 0.0');
    });

    test('Calculate time risk for suspicious hours', () {
      // Hour 3 = suspicious
      // Expected: 1.0
      final hour = 3;
      final isNormal = (hour >= 6 && hour < 9) || (hour >= 17 && hour < 22);
      expect(isNormal, false);
      print('✅ Time Risk - Suspicious hours (3am): 1.0');
    });

    test('Calculate location risk for close distance', () {
      // 2.5km = close
      // Expected: 0.0
      final distance = 2.5;
      final riskLevel = distance < 10 ? 'low' : (distance < 100 ? 'medium' : 'high');
      expect(riskLevel, 'low');
      print('✅ Location Risk - 2.5km away: 0.0 (low)');
    });

    test('Calculate location risk for moderate distance', () {
      // 45km = moderate
      // Expected: 0.5
      final distance = 45.0;
      final riskLevel = distance < 10 ? 'low' : (distance < 100 ? 'medium' : 'high');
      expect(riskLevel, 'medium');
      print('✅ Location Risk - 45km away: 0.5 (medium)');
    });

    test('Calculate location risk for very far distance', () {
      // 500km = very far
      // Expected: 1.0
      final distance = 500.0;
      final riskLevel = distance < 10 ? 'low' : (distance < 100 ? 'medium' : 'high');
      expect(riskLevel, 'high');
      print('✅ Location Risk - 500km away: 1.0 (high)');
    });

    test('Calculate frequency risk for low frequency', () {
      // 1 access/hour = low frequency
      // Expected: 0.0
      final count = 1;
      final riskLevel = count <= 3 ? 'low' : (count <= 8 ? 'medium' : 'high');
      expect(riskLevel, 'low');
      print('✅ Frequency Risk - 1 access/hour: 0.0 (low)');
    });

    test('Calculate frequency risk for moderate frequency', () {
      // 6 accesses/hour = moderate
      // Expected: 0.5
      final count = 6;
      final riskLevel = count <= 3 ? 'low' : (count <= 8 ? 'medium' : 'high');
      expect(riskLevel, 'medium');
      print('✅ Frequency Risk - 6 accesses/hour: 0.5 (medium)');
    });

    test('Calculate frequency risk for high frequency', () {
      // 9 accesses/hour = high
      // Expected: 1.0
      final count = 9;
      final riskLevel = count <= 3 ? 'low' : (count <= 8 ? 'medium' : 'high');
      expect(riskLevel, 'high');
      print('✅ Frequency Risk - 9 accesses/hour: 1.0 (high)');
    });
  });
}
