import 'package:flutter_background_service/flutter_background_service.dart';
import 'package:shared_preferences/shared_preferences.dart';

class BackgroundServiceControlService {
  static const String _kServiceEnabledKey = 'pke_bg_service_enabled';
  static const bool _defaultEnabled = true;

  /// Check if background service is currently enabled (persisted preference)
  Future<bool> isEnabled() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getBool(_kServiceEnabledKey) ?? _defaultEnabled;
  }

  /// Start the background service and persist the enabled state
  Future<void> startService() async {
    try {
      final service = FlutterBackgroundService();
      final isServiceRunning = await service.isRunning();
      if (!isServiceRunning) {
        await service.startService();
      }
      final prefs = await SharedPreferences.getInstance();
      await prefs.setBool(_kServiceEnabledKey, true);
      print('[PKE][BG-CTRL] Background service started and enabled');
    } catch (e) {
      print('[PKE][BG-CTRL] Error starting service: $e');
      rethrow;
    }
  }

  /// Stop the background service and persist the disabled state
  Future<void> stopService() async {
    try {
      final service = FlutterBackgroundService();
      final isRunning = await service.isRunning();
      print('[PKE][BG-CTRL] Stopping service (isRunning=$isRunning)');
      
      // First, persist the disabled state BEFORE stopping
      final prefs = await SharedPreferences.getInstance();
      await prefs.setBool(_kServiceEnabledKey, false);
      print('[PKE][BG-CTRL] Disabled flag set in SharedPreferences');
      
      // Then invoke the stop if the service is running
      if (isRunning) {
        service.invoke('stopService');
        print('[PKE][BG-CTRL] Invoked stopService event');
      }
      
      print('[PKE][BG-CTRL] Background service stopped and disabled');
    } catch (e) {
      print('[PKE][BG-CTRL] Error stopping service: $e');
      rethrow;
    }
  }

  /// Toggle background service on/off
  Future<bool> toggleService() async {
    final currentState = await isEnabled();
    if (currentState) {
      await stopService();
      return false;
    } else {
      await startService();
      return true;
    }
  }
}
