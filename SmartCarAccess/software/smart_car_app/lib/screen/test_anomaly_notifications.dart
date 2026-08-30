import 'package:flutter/material.dart';
import 'package:smart_car_app/service/anomaly_scorer.dart';
import 'package:smart_car_app/service/notification_service.dart';
import 'package:smart_car_app/service/language_service.dart';
import 'package:geolocator/geolocator.dart';
import 'package:smart_car_app/service/time_anomaly_detector.dart';

class TestAnomalyNotificationsScreen extends StatefulWidget {
  const TestAnomalyNotificationsScreen({Key? key}) : super(key: key);

  @override
  State<TestAnomalyNotificationsScreen> createState() =>
      _TestAnomalyNotificationsScreenState();
}

class _TestAnomalyNotificationsScreenState
    extends State<TestAnomalyNotificationsScreen> {
  String _result = 'Click a test button to run';
  bool _isLoading = false;

  Future<void> _testNormalAccess() async {
    setState(() => _isLoading = true);
    try {
      final input = AnomalyInput(
        timestamp: DateTime.now().toIso8601String(),
        hour: 8,
        weekday: 3,
        location: Location(lat: 10.7769, lng: 106.7009),
        distanceFromUsual: 2.5,
        accessCountLastHour: 1,
      );

      final result = await AnomalyScorer.evaluateAnomaly(input);

      setState(() {
        _result =
            'Test 1: NORMAL ACCESS\nSeverity: ${result.severity}\nAction: ${result.action}\nNotify: ${result.shouldNotify}\nReason: ${result.reason}';
      });

      if (mounted) {
        _showNotification(result.severity, 'Normal Access');
      }
    } catch (e) {
      setState(() => _result = 'Error: $e');
    } finally {
      setState(() => _isLoading = false);
    }
  }

  Future<void> _testMediumRisk() async {
    setState(() => _isLoading = true);
    try {
      final input = AnomalyInput(
        timestamp: DateTime.now().toIso8601String(),
        hour: 2,
        weekday: 3,
        location: Location(lat: 10.8, lng: 106.8),
        distanceFromUsual: 45.0,
        accessCountLastHour: 2,
      );

      final result = await AnomalyScorer.evaluateAnomaly(input);

      setState(() {
        _result =
            'Test 2: MEDIUM RISK (Unusual Time + Distance)\nSeverity: ${result.severity}\nAction: ${result.action}\nNotify: ${result.shouldNotify}\nReason: ${result.reason}';
      });

      if (mounted) {
        _showNotification(result.severity, 'Medium Risk Detected');
      }
    } catch (e) {
      setState(() => _result = 'Error: $e');
    } finally {
      setState(() => _isLoading = false);
    }
  }

  Future<void> _testHighRisk() async {
    setState(() => _isLoading = true);
    try {
      final input = AnomalyInput(
        timestamp: DateTime.now().toIso8601String(),
        hour: 3,
        weekday: 7,
        location: Location(lat: 21.0, lng: 105.0),
        distanceFromUsual: 250.0,
        accessCountLastHour: 9,
      );

      final result = await AnomalyScorer.evaluateAnomaly(input);

      setState(() {
        _result =
            'Test 3: HIGH RISK (Multiple Factors)\nSeverity: ${result.severity}\nAction: ${result.action}\nNotify: ${result.shouldNotify}\nReason: ${result.reason}';
      });

      if (mounted) {
        _showNotification(result.severity, 'High Risk - Access Blocked');
      }
    } catch (e) {
      setState(() => _result = 'Error: $e');
    } finally {
      setState(() => _isLoading = false);
    }
  }

  Future<void> _testHighFrequency() async {
    setState(() => _isLoading = true);
    try {
      final input = AnomalyInput(
        timestamp: DateTime.now().toIso8601String(),
        hour: 9,
        weekday: 2,
        location: Location(lat: 10.7769, lng: 106.7009),
        distanceFromUsual: 1.0,
        accessCountLastHour: 6,
      );

      final result = await AnomalyScorer.evaluateAnomaly(input);

      setState(() {
        _result =
            'Test 4: HIGH FREQUENCY (6 accesses/hour)\nSeverity: ${result.severity}\nAction: ${result.action}\nNotify: ${result.shouldNotify}\nReason: ${result.reason}';
      });

      if (mounted) {
        _showNotification(result.severity, 'High Frequency Access');
      }
    } catch (e) {
      setState(() => _result = 'Error: $e');
    } finally {
      setState(() => _isLoading = false);
    }
  }

  Future<void> _testFarLocation() async {
    setState(() => _isLoading = true);
    try {
      final input = AnomalyInput(
        timestamp: DateTime.now().toIso8601String(),
        hour: 10,
        weekday: 1,
        location: Location(lat: 16.0, lng: 108.0),
        distanceFromUsual: 500.0,
        accessCountLastHour: 1,
      );

      final result = await AnomalyScorer.evaluateAnomaly(input);

      setState(() {
        _result =
            'Test 5: VERY FAR LOCATION (500km)\nSeverity: ${result.severity}\nAction: ${result.action}\nNotify: ${result.shouldNotify}\nReason: ${result.reason}';
      });

      if (mounted) {
        _showNotification(result.severity, 'Impossible Location');
      }
    } catch (e) {
      setState(() => _result = 'Error: $e');
    } finally {
      setState(() => _isLoading = false);
    }
  }

  void _showNotification(String severity, String testName) {
    final notificationService = NotificationService.instance;
    final title = notificationService.getNotificationTitle(severity);
    final body = notificationService.getNotificationBody(severity);

    notificationService.showNotification(
      context,
      title,
      body,
      severity,
    );

    print('🔔 Notification shown for $testName');
    print('   Severity: $severity');
    print('   Title: $title');
    print('   Body: $body');
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Test Anomaly Notifications'),
        backgroundColor: Colors.blue[800],
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Card(
              color: Colors.blue[50],
              child: Padding(
                padding: const EdgeInsets.all(12),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text(
                      '📊 Test Results:',
                      style:
                          TextStyle(fontWeight: FontWeight.bold, fontSize: 14),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      _result,
                      style: const TextStyle(fontSize: 12, color: Colors.black87),
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 20),
            const Text(
              '🧪 Anomaly Detection Tests',
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 16),
            ),
            const SizedBox(height: 12),
            _buildTestButton(
              'Test 1: Normal Access',
              '✅ Low Risk - Should Allow',
              Colors.green,
              _testNormalAccess,
            ),
            const SizedBox(height: 8),
            _buildTestButton(
              'Test 2: Unusual Time + Distance',
              '⚠️ Medium Risk - Should Confirm',
              Colors.orange,
              _testMediumRisk,
            ),
            const SizedBox(height: 8),
            _buildTestButton(
              'Test 3: Multiple Risk Factors',
              '🚨 High Risk - Should Block',
              Colors.red,
              _testHighRisk,
            ),
            const SizedBox(height: 8),
            _buildTestButton(
              'Test 4: High Frequency (6/hour)',
              '⚠️ Medium Risk - Frequency',
              Colors.orange,
              _testHighFrequency,
            ),
            const SizedBox(height: 8),
            _buildTestButton(
              'Test 5: Very Far Location (500km)',
              '🚨 High Risk - Impossible',
              Colors.red,
              _testFarLocation,
            ),
            const SizedBox(height: 20),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Colors.grey[200],
                borderRadius: BorderRadius.circular(8),
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'ℹ️ About Tests',
                    style: TextStyle(fontWeight: FontWeight.bold),
                  ),
                  const SizedBox(height: 8),
                  const Text(
                    '• Each test creates a dummy AnomalyInput\n'
                    '• Tests different risk scenarios\n'
                    '• Notifications shown in SnackBar\n'
                    '• Messages follow language settings\n'
                    '• Check console for detailed output',
                    style: TextStyle(fontSize: 12),
                  ),
                  const SizedBox(height: 8),
                  Text(
                    'Current Language: ${LanguageService.instance.currentLanguage.toUpperCase()}',
                    style: const TextStyle(
                      fontSize: 12,
                      fontWeight: FontWeight.bold,
                      color: Colors.blue,
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildTestButton(
    String title,
    String subtitle,
    Color color,
    VoidCallback onPressed,
  ) {
    return ElevatedButton(
      onPressed: _isLoading ? null : onPressed,
      style: ElevatedButton.styleFrom(
        backgroundColor: color,
        disabledBackgroundColor: Colors.grey[300],
        padding: const EdgeInsets.symmetric(vertical: 12),
      ),
      child: Column(
        children: [
          Text(
            title,
            style: const TextStyle(
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          Text(
            subtitle,
            style: const TextStyle(
              fontSize: 12,
              color: Colors.white70,
            ),
          ),
        ],
      ),
    );
  }
}
