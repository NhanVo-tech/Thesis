import 'package:flutter/material.dart';
import 'screen/gps_test_screen.dart';

/// GPS Test App Entry Point
/// 
/// Để chạy test GPS:
/// flutter run -t lib/main_gps_test.dart
void main() {
  runApp(const GpsTestApp());
}

class GpsTestApp extends StatelessWidget {
  const GpsTestApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Smart Car GPS Test',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue),
        useMaterial3: true,
      ),
      home: const GpsTestScreen(),
    );
  }
}
