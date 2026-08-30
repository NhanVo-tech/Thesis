import 'dart:async';

import 'package:flutter/material.dart';
import '../service/ble_phase_test.dart';
import '../service/gps_service.dart';

/// GPS Test Screen
/// 
/// Màn hình test tính năng GPS
class GpsTestScreen extends StatefulWidget {
  const GpsTestScreen({Key? key}) : super(key: key);

  @override
  State<GpsTestScreen> createState() => _GpsTestScreenState();
}

class _GpsTestScreenState extends State<GpsTestScreen> {
  final BlePhaseTestService _bleService = BlePhaseTestService();
  final GpsService _gpsService = GpsService();
  Timer? _locationRefreshTimer;
  
  String _status = 'Ready';
  String _locationInfo = 'No location yet';
  bool _isLoading = false;
  bool _isAuthenticated = false;
  
  // ESP32 device address - THAY ĐỔI NÀY THEO THIẾT BỊ CỦA BẠN
  final String _deviceAddress = "XX:XX:XX:XX:XX:XX";

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) {
        return;
      }
      _testGetLocation();
      _startAutoRefresh();
    });
  }

  @override
  void dispose() {
    _locationRefreshTimer?.cancel();
    super.dispose();
  }

  void _startAutoRefresh() {
    _locationRefreshTimer?.cancel();
    _locationRefreshTimer = Timer.periodic(const Duration(seconds: 30), (_) {
      if (!mounted || _isLoading) {
        return;
      }
      _testGetLocation(silent: true);
    });
  }
  
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('GPS Feature Test'),
        backgroundColor: Colors.blue,
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // Status card
            Card(
              color: _isAuthenticated ? Colors.green[50] : Colors.grey[200],
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Status',
                      style: Theme.of(context).textTheme.titleLarge,
                    ),
                    const SizedBox(height: 8),
                    Text(
                      _status,
                      style: TextStyle(
                        fontSize: 16,
                        color: _isAuthenticated ? Colors.green[900] : Colors.black87,
                      ),
                    ),
                  ],
                ),
              ),
            ),
            
            const SizedBox(height: 16),
            
            // Location info card
            Card(
              color: Colors.blue[50],
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Location Info',
                      style: Theme.of(context).textTheme.titleLarge,
                    ),
                    const SizedBox(height: 8),
                    Text(
                      _locationInfo,
                      style: const TextStyle(
                        fontSize: 14,
                        fontFamily: 'monospace',
                      ),
                    ),
                  ],
                ),
              ),
            ),
            
            const SizedBox(height: 24),
            
            // Test buttons
            ElevatedButton.icon(
              icon: const Icon(Icons.bluetooth),
              label: const Text('Step 1: Connect & Authenticate'),
              onPressed: _isLoading ? null : _testAuthentication,
              style: ElevatedButton.styleFrom(
                padding: const EdgeInsets.all(16),
                backgroundColor: Colors.blue,
              ),
            ),
            
            const SizedBox(height: 12),
            
            ElevatedButton.icon(
              icon: const Icon(Icons.location_searching),
              label: const Text('Step 2: Get GPS Location'),
              onPressed: _isLoading ? null : _testGetLocation,
              style: ElevatedButton.styleFrom(
                padding: const EdgeInsets.all(16),
                backgroundColor: Colors.orange,
              ),
            ),
            
            const SizedBox(height: 12),
            
            ElevatedButton.icon(
              icon: const Icon(Icons.send),
              label: const Text('Step 3: Send GPS to ESP32'),
              onPressed: (_isLoading || !_isAuthenticated) ? null : _testSendGps,
              style: ElevatedButton.styleFrom(
                padding: const EdgeInsets.all(16),
                backgroundColor: Colors.green,
              ),
            ),
            
            const SizedBox(height: 24),
            
            // Quick test button
            OutlinedButton.icon(
              icon: const Icon(Icons.play_arrow),
              label: const Text('Quick Test (All Steps)'),
              onPressed: _isLoading ? null : _quickTest,
              style: OutlinedButton.styleFrom(
                padding: const EdgeInsets.all(16),
              ),
            ),
            
            const Spacer(),
            
            // Loading indicator
            if (_isLoading)
              const Center(
                child: CircularProgressIndicator(),
              ),
          ],
        ),
      ),
    );
  }
  
  Future<void> _testAuthentication() async {
    setState(() {
      _isLoading = true;
      _status = 'Connecting to ESP32...';
      _isAuthenticated = false;
    });
    
    try {
      debugPrint('Testing Phase B authentication...');
      final result = await _bleService.testPhaseB(
        deviceAddress: _deviceAddress,
        timeout: const Duration(seconds: 30),
      );
      
      if (result.success) {
        setState(() {
          _status = '✓ Authenticated successfully!\nSession keys ready.';
          _isAuthenticated = true;
        });
        
        _showSnackBar('Authentication successful!', Colors.green);
      } else {
        setState(() {
          _status = '✗ Authentication failed: ${result.message}';
        });
        
        _showSnackBar('Authentication failed!', Colors.red);
      }
    } catch (e) {
      setState(() {
        _status = '✗ Error: $e';
      });
      
      _showSnackBar('Error: $e', Colors.red);
    } finally {
      setState(() {
        _isLoading = false;
      });
    }
  }
  
  Future<void> _testGetLocation({bool silent = false}) async {
    if (_isLoading) {
      return;
    }

    if (mounted) {
      setState(() {
        _isLoading = !silent;
        _status = silent ? 'Refreshing GPS location...' : 'Getting GPS location...';
      });
    }
    
    try {
      // Check permissions
      final hasPermission = await _gpsService.checkAndRequestPermissions();
      if (!hasPermission) {
        if (mounted) {
          setState(() {
            _status = '✗ GPS permission denied';
            _locationInfo = 'Please enable location permission in Settings';
          });
        }
        
        if (!silent) {
          _showSnackBar('GPS permission denied!', Colors.red);
        }
        return;
      }
      
      // Get location
      debugPrint('Getting current position...');
      final position = await _gpsService.getCurrentPosition(
        timeout: const Duration(seconds: 15),
      );
      
      if (position != null) {
        final address = await _gpsService.getAddressFromPosition(position);
        if (mounted) {
          setState(() {
            _status = silent ? '✓ Location auto-refreshed' : '✓ Location obtained';
            _locationInfo = 'Place:     ${address ?? 'Unknown area'}\n'
                           'Latitude:  ${position.latitude.toStringAsFixed(6)}\n'
                           'Longitude: ${position.longitude.toStringAsFixed(6)}\n'
                           'Altitude:  ${position.altitude.toStringAsFixed(1)} m\n'
                           'Accuracy:  ${position.accuracy.toStringAsFixed(1)} m\n'
                           'Timestamp: ${position.timestamp}';
          });
        }
        
        if (!silent) {
          _showSnackBar('Location obtained!', Colors.green);
        }
      } else {
        if (mounted) {
          setState(() {
            _status = '✗ Failed to get location';
            _locationInfo = 'Could not get GPS position. Check if GPS is enabled.';
          });
        }
        
        if (!silent) {
          _showSnackBar('Failed to get location!', Colors.red);
        }
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          _status = '✗ Error: $e';
        });
      }
      
      if (!silent) {
        _showSnackBar('Error: $e', Colors.red);
      }
    } finally {
      if (mounted) {
        setState(() {
          _isLoading = false;
        });
      }
    }
  }
  
  Future<void> _testSendGps() async {
    if (!_isAuthenticated) {
      _showSnackBar('Please authenticate first!', Colors.orange);
      return;
    }
    
    setState(() {
      _isLoading = true;
      _status = 'Sending GPS data to ESP32...';
    });
    
    try {
      debugPrint('Sending GPS location...');
      final success = await _bleService.sendGpsLocation();
      
      if (success) {
        setState(() {
          _status = '✓ GPS data sent successfully!\nCheck ESP32 serial monitor for decrypted data.';
        });
        
        _showSnackBar('GPS data sent to ESP32!', Colors.green);
      } else {
        setState(() {
          _status = '✗ Failed to send GPS data';
        });
        
        _showSnackBar('Failed to send GPS data!', Colors.red);
      }
    } catch (e) {
      setState(() {
        _status = '✗ Error: $e';
      });
      
      _showSnackBar('Error: $e', Colors.red);
    } finally {
      setState(() {
        _isLoading = false;
      });
    }
  }
  
  Future<void> _quickTest() async {
    // Run all tests sequentially
    await _testAuthentication();
    
    if (_isAuthenticated) {
      await Future.delayed(const Duration(seconds: 1));
      await _testGetLocation();
      
      await Future.delayed(const Duration(seconds: 1));
      await _testSendGps();
    }
  }
  
  void _showSnackBar(String message, Color backgroundColor) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: backgroundColor,
        duration: const Duration(seconds: 3),
      ),
    );
  }
}
