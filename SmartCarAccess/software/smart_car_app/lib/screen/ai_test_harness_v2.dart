import 'package:flutter/material.dart';
import 'package:smart_car_app/service/ai_service.dart';
import 'package:smart_car_app/service/anomaly_scorer.dart';
import 'package:smart_car_app/service/notification_service.dart';
import 'package:smart_car_app/service/language_service.dart';
import 'package:smart_car_app/service/push_notification_service.dart';
import 'package:smart_car_app/screen/location.dart';

/// Clean and minimal AI Anomaly Detection Test UI
class AITestHarnessV2 extends StatefulWidget {
  const AITestHarnessV2({Key? key}) : super(key: key);

  @override
  State<AITestHarnessV2> createState() => _AITestHarnessV2State();
}

class _AITestHarnessV2State extends State<AITestHarnessV2> {
  List<TestCaseV2> testCases = [];
  bool isLoading = false;

  @override
  void initState() {
    super.initState();
    _initializeTestCases();
  }

  void _initializeTestCases() {
    testCases = [
      TestCaseV2(
        id: 'test_01',
        name: 'Normal Access',
        description: '7 AM, within location, 1 access/hr',
        input: AnomalyInput(timestamp: '2024-04-27T07:30:00Z', hour: 7, weekday: 6, location: Location(lat: 10.7769, lng: 106.6966), distanceFromUsual: 5.0, accessCountLastHour: 1),
        expected: ('low', 'ALLOW'),
      ),
      TestCaseV2(
        id: 'test_02',
        name: 'Unusual Time + Distance',
        description: '2 AM, 45km away, 1 access/hr',
        input: AnomalyInput(timestamp: '2024-04-27T02:15:00Z', hour: 2, weekday: 6, location: Location(lat: 10.5768, lng: 106.3966), distanceFromUsual: 45.0, accessCountLastHour: 1),
        expected: ('medium', 'CONFIRM'),
      ),
      TestCaseV2(
        id: 'test_03',
        name: 'Multiple Risk Factors',
        description: '1 AM, 500km away, 7 accesses/hr',
        input: AnomalyInput(timestamp: '2024-04-27T01:00:00Z', hour: 1, weekday: 6, location: Location(lat: 8.0, lng: 110.0), distanceFromUsual: 500.0, accessCountLastHour: 7),
        expected: ('high', 'BLOCK'),
      ),
      TestCaseV2(
        id: 'test_04',
        name: 'High Frequency Access',
        description: '9 AM, 2km away, 6 accesses/hr',
        input: AnomalyInput(timestamp: '2024-04-27T09:00:00Z', hour: 9, weekday: 5, location: Location(lat: 10.7769, lng: 106.6966), distanceFromUsual: 2.0, accessCountLastHour: 6),
        expected: ('medium', 'CONFIRM'),
      ),
      TestCaseV2(
        id: 'test_05',
        name: 'Business Hours Normal',
        description: '8 AM, 8km away, 2 accesses/hr',
        input: AnomalyInput(timestamp: '2024-04-27T08:00:00Z', hour: 8, weekday: 5, location: Location(lat: 10.7765, lng: 106.6960), distanceFromUsual: 8.0, accessCountLastHour: 2),
        expected: ('low', 'ALLOW'),
      ),
      TestCaseV2(
        id: 'test_06',
        name: 'Evening Far Location',
        description: '11 PM, 250km away, 1 access/hr',
        input: AnomalyInput(timestamp: '2024-04-27T23:00:00Z', hour: 23, weekday: 6, location: Location(lat: 12.0, lng: 108.5), distanceFromUsual: 250.0, accessCountLastHour: 1),
        expected: ('high', 'BLOCK'),
      ),
    ];
  }

  Future<void> _runTest(int index) async {
    setState(() => isLoading = true);
    try {
      final test = testCases[index];
      final response = await AIService.detectAnomalyWithAI(test.input);
      setState(() {
        test.response = (response.severity, response.action);
        test.confidence = response.confidenceScore;
        test.reason = response.reason;
        isLoading = false;
      });
      print('✅ ${test.id}: ${test.response}');

      // Show notification if severity is medium or high
      if (response.severity == 'medium' || response.severity == 'high') {
        final notificationService = NotificationService.instance;
        final title = notificationService.getNotificationTitle(response.severity);
        final body = notificationService.getNotificationBody(response.severity);
        
        debugPrint('📢 Sending notification: $title - $body');
        
        // Send system push notification (works even when app is closed)
        await PushNotificationService.instance.showNotification(
          title: title,
          body: body,
        );

        if (mounted) {
          // Show dialog for in-app interaction
          _showAnomalyConfirmationDialog(response.severity);
        }
      }
    } catch (e) {
      setState(() => isLoading = false);
      print('❌ ${testCases[index].id}: $e');
    }
  }

  void _showAnomalyConfirmationDialog(String severity) {
    final language = LanguageService.instance.currentLanguage;
    final notificationService = NotificationService.instance;
    final title = notificationService.getNotificationTitle(severity);
    final body = notificationService.getNotificationBody(severity);
    
    final isYouQuestion = language == 'vi' ? 'Đó có phải là bạn không?' : 'Is that you?';
    final notMeBtn = language == 'vi' ? 'Không phải tôi' : "It's not me";
    final isMeBtn = language == 'vi' ? 'Tôi' : "It's me";

    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) => AlertDialog(
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        backgroundColor: Colors.white,
        title: Text(
          title,
          style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
        ),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              body,
              style: const TextStyle(fontSize: 14, color: Colors.grey),
            ),
            const SizedBox(height: 16),
            Text(
              isYouQuestion,
              style: const TextStyle(
                fontSize: 14,
                fontWeight: FontWeight.bold,
                color: Colors.black,
              ),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () {
              Navigator.pop(context);
              _showLocationCheckDialog();
            },
            child: Text(
              notMeBtn,
              style: const TextStyle(color: Colors.red, fontSize: 14),
            ),
          ),
          TextButton(
            onPressed: () {
              Navigator.pop(context);
              _showPasswordChangeRecommendation();
            },
            child: Text(
              isMeBtn,
              style: const TextStyle(color: Colors.black, fontSize: 14),
            ),
          ),
        ],
      ),
    );
  }

  void _showLocationCheckDialog() {
    final language = LanguageService.instance.currentLanguage;
    final message = language == 'vi'
        ? 'Vui lòng kiểm tra khóa của bạn'
        : 'Please check your key';
    final okBtn = language == 'vi' ? 'Đồng ý' : 'OK';

    showDialog(
      context: context,
      barrierDismissible: true,
      builder: (dialogContext) => AlertDialog(
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        backgroundColor: Colors.white,
        icon: Icon(
          Icons.vpn_key,
          size: 48,
          color: Colors.blue[300],
        ),
        title: Text(
          message,
          textAlign: TextAlign.center,
          style: const TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.bold,
          ),
        ),
        actions: [
          Center(
            child: ElevatedButton(
              onPressed: () {
                Navigator.pop(dialogContext);
              },
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.blue[300],
              ),
              child: Text(
                okBtn,
                style: const TextStyle(color: Colors.white),
              ),
            ),
          ),
        ],
      ),
    );
  }

  void _showPasswordChangeRecommendation() {
    final language = LanguageService.instance.currentLanguage;
    final title = language == 'vi' ? 'Khuyến cáo bảo mật' : 'Security Recommendation';
    final message = language == 'vi'
        ? 'Hãy đổi mật khẩu để bảo vệ tài khoản của bạn khỏi truy cập trái phép'
        : 'Please change your password to protect your account from unauthorized access';
    final okBtn = language == 'vi' ? 'Đồng ý' : 'OK';

    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) => AlertDialog(
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        backgroundColor: Colors.white,
        title: Text(
          title,
          style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
        ),
        content: Text(
          message,
          style: const TextStyle(fontSize: 14, color: Colors.grey),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: Text(
              okBtn,
              style: const TextStyle(color: Colors.blue, fontSize: 14),
            ),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('AI Test Results'),
        backgroundColor: Colors.deepPurple,
        elevation: 0,
      ),
      body: SingleChildScrollView(
        child: Padding(
          padding: const EdgeInsets.all(12),
          child: Column(
            children: [
              _buildStats(),
              const SizedBox(height: 16),
              ListView.builder(
                shrinkWrap: true,
                physics: const NeverScrollableScrollPhysics(),
                itemCount: testCases.length,
                itemBuilder: (_, i) => _buildTestCard(testCases[i], i),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildStats() {
    final passed = testCases.where((t) => t.isPassed).length;
    final failed = testCases.where((t) => t.response != null && !t.isPassed).length;
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
      children: [
        _stat('PASS', passed.toString(), Colors.green),
        _stat('FAIL', failed.toString(), Colors.red),
        _stat('TOTAL', testCases.length.toString(), Colors.blue),
      ],
    );
  }

  Widget _stat(String label, String value, Color color) {
    return Column(
      children: [
        Text(value, style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold, color: color)),
        Text(label, style: const TextStyle(fontSize: 11, color: Colors.grey)),
      ],
    );
  }

  Widget _buildTestCard(TestCaseV2 test, int index) {
    final hasResponse = test.response != null;
    final isPassed = test.isPassed;

    return Card(
      margin: const EdgeInsets.only(bottom: 10),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
      child: InkWell(
        onTap: () => _runTest(index),
        child: Padding(
          padding: const EdgeInsets.all(12),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // Header
              Row(
                children: [
                  Icon(
                    hasResponse ? (isPassed ? Icons.check_circle : Icons.close_rounded) : Icons.hourglass_empty,
                    size: 20,
                    color: hasResponse ? (isPassed ? Colors.green : Colors.red) : Colors.grey,
                  ),
                  const SizedBox(width: 10),
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(test.name, style: const TextStyle(fontSize: 13, fontWeight: FontWeight.bold)),
                        Text(test.description, style: const TextStyle(fontSize: 11, color: Colors.grey), maxLines: 1, overflow: TextOverflow.ellipsis),
                      ],
                    ),
                  ),
                  if (isLoading)
                    const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(strokeWidth: 2))
                  else
                    ElevatedButton(onPressed: () => _runTest(index), child: const Text('Run', style: TextStyle(fontSize: 11)), style: ElevatedButton.styleFrom(padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4), backgroundColor: Colors.deepPurple)),
                ],
              ),
              
              // Results
              if (hasResponse) ...[
                const SizedBox(height: 12),
                Row(
                  children: [
                    Expanded(child: _resultBox('Expected', test.expected.$1, test.expected.$2)),
                    const SizedBox(width: 8),
                    Icon(isPassed ? Icons.check : Icons.close, color: isPassed ? Colors.green : Colors.red, size: 20),
                    const SizedBox(width: 8),
                    Expanded(child: _resultBox('Got', test.response!.$1, test.response!.$2)),
                  ],
                ),
                const SizedBox(height: 8),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text('Score: ${(test.confidence! * 100).toStringAsFixed(0)}%', style: const TextStyle(fontSize: 10, color: Colors.grey)),
                    Text(isPassed ? '✅ PASS' : '❌ FAIL', style: TextStyle(fontSize: 11, fontWeight: FontWeight.bold, color: isPassed ? Colors.green : Colors.red)),
                  ],
                ),
              ],
            ],
          ),
        ),
      ),
    );
  }

  Widget _resultBox(String label, String severity, String action) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.center,
      children: [
        Text(label, style: const TextStyle(fontSize: 9, color: Colors.grey)),
        const SizedBox(height: 4),
        Container(
          padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 3),
          decoration: BoxDecoration(color: _getSeverityColor(severity).withOpacity(0.2), borderRadius: BorderRadius.circular(4)),
          child: Text(severity, style: TextStyle(fontSize: 11, fontWeight: FontWeight.bold, color: _getSeverityColor(severity))),
        ),
        const SizedBox(height: 3),
        Text(action, style: const TextStyle(fontSize: 10), textAlign: TextAlign.center),
      ],
    );
  }

  Color _getSeverityColor(String severity) {
    return severity == 'high' ? Colors.red : severity == 'medium' ? Colors.orange : Colors.green;
  }
}

class TestCaseV2 {
  final String id;
  final String name;
  final String description;
  final AnomalyInput input;
  final (String, String) expected; // (severity, action)
  (String, String)? response;
  double? confidence;
  String? reason;

  bool get isPassed => response != null && response == expected;

  TestCaseV2({required this.id, required this.name, required this.description, required this.input, required this.expected});
}
