import 'package:flutter/material.dart';
import 'package:smart_car_app/service/anomaly_detection_service.dart';
import 'package:smart_car_app/service/language_service.dart';
import 'package:smart_car_app/service/notification_service.dart';

/// Test widget to demonstrate anomaly detection notifications
class AnomalyDetectionTestWidget extends StatefulWidget {
  const AnomalyDetectionTestWidget({Key? key}) : super(key: key);

  @override
  State<AnomalyDetectionTestWidget> createState() => _AnomalyDetectionTestWidgetState();
}

class _AnomalyDetectionTestWidgetState extends State<AnomalyDetectionTestWidget> {
  late LanguageService _languageService;
  late NotificationService _notificationService;

  @override
  void initState() {
    super.initState();
    _languageService = LanguageService.instance;
    _notificationService = NotificationService.instance;
  }

  /// Create a test anomaly decision with medium severity
  AnomalyEnrichedDecision _createMediumSeverityDecision() {
    return AnomalyEnrichedDecision(
      isAnomalous: true,
      confidenceScore: 0.62,
      reason: 'unusual location and time',
      severity: 'medium',
      action: 'CONFIRM',
      shouldNotify: true,
      timestamp: DateTime.now(),
    );
  }

  /// Create a test anomaly decision with high severity
  AnomalyEnrichedDecision _createHighSeverityDecision() {
    return AnomalyEnrichedDecision(
      isAnomalous: true,
      confidenceScore: 0.85,
      reason: 'multiple risk factors detected',
      severity: 'high',
      action: 'BLOCK',
      shouldNotify: true,
      timestamp: DateTime.now(),
    );
  }

  /// Show a test notification
  void _showTestNotification(AnomalyEnrichedDecision decision) {
    _notificationService.showNotification(
      context,
      decision.notificationTitle,
      decision.notificationBody,
      decision.severity,
    );
  }

  /// Show a test blocking dialog (for high severity)
  Future<void> _showTestBlockingDialog(AnomalyEnrichedDecision decision) async {
    if (decision.severity == 'high') {
      final result = await _notificationService.showBlockingDialog(
        context,
        decision.notificationTitle,
        decision.notificationBody,
      );
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(
            result == true ? 'Access Allowed' : 'Access Blocked',
            style: const TextStyle(color: Colors.white),
          ),
          backgroundColor: result == true ? Colors.green : Colors.red,
        ),
      );
    }
  }

  /// Switch language
  Future<void> _switchLanguage() async {
    final newLanguage = _languageService.currentLanguage == 'en' ? 'vi' : 'en';
    await _languageService.setLanguage(newLanguage);
    setState(() {});
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(
            newLanguage == 'en'
                ? 'Language changed to English'
                : 'Ngôn ngữ đã thay đổi sang Tiếng Việt',
          ),
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Anomaly Detection - Notifications Test'),
        actions: [
          IconButton(
            icon: const Icon(Icons.language),
            onPressed: _switchLanguage,
            tooltip:
                _languageService.currentLanguage == 'en' ? 'Switch to Vietnamese' : 'Switch to English',
          ),
        ],
      ),
      body: SingleChildScrollView(
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                'Current Language: ${_languageService.currentLanguage.toUpperCase()}',
                style: Theme.of(context).textTheme.titleLarge,
              ),
              const SizedBox(height: 24),
              _buildNotificationPreviewSection(),
              const SizedBox(height: 24),
              _buildTestButtonsSection(),
              const SizedBox(height: 24),
              _buildDecisionDisplaySection(),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildNotificationPreviewSection() {
    final mediumDecision = _createMediumSeverityDecision();
    final highDecision = _createHighSeverityDecision();

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Notification Previews',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 16),
            _buildNotificationCard('MEDIUM SEVERITY', mediumDecision, Colors.orange),
            const SizedBox(height: 16),
            _buildNotificationCard('HIGH SEVERITY', highDecision, Colors.red),
          ],
        ),
      ),
    );
  }

  Widget _buildNotificationCard(String label, AnomalyEnrichedDecision decision, Color color) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: color.withOpacity(0.1),
        border: Border.all(color: color),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            label,
            style: TextStyle(fontWeight: FontWeight.bold, color: color),
          ),
          const SizedBox(height: 8),
          Text(
            'Title: ${decision.notificationTitle}',
            style: const TextStyle(fontWeight: FontWeight.bold),
          ),
          const SizedBox(height: 4),
          Text('Body: ${decision.notificationBody}'),
          const SizedBox(height: 4),
          Text('Action: ${decision.action}'),
        ],
      ),
    );
  }

  Widget _buildTestButtonsSection() {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Test Notifications',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 16),
            SizedBox(
              width: double.infinity,
              child: ElevatedButton(
                style: ElevatedButton.styleFrom(backgroundColor: Colors.orange),
                onPressed: () => _showTestNotification(_createMediumSeverityDecision()),
                child: const Text('Show Medium Severity Notification'),
              ),
            ),
            const SizedBox(height: 8),
            SizedBox(
              width: double.infinity,
              child: ElevatedButton(
                style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
                onPressed: () => _showTestNotification(_createHighSeverityDecision()),
                child: const Text('Show High Severity Notification'),
              ),
            ),
            const SizedBox(height: 8),
            SizedBox(
              width: double.infinity,
              child: ElevatedButton(
                style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
                onPressed: () => _showTestBlockingDialog(_createHighSeverityDecision()),
                child: const Text('Show High Severity Dialog'),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildDecisionDisplaySection() {
    final mediumDecision = _createMediumSeverityDecision();
    final highDecision = _createHighSeverityDecision();

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Decision Objects (JSON)',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 16),
            _buildJsonDisplay('Medium Severity', mediumDecision),
            const SizedBox(height: 16),
            _buildJsonDisplay('High Severity', highDecision),
          ],
        ),
      ),
    );
  }

  Widget _buildJsonDisplay(String title, AnomalyEnrichedDecision decision) {
    final json = decision.toJson();
    final jsonString = json.entries
        .map((e) => '  "${e.key}": ${e.value is String ? '"${e.value}"' : e.value}')
        .join(',\n');

    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Colors.grey[100],
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: Colors.grey[300]!),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            title,
            style: const TextStyle(fontWeight: FontWeight.bold),
          ),
          const SizedBox(height: 8),
          SingleChildScrollView(
            scrollDirection: Axis.horizontal,
            child: Text(
              '{\n$jsonString\n}',
              style: const TextStyle(
                fontFamily: 'Courier',
                fontSize: 11,
              ),
            ),
          ),
        ],
      ),
    );
  }
}
