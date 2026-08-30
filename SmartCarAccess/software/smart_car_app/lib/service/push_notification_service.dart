import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import 'package:flutter/material.dart';

/// Service for handling system push notifications (works even when app is closed)
class PushNotificationService {
  static final PushNotificationService _instance =
      PushNotificationService._internal();

  static PushNotificationService get instance => _instance;

  late FlutterLocalNotificationsPlugin _notificationsPlugin;
  bool _isInitialized = false;
  
  PushNotificationService._internal() {
    _initializeNotifications();
  }

  void _initializeNotifications() {
    _notificationsPlugin = FlutterLocalNotificationsPlugin();

    // Android initialization
    const AndroidInitializationSettings androidSettings =
        AndroidInitializationSettings('@mipmap/ic_launcher');

    // iOS initialization
    final DarwinInitializationSettings iosSettings =
        DarwinInitializationSettings(
          requestAlertPermission: true,
          requestBadgePermission: true,
          requestSoundPermission: true,
        );

    final InitializationSettings settings = InitializationSettings(
      android: androidSettings,
      iOS: iosSettings,
    );

    _notificationsPlugin.initialize(
      settings,
      onDidReceiveNotificationResponse: _onNotificationTap,
    ).then((_) {
      _isInitialized = true;
      debugPrint('✅ Push Notifications initialized');
    }).catchError((e) {
      debugPrint('❌ Push Notifications init error: $e');
    });
  }

  void _onNotificationTap(NotificationResponse response) {
    debugPrint('📲 Notification tapped: ${response.payload}');
  }

  /// Show system notification (appears even when app is closed)
  Future<void> showNotification({
    required String title,
    required String body,
    String? payload,
  }) async {
    try {
      if (!_isInitialized) {
        await Future.delayed(const Duration(milliseconds: 500));
      }

      const AndroidNotificationDetails androidDetails =
          AndroidNotificationDetails(
        'anomaly_detection_channel',
        'Anomaly Detection',
        channelDescription: 'Notifications for anomalous access detection',
        importance: Importance.max,
        priority: Priority.high,
        enableVibration: true,
      );

      final DarwinNotificationDetails iosDetails =
          DarwinNotificationDetails(
            presentAlert: true,
            presentBadge: true,
            presentSound: true,
          );

      final NotificationDetails details = NotificationDetails(
        android: androidDetails,
        iOS: iosDetails,
      );

      await _notificationsPlugin.show(
        DateTime.now().millisecondsSinceEpoch ~/ 1000,
        title,
        body,
        details,
        payload: payload,
      );
      
      debugPrint('✅ Notification sent: $title');
    } catch (e) {
      debugPrint('❌ Error sending notification: $e');
    }
  }

  /// Cancel all notifications
  Future<void> cancelAllNotifications() async {
    try {
      await _notificationsPlugin.cancelAll();
      debugPrint('✅ All notifications cancelled');
    } catch (e) {
      debugPrint('❌ Error cancelling notifications: $e');
    }
  }
}
