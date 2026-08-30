import 'dart:convert';

class AIResult {
  final bool isAnomalous;
  final double confidenceScore;
  final String reason;

  AIResult({
    required this.isAnomalous,
    required this.confidenceScore,
    required this.reason,
  });

  factory AIResult.fromJson(Map<String, dynamic> json) {
    return AIResult(
      isAnomalous: json['is_anomalous'] as bool,
      confidenceScore: json['confidence_score'] as double,
      reason: json['reason'] as String,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'is_anomalous': isAnomalous,
      'confidence_score': confidenceScore,
      'reason': reason,
    };
  }
}

class Context {
  final String userId;
  final String carId;
  final String timestamp;

  Context({
    required this.userId,
    required this.carId,
    required this.timestamp,
  });

  factory Context.fromJson(Map<String, dynamic> json) {
    return Context(
      userId: json['user_id'] as String,
      carId: json['car_id'] as String,
      timestamp: json['timestamp'] as String,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'user_id': userId,
      'car_id': carId,
      'timestamp': timestamp,
    };
  }
}

class AnomalyDecisionInput {
  final AIResult aiResult;
  final Context context;

  AnomalyDecisionInput({
    required this.aiResult,
    required this.context,
  });

  factory AnomalyDecisionInput.fromJson(Map<String, dynamic> json) {
    return AnomalyDecisionInput(
      aiResult: AIResult.fromJson(json['ai_result'] as Map<String, dynamic>),
      context: Context.fromJson(json['context'] as Map<String, dynamic>),
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'ai_result': aiResult.toJson(),
      'context': context.toJson(),
    };
  }
}

class Notification {
  final String title;
  final String body;

  Notification({
    required this.title,
    required this.body,
  });

  Map<String, dynamic> toJson() {
    return {
      'title': title,
      'body': body,
    };
  }
}

class Log {
  final bool aiPrediction;
  final double confidenceScore;

  Log({
    required this.aiPrediction,
    required this.confidenceScore,
  });

  Map<String, dynamic> toJson() {
    return {
      'ai_prediction': aiPrediction,
      'confidence_score': confidenceScore,
    };
  }
}

class AnomalyDecisionOutput {
  final String severity;
  final String action;
  final String userMessage;
  final bool shouldNotify;
  final Notification notification;
  final Log log;

  AnomalyDecisionOutput({
    required this.severity,
    required this.action,
    required this.userMessage,
    required this.shouldNotify,
    required this.notification,
    required this.log,
  });

  Map<String, dynamic> toJson() {
    return {
      'severity': severity,
      'action': action,
      'user_message': userMessage,
      'should_notify': shouldNotify,
      'notification': notification.toJson(),
      'log': log.toJson(),
    };
  }

  String toJsonString() {
    return jsonEncode(toJson());
  }
}

enum Severity {
  low,
  medium,
  high,
}

enum Action {
  allow,
  confirm,
  block,
}

class AnomalyDecisionEngine {
  static AnomalyDecisionOutput processDecision(AnomalyDecisionInput input) {
    // Step 1: Determine severity
    Severity severity = _determineSeverity(input.aiResult.confidenceScore);

    // Step 2: Decision logic
    Action action = _determineAction(severity);

    // Step 3: User message
    String userMessage = _generateUserMessage(input.aiResult.reason, severity);

    // Step 4: Notification
    bool shouldNotify = _shouldNotify(severity);
    Notification notification = _generateNotification(severity, input.aiResult.reason);

    // Step 5: Log
    Log log = Log(
      aiPrediction: input.aiResult.isAnomalous,
      confidenceScore: input.aiResult.confidenceScore,
    );

    return AnomalyDecisionOutput(
      severity: severity.name.toUpperCase(),
      action: action.name.toUpperCase(),
      userMessage: userMessage,
      shouldNotify: shouldNotify,
      notification: notification,
      log: log,
    );
  }

  static Severity _determineSeverity(double confidenceScore) {
    if (confidenceScore <= 0.6) {
      return Severity.low;
    } else if (confidenceScore <= 0.85) {
      return Severity.medium;
    } else {
      return Severity.high;
    }
  }

  static Action _determineAction(Severity severity) {
    switch (severity) {
      case Severity.low:
        return Action.allow;
      case Severity.medium:
        return Action.confirm;
      case Severity.high:
        return Action.block;
    }
  }

  static String _generateUserMessage(String reason, Severity severity) {
    // Translate AI reason into natural Vietnamese language
    // Keep short, clear, non-technical, ≤12 words
    switch (reason.toLowerCase()) {
      case 'unusual time':
        return 'Truy cập ngoài giờ bình thường';
      case 'distant location':
        return 'Vị trí xa lạ';
      case 'unknown device':
        return 'Thiết bị chưa từng sử dụng';
      case 'high frequency':
        return 'Tần suất truy cập cao';
      case 'multiple risk factors':
        return 'Nhiều dấu hiệu bất thường';
      case 'normal access pattern':
        return 'Truy cập bình thường';
      default:
        return 'Hoạt động có dấu hiệu bất thường';
    }
  }

  static bool _shouldNotify(Severity severity) {
    switch (severity) {
      case Severity.low:
        return false;
      case Severity.medium:
        return true; // optional soft warning
      case Severity.high:
        return true; // urgent alert
    }
  }

  static Notification _generateNotification(Severity severity, String reason) {
    switch (severity) {
      case Severity.low:
        return Notification(title: '', body: '');
      case Severity.medium:
        return Notification(
          title: 'Cảnh báo truy cập',
          body: 'Có dấu hiệu bất thường, vui lòng xác nhận',
        );
      case Severity.high:
        return Notification(
          title: 'Truy cập bị chặn',
          body: 'Phát hiện hoạt động đáng ngờ, yêu cầu xác minh',
        );
    }
  }
}