import 'package:flutter/material.dart';
import 'package:smart_car_app/service/ai_service.dart';
import 'package:smart_car_app/service/anomaly_scorer.dart';

/// UI Test Harness for AI Anomaly Detection
/// Displays pre-defined test cases with expected outputs
/// Sends to Gemini API and compares responses
class AITestHarness extends StatefulWidget {
  const AITestHarness({Key? key}) : super(key: key);

  @override
  State<AITestHarness> createState() => _AITestHarnessState();
}

class _AITestHarnessState extends State<AITestHarness> {
  List<TestCase> testCases = [];
  int selectedTestIndex = 0;
  bool isLoading = false;

  @override
  void initState() {
    super.initState();
    _initializeTestCases();
  }

  void _initializeTestCases() {
    testCases = [
      /// Test 1: Normal Access (Low Risk)
      TestCase(
        id: 'test_01',
        name: 'Normal Access - Low Risk',
        description: 'Early morning access (7 AM) within usual location',
        input: AnomalyInput(
          timestamp: '2024-04-27T07:30:00Z',
          hour: 7,
          weekday: 6,
          location: Location(lat: 10.7769, lng: 106.6966),
          distanceFromUsual: 5.0,
          accessCountLastHour: 1,
        ),
        displayTime: '07:30',
        expectedOutput: AnomalyOutput(
          isAnomalous: false,
          confidenceScore: 0.15,
          reason: 'normal access pattern',
          severity: 'low',
          action: 'ALLOW',
          shouldNotify: false,
        ),
      ),

      /// Test 2: Unusual Time + Distance (Medium Risk)
      TestCase(
        id: 'test_02',
        name: 'Unusual Time + Distance - Medium Risk',
        description: 'Midnight access (2 AM) from 45km away',
        input: AnomalyInput(
          timestamp: '2024-04-27T02:15:00Z',
          hour: 2,
          weekday: 6,
          location: Location(lat: 10.5768, lng: 106.3966),
          distanceFromUsual: 45.0,
          accessCountLastHour: 1,
        ),
        displayTime: '02:15',
        expectedOutput: AnomalyOutput(
          isAnomalous: true,
          confidenceScore: 0.58,
          reason: 'unusual location or time',
          severity: 'medium',
          action: 'CONFIRM',
          shouldNotify: true,
        ),
      ),

      /// Test 3: Multiple Risk Factors (High Risk)
      TestCase(
        id: 'test_03',
        name: 'Multiple Risk Factors - High Risk',
        description: 'Midnight access (1 AM) from 500km away, 7 accesses/hour',
        input: AnomalyInput(
          timestamp: '2024-04-27T01:00:00Z',
          hour: 1,
          weekday: 6,
          location: Location(lat: 8.0, lng: 110.0),
          distanceFromUsual: 500.0,
          accessCountLastHour: 7,
        ),
        displayTime: '01:00',
        expectedOutput: AnomalyOutput(
          isAnomalous: true,
          confidenceScore: 0.92,
          reason: 'multiple risk factors detected',
          severity: 'high',
          action: 'BLOCK',
          shouldNotify: true,
        ),
      ),

      /// Test 4: High Frequency Access (Medium Risk)
      TestCase(
        id: 'test_04',
        name: 'High Frequency - Medium Risk',
        description: 'Business hours access but 6 times in last hour',
        input: AnomalyInput(
          timestamp: '2024-04-27T09:00:00Z',
          hour: 9,
          weekday: 5,
          location: Location(lat: 10.7769, lng: 106.6966),
          distanceFromUsual: 2.0,
          accessCountLastHour: 6,
        ),
        displayTime: '09:00',
        expectedOutput: AnomalyOutput(
          isAnomalous: true,
          confidenceScore: 0.52,
          reason: 'unusual access frequency',
          severity: 'medium',
          action: 'CONFIRM',
          shouldNotify: true,
        ),
      ),

      /// Test 5: Business Hours + Normal Location (Low Risk)
      TestCase(
        id: 'test_05',
        name: 'Business Hours Normal Location - Low Risk',
        description: '8 AM access within 8km, 2 accesses/hour',
        input: AnomalyInput(
          timestamp: '2024-04-27T08:00:00Z',
          hour: 8,
          weekday: 5,
          location: Location(lat: 10.7765, lng: 106.6960),
          distanceFromUsual: 8.0,
          accessCountLastHour: 2,
        ),
        displayTime: '08:00',
        expectedOutput: AnomalyOutput(
          isAnomalous: false,
          confidenceScore: 0.22,
          reason: 'normal access pattern',
          severity: 'low',
          action: 'ALLOW',
          shouldNotify: false,
        ),
      ),

      /// Test 6: Evening Access Far Location (High Risk)
      TestCase(
        id: 'test_06',
        name: 'Evening Access Far Location - High Risk',
        description: '11 PM access from 250km away',
        input: AnomalyInput(
          timestamp: '2024-04-27T23:00:00Z',
          hour: 23,
          weekday: 6,
          location: Location(lat: 12.0, lng: 108.5),
          distanceFromUsual: 250.0,
          accessCountLastHour: 1,
        ),
        displayTime: '23:00',
        expectedOutput: AnomalyOutput(
          isAnomalous: true,
          confidenceScore: 0.78,
          reason: 'multiple risk factors detected',
          severity: 'high',
          action: 'BLOCK',
          shouldNotify: true,
        ),
      ),
    ];
  }

  Future<void> _runTest(int index) async {
    setState(() {
      isLoading = true;
      selectedTestIndex = index;
    });

    try {
      final testCase = testCases[index];
      final response = await AIService.detectAnomalyWithAI(testCase.input);

      setState(() {
        testCases[index].aiResponse = response;
        testCases[index].testStatus = _evaluateTestResult(testCases[index]);
        isLoading = false;
      });

      print('✅ Test ${testCase.id} completed');
      print('   AI Response: $response');
      print('   Status: ${testCases[index].testStatus}');
    } catch (e) {
      setState(() {
        testCases[index].testStatus = 'ERROR: $e';
        isLoading = false;
      });
      print('❌ Test ${testCases[index].id} failed: $e');
    }
  }

  String _evaluateTestResult(TestCase testCase) {
    if (testCase.aiResponse == null) return 'PENDING';

    final ai = testCase.aiResponse!;
    final expected = testCase.expectedOutput;

    bool isAnomalousMatch = ai.isAnomalous == expected.isAnomalous;
    bool severityMatch = ai.severity == expected.severity;
    bool actionMatch = ai.action == expected.action;
    bool notifyMatch = ai.shouldNotify == expected.shouldNotify;

    int matches = [isAnomalousMatch, severityMatch, actionMatch, notifyMatch]
        .where((e) => e)
        .length;

    // Detailed logging
    print('===== Test ${testCase.id} Details =====');
    print('Expected: isAnomalous=${expected.isAnomalous}, severity=${expected.severity}, action=${expected.action}, shouldNotify=${expected.shouldNotify}');
    print('AI Got:   isAnomalous=${ai.isAnomalous}, severity=${ai.severity}, action=${ai.action}, shouldNotify=${ai.shouldNotify}');
    print('Matches:  isAnomalous=$isAnomalousMatch, severity=$severityMatch, action=$actionMatch, shouldNotify=$notifyMatch');
    print('Reason: ${ai.reason}');
    print('Score: ${ai.confidenceScore}');
    print('================================\n');

    if (matches == 4) {
      return '✅ PASS (4/4 fields match)';
    } else if (matches >= 3) {
      return '⚠️  PARTIAL (${matches}/4 fields match)';
    } else {
      return '❌ FAIL (${matches}/4 fields match)';
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('AI Anomaly Detection Test Harness'),
        backgroundColor: Colors.deepPurple,
      ),
      body: SingleChildScrollView(
        child: Padding(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // Test List
              Card(
                child: ListView.builder(
                  shrinkWrap: true,
                  physics: const NeverScrollableScrollPhysics(),
                  itemCount: testCases.length,
                  itemBuilder: (context, index) {
                    final testCase = testCases[index];
                    return _buildTestCaseItem(context, testCase, index);
                  },
                ),
              ),
              const SizedBox(height: 20),

              // Detailed View of Selected Test
              if (selectedTestIndex >= 0 && selectedTestIndex < testCases.length)
                _buildDetailedTestView(),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildTestCaseItem(
      BuildContext context, TestCase testCase, int index) {
    final statusColor = _getStatusColor(testCase.testStatus);
    final isSelected = selectedTestIndex == index;

    return Container(
      color: isSelected ? Colors.purple.withOpacity(0.1) : Colors.transparent,
      child: ListTile(
        leading: CircleAvatar(
          backgroundColor: statusColor,
          child: Text(
            '${index + 1}',
            style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold),
          ),
        ),
        title: Text(
          testCase.name,
          style: const TextStyle(fontWeight: FontWeight.bold),
        ),
        subtitle: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(testCase.description, style: const TextStyle(fontSize: 12)),
            const SizedBox(height: 4),
            // Expected
            Text(
              'Expected: ${testCase.expectedOutput.severity.toUpperCase()}/${testCase.expectedOutput.action}',
              style: const TextStyle(fontSize: 11, color: Colors.grey),
            ),
            // AI Response
            if (testCase.aiResponse != null)
              Text(
                'AI Got: ${testCase.aiResponse!.severity.toUpperCase()}/${testCase.aiResponse!.action} (${(testCase.aiResponse!.confidenceScore * 100).toStringAsFixed(0)}%)',
                style: TextStyle(
                  fontSize: 11,
                  color: _getStatusColor(testCase.testStatus),
                  fontWeight: FontWeight.bold,
                ),
              ),
            if (testCase.testStatus != null)
              Text(
                testCase.testStatus!,
                style: TextStyle(
                  fontSize: 10,
                  color: statusColor,
                  fontWeight: FontWeight.bold,
                ),
              ),
          ],
        ),
        trailing: isLoading && selectedTestIndex == index
            ? const SizedBox(
                width: 24,
                height: 24,
                child: CircularProgressIndicator(strokeWidth: 2),
              )
            : ElevatedButton.icon(
                onPressed: isLoading ? null : () => _runTest(index),
                icon: const Icon(Icons.play_arrow),
                label: const Text('Run'),
                style: ElevatedButton.styleFrom(
                  backgroundColor: Colors.deepPurple,
                  foregroundColor: Colors.white,
                ),
              ),
        onTap: () {
          setState(() {
            selectedTestIndex = index;
          });
        },
      ),
    );
  }

  Widget _buildDetailedTestView() {
    final testCase = testCases[selectedTestIndex];

    return Card(
      elevation: 4,
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Test: ${testCase.name}',
              style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 16),

            // Input Data
            const Text(
              'INPUT',
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 14),
            ),
            const SizedBox(height: 8),
            _buildDataTable([
              ['Time', testCase.displayTime],
              ['Distance', '${testCase.input.distanceFromUsual} km'],
              ['Access Count/hr', '${testCase.input.accessCountLastHour}'],
              ['Location', '${testCase.input.location.lat.toStringAsFixed(4)}, ${testCase.input.location.lng.toStringAsFixed(4)}'],
            ]),
            const SizedBox(height: 16),

            // Expected Output
            const Text(
              'EXPECTED OUTPUT',
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 14),
            ),
            const SizedBox(height: 8),
            _buildDataTable([
              ['Is Anomalous', testCase.expectedOutput.isAnomalous ? 'YES' : 'NO'],
              ['Severity', testCase.expectedOutput.severity.toUpperCase()],
              ['Action', testCase.expectedOutput.action],
              ['Should Notify', testCase.expectedOutput.shouldNotify ? 'YES' : 'NO'],
              ['Confidence', '${(testCase.expectedOutput.confidenceScore * 100).toStringAsFixed(0)}%'],
            ]),
            const SizedBox(height: 16),

            // AI Response (if available)
            if (testCase.aiResponse != null) ...[
              const Text(
                'AI RESPONSE',
                style: TextStyle(fontWeight: FontWeight.bold, fontSize: 14, color: Colors.blue),
              ),
              const SizedBox(height: 8),
              _buildDataTable([
                ['Is Anomalous', testCase.aiResponse!.isAnomalous ? 'YES' : 'NO'],
                ['Severity', testCase.aiResponse!.severity.toUpperCase()],
                ['Action', testCase.aiResponse!.action],
                ['Should Notify', testCase.aiResponse!.shouldNotify ? 'YES' : 'NO'],
                ['Confidence', '${(testCase.aiResponse!.confidenceScore * 100).toStringAsFixed(0)}%'],
                ['Reason', testCase.aiResponse!.reason],
              ]),
              const SizedBox(height: 16),

              // Test Result
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: _getStatusColor(testCase.testStatus).withOpacity(0.1),
                  border: Border.all(color: _getStatusColor(testCase.testStatus)),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      testCase.testStatus ?? 'UNKNOWN',
                      style: TextStyle(
                        fontWeight: FontWeight.bold,
                        fontSize: 14,
                        color: _getStatusColor(testCase.testStatus),
                      ),
                    ),
                    const SizedBox(height: 8),
                    _buildComparisonRow('Is Anomalous', 
                      testCase.expectedOutput.isAnomalous == testCase.aiResponse!.isAnomalous),
                    _buildComparisonRow('Severity', 
                      testCase.expectedOutput.severity == testCase.aiResponse!.severity),
                    _buildComparisonRow('Action', 
                      testCase.expectedOutput.action == testCase.aiResponse!.action),
                    _buildComparisonRow('Should Notify', 
                      testCase.expectedOutput.shouldNotify == testCase.aiResponse!.shouldNotify),
                  ],
                ),
              ),
            ] else
              Center(
                child: Padding(
                  padding: const EdgeInsets.all(20.0),
                  child: Column(
                    children: [
                      Icon(Icons.hourglass_empty, size: 48, color: Colors.grey),
                      const SizedBox(height: 8),
                      const Text('Click "Run" to execute this test'),
                    ],
                  ),
                ),
              ),
          ],
        ),
      ),
    );
  }

  Widget _buildDataTable(List<List<String>> rows) {
    return Table(
      border: TableBorder.all(color: Colors.grey.shade300),
      children: rows.map((row) {
        return TableRow(
          children: [
            Padding(
              padding: const EdgeInsets.all(8.0),
              child: Text(
                row[0],
                style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 12),
              ),
            ),
            Padding(
              padding: const EdgeInsets.all(8.0),
              child: Text(row[1], style: const TextStyle(fontSize: 12)),
            ),
          ],
        );
      }).toList(),
    );
  }

  Widget _buildComparisonRow(String label, bool matches) {
    return Row(
      children: [
        Icon(
          matches ? Icons.check_circle : Icons.cancel,
          color: matches ? Colors.green : Colors.red,
          size: 18,
        ),
        const SizedBox(width: 8),
        Text(label),
      ],
    );
  }

  Color _getStatusColor(String? status) {
    if (status == null) return Colors.grey;
    if (status.contains('✅')) return Colors.green;
    if (status.contains('⚠️')) return Colors.orange;
    if (status.contains('❌')) return Colors.red;
    if (status.contains('ERROR')) return Colors.red;
    return Colors.grey;
  }
}

/// Test Case Model
class TestCase {
  final String id;
  final String name;
  final String description;
  final AnomalyInput input;
  final String displayTime;
  final AnomalyOutput expectedOutput;
  AnomalyOutput? aiResponse;
  String? testStatus;

  TestCase({
    required this.id,
    required this.name,
    required this.description,
    required this.input,
    required this.displayTime,
    required this.expectedOutput,
  });
}
