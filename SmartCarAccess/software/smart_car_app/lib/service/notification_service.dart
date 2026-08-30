import 'package:flutter/material.dart';
import 'language_service.dart';

/// Notification service for sending anomaly detection alerts
class NotificationService {
  static final NotificationService _instance = NotificationService._internal();
  static NotificationService get instance => _instance;

  NotificationService._internal();

  /// Get notification title based on severity and language
  String getNotificationTitle(String severity) {
    final language = LanguageService.instance.currentLanguage;
    
    if (language == 'vi') {
      switch (severity) {
        case 'medium':
          return 'Xác nhận truy cập';
        case 'high':
          return 'Truy cập bị chặn';
        default:
          return '';
      }
    } else {
      switch (severity) {
        case 'medium':
          return 'Confirm Access';
        case 'high':
          return 'Access Blocked';
        default:
          return '';
      }
    }
  }

  /// Get notification body based on severity and language
  String getNotificationBody(String severity) {
    final language = LanguageService.instance.currentLanguage;
    
    if (language == 'vi') {
      switch (severity) {
        case 'medium':
          return 'Có dấu hiệu truy cập bất thường, vui lòng xác nhận';
        case 'high':
          return 'Phát hiện hoạt động đáng ngờ, yêu cầu xác minh';
        default:
          return '';
      }
    } else {
      switch (severity) {
        case 'medium':
          return 'Unusual access detected, please confirm';
        case 'high':
          return 'Suspicious activity detected, verification required';
        default:
          return '';
      }
    }
  }

  /// Show notification in app (using SnackBar/Dialog)
  void showNotification(
    BuildContext context,
    String title,
    String body,
    String severity,
  ) {
    if (title.isEmpty || body.isEmpty) return;

    final color = _getSeverityColor(severity);
    final icon = _getSeverityIcon(severity);

    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(icon, color: Colors.white),
                const SizedBox(width: 12),
                Expanded(
                  child: Text(
                    title,
                    style: const TextStyle(
                      fontWeight: FontWeight.bold,
                      fontSize: 16,
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 8),
            Padding(
              padding: const EdgeInsets.only(left: 36),
              child: Text(body),
            ),
          ],
        ),
        backgroundColor: color,
        duration: Duration(seconds: severity == 'high' ? 10 : 5),
        behavior: SnackBarBehavior.floating,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8)),
        margin: const EdgeInsets.all(16),
      ),
    );
  }

  /// Show dialog for high severity anomalies
  Future<bool?> showBlockingDialog(
    BuildContext context,
    String title,
    String body,
  ) {
    return showDialog<bool>(
      context: context,
      barrierDismissible: false,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Row(
            children: [
              const Icon(Icons.warning, color: Colors.red),
              const SizedBox(width: 12),
              Expanded(child: Text(title)),
            ],
          ),
          content: Text(body),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(false),
              child: Text(
                LanguageService.instance.translate('cancel'),
                style: const TextStyle(color: Colors.grey),
              ),
            ),
            ElevatedButton(
              onPressed: () => Navigator.of(context).pop(true),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.green,
              ),
              child: const Text(
                'Proceed',
                style: TextStyle(color: Colors.white),
              ),
            ),
          ],
        );
      },
    );
  }

  /// Get color based on severity
  Color _getSeverityColor(String severity) {
    switch (severity) {
      case 'low':
        return Colors.blue;
      case 'medium':
        return Colors.orange;
      case 'high':
        return Colors.red;
      default:
        return Colors.grey;
    }
  }

  /// Get icon based on severity
  IconData _getSeverityIcon(String severity) {
    switch (severity) {
      case 'low':
        return Icons.info;
      case 'medium':
        return Icons.warning;
      case 'high':
        return Icons.error;
      default:
        return Icons.notifications;
    }
  }
}
