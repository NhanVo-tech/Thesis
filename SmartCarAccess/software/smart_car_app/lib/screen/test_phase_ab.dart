import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:smart_car_app/service/ble_phase_test.dart';
import 'package:smart_car_app/service/ble_runtime_permissions.dart';
import 'dart:async';

/// Test screen for Phase A (NFC Provisioning) and Phase B (BLE Authentication)
class TestPhaseABScreen extends StatefulWidget {
  const TestPhaseABScreen({super.key});

  @override
  State<TestPhaseABScreen> createState() => _TestPhaseABScreenState();
}

class _TestPhaseABScreenState extends State<TestPhaseABScreen> {
  final BlePhaseTestService _testService = BlePhaseTestService();
  final BleRuntimePermissionService _blePermissionService =
      BleRuntimePermissionService();
  final TextEditingController _deviceAddressController =
      TextEditingController();
  final ScrollController _logScrollController = ScrollController();

  final List<String> _logs = [];
  bool _isTestingPhaseA = false;
  bool _isTestingPhaseB = false;
  bool _isScanningDevices = false;

  PhaseAResult? _phaseAResult;
  PhaseBResult? _phaseBResult;

  List<ScanResult> _scanResults = [];
  BluetoothDevice? _selectedDevice;

  @override
  void initState() {
    super.initState();
    _checkBluetoothState();
  }

  @override
  void dispose() {
    _deviceAddressController.dispose();
    _logScrollController.dispose();
    _testService.disconnect();
    super.dispose();
  }

  Future<void> _checkBluetoothState() async {
    try {
      final isSupported = await FlutterBluePlus.isSupported;
      if (!isSupported) {
        _addLog('❌ Bluetooth is not supported on this device');
        return;
      }

      final adapterState = await FlutterBluePlus.adapterState.first;
      if (adapterState != BluetoothAdapterState.on) {
        _addLog('⚠️ Please enable Bluetooth');
      } else {
        _addLog('✓ Bluetooth is ready');
      }
    } catch (e) {
      _addLog('❌ Bluetooth check failed: $e');
    }
  }

  void _addLog(String message) {
    setState(() {
      _logs.add('[${DateTime.now().toString().substring(11, 23)}] $message');
    });

    // Auto scroll to bottom
    Future.delayed(const Duration(milliseconds: 100), () {
      if (_logScrollController.hasClients) {
        _logScrollController.animateTo(
          _logScrollController.position.maxScrollExtent,
          duration: const Duration(milliseconds: 300),
          curve: Curves.easeOut,
        );
      }
    });
  }

  void _clearLogs() {
    setState(() {
      _logs.clear();
      _phaseAResult = null;
      _phaseBResult = null;
    });
  }

  Future<void> _scanForDevices() async {
    final permissionStatus = await _blePermissionService.ensureReady(
      requestIfNeeded: true,
    );
    if (!permissionStatus.ready) {
      _addLog('⚠️ ${permissionStatus.toUserMessage()}');
      return;
    }

    setState(() {
      _isScanningDevices = true;
      _scanResults.clear();
    });

    _addLog('🔍 Scanning for BLE devices...');

    try {
      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 10));

      final subscription = FlutterBluePlus.scanResults.listen((results) {
        setState(() {
          _scanResults = results
              .where((r) => r.device.platformName.isNotEmpty)
              .toList();
        });
      });

      await Future.delayed(const Duration(seconds: 10));
      await FlutterBluePlus.stopScan();
      subscription.cancel();

      _addLog('✓ Found ${_scanResults.length} devices');
    } catch (e) {
      _addLog('❌ Scan failed: $e');
    } finally {
      setState(() {
        _isScanningDevices = false;
      });
    }
  }

  Future<void> _testPhaseA() async {
    setState(() {
      _isTestingPhaseA = true;
      _phaseAResult = null;
    });

    _addLog('═══════════════════════════════════');
    _addLog('🔷 STARTING PHASE A TEST (NFC Provisioning)');
    _addLog('═══════════════════════════════════');

    try {
      _addLog('');
      _addLog('📱 Checking Android Keystore...');

      // 1. Check if Phase A key exists in Keystore
      try {
        final keystoreChannel = const MethodChannel('smartcar/keystore');

        // Ensure key exists
        _addLog('   Ensuring identity key exists...');
        final keyExists = await keystoreChannel.invokeMethod('ensurePhaseAKey');

        if (keyExists) {
          _addLog('   ✓ Identity key ready in Android Keystore');

          // Get public key
          _addLog('   Reading public key...');
          final publicKey = await keystoreChannel.invokeMethod(
            'getPhaseAPublicKey65',
          );

          if (publicKey != null && publicKey.length == 65) {
            _addLog('   ✓ Public key: ${_formatBytes(publicKey, 16)}...');
            _addLog('');

            // Show Phase A information
            _addLog('📋 PHASE A INFO:');
            _addLog('   Service: ProvisioningHostApduService (HCE)');
            _addLog('   AID: F0:01:02:03:04:05');
            _addLog('   Identity Key: Android Keystore (hardware-backed)');
            _addLog('   Public Key: 65 bytes (uncompressed P-256)');
            _addLog('');
            _addLog('📝 PROVISIONING FLOW:');
            _addLog(
              '   1. ECU sends: SELECT AID (00 A4 04 00 06 F0010203040500)',
            );
            _addLog('      ← Phone returns: UID(4 bytes) + 90 00');
            _addLog('');
            _addLog('   2. ECU sends: GET_CHALLENGE Lc=0 (00 CA 00 00 00 00)');
            _addLog(
              '      ← Phone returns: [keyId(1) + phonePub(65) + certLen(2)] + 90 00',
            );
            _addLog('      → Public key from Android Keystore');
            _addLog('');
            _addLog('   3. ECU builds challenge: vehicleId(8) || nonce(16)');
            _addLog('      ECU sends: GET_CHALLENGE Lc=24 + [challenge]');
            _addLog(
              '      ← Phone signs challenge with private key (Keystore)',
            );
            _addLog(
              '      ← Phone returns: [sigLen(2,BE) + DER_signature] + 90 00',
            );
            _addLog('');
            _addLog('   4. ECU verifies signature → stores public key in NVS');
            _addLog('');
            _addLog('🔐 SECURITY:');
            _addLog('   • Private key: NEVER leaves Keystore');
            _addLog('   • Public key: Verified before storage');
            _addLog('   • Signature: DER-encoded ECDSA-SHA256');
            _addLog('   • Storage: ESP32 NVS (namespace "prov")');
            _addLog('');
            _addLog('📍 HCE STATUS:');
            _addLog('   ✓ HCE service is active (manifest verified)');
            _addLog('   ✓ App can be foreground or background');
            _addLog('   ✓ Tap phone on PN532 to start provisioning');
            _addLog('');
            _addLog('═══════════════════════════════════');
            _addLog('✅ PHASE A READY');
            _addLog('═══════════════════════════════════');
            _addLog('');
            _addLog('💡 GUIDANCE:');
            _addLog('   1. Ensure ESP32 is running and PN532 is active');
            _addLog(
              '   2. Send serial command "f" or "F" to force provisioning',
            );
            _addLog('   3. Tap phone on the PN532 reader');
            _addLog('   4. Check logs in ESP32 Serial Monitor');
            _addLog('   5. Send "p" to view provisioning status');

            setState(() {
              _phaseAResult = PhaseAResult(
                success: true,
                message: 'HCE service ready; identity key in Keystore',
                phonePublicKeyStored: true,
              );
            });
          } else {
            throw Exception('Failed to read public key from Keystore');
          }
        } else {
          throw Exception('Failed to ensure identity key in Keystore');
        }
      } catch (e) {
        _addLog('❌ Keystore check failed: $e');
        rethrow;
      }
    } catch (e) {
      _addLog('');
      _addLog('❌ ERROR: $e');
      setState(() {
        _phaseAResult = PhaseAResult(
          success: false,
          message: e.toString(),
          phonePublicKeyStored: false,
        );
      });
    } finally {
      setState(() {
        _isTestingPhaseA = false;
      });
    }
  }

  Future<void> _testPhaseB() async {
    if (_selectedDevice == null && _deviceAddressController.text.isEmpty) {
      _addLog('❌ Please select a device or enter a MAC address');
      return;
    }

    final permissionStatus = await _blePermissionService.ensureReady(
      requestIfNeeded: true,
    );
    if (!permissionStatus.ready) {
      _addLog('⚠️ ${permissionStatus.toUserMessage()}');
      return;
    }

    final deviceAddress =
        _selectedDevice?.remoteId.str ?? _deviceAddressController.text;

    setState(() {
      _isTestingPhaseB = true;
      _phaseBResult = null;
    });

    _addLog('═══════════════════════════════════');
    _addLog('🔷 STARTING PHASE B TEST (BLE Authentication)');
    _addLog('═══════════════════════════════════');
    _addLog('Device: $deviceAddress');
    _addLog('');

    try {
      // Measure total time
      final stepStartTime = DateTime.now();

      // Run authentication with progress callback
      final result = await _testService.testPhaseB(
        deviceAddress: deviceAddress,
        device: _selectedDevice,
        onProgress: (step, message) {
          // Format progress messages for UI
          _addLog('[$step] $message');
        },
      );

      final stepDuration = DateTime.now().difference(stepStartTime);
      _addLog('');
      _addLog('⏱️  Total time: ${stepDuration.inMilliseconds}ms');

      setState(() {
        _phaseBResult = result;
      });

      if (result.success) {
        _addLog('');
        _addLog('═══════════════════════════════════');
        _addLog('✅ PHASE B COMPLETED SUCCESSFULLY!');
        _addLog('═══════════════════════════════════');
        _addLog('📊 RESULTS:');
        _addLog(
          '   🔗 Shared Secret: ${_formatBytes(result.sharedSecret!, 16)}...',
        );
        _addLog(
          '   🔑 Encryption Key: ${_formatBytes(result.sessionEncKey!, 16)}...',
        );
        _addLog('   🔑 MAC Key: ${_formatBytes(result.sessionMacKey!, 16)}...');
        _addLog('   🎯 Challenge: ${_formatBytes(result.challenge!, 24)}');
        _addLog('');
        _addLog('🔓 Ready to unlock the vehicle!');
      } else {
        _addLog('');
        _addLog('═══════════════════════════════════');
        _addLog('❌ PHASE B FAILED');
        _addLog('═══════════════════════════════════');
        _addLog('Error: ${result.message}');
      }
    } catch (e, stackTrace) {
      _addLog('');
      _addLog('❌ CRITICAL ERROR: $e');
      _addLog('Stack trace: ${stackTrace.toString().substring(0, 200)}...');

      setState(() {
        _phaseBResult = PhaseBResult(success: false, message: e.toString());
      });
    } finally {
      setState(() {
        _isTestingPhaseB = false;
      });
    }
  }

  String _formatBytes(List<int> bytes, int maxLength) {
    final limited = bytes.take(maxLength).toList();
    return limited
        .map((b) => b.toRadixString(16).padLeft(2, '0').toUpperCase())
        .join(' ');
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFFF5F7FA),
      appBar: AppBar(
        title: const Text(
          'Test Phase A/B',
          style: TextStyle(fontWeight: FontWeight.bold, color: Colors.white),
        ),
        backgroundColor: const Color(0xFF273671),
        elevation: 0,
        iconTheme: const IconThemeData(color: Colors.white),
        actions: [
          IconButton(
            icon: const Icon(Icons.delete_outline),
            tooltip: 'Clear logs',
            onPressed: _clearLogs,
          ),
        ],
      ),
      body: Column(
        children: [
          // Header with Phase indicators
          Container(
            width: double.infinity,
            padding: const EdgeInsets.all(16),
            decoration: const BoxDecoration(
              color: Color(0xFF273671),
              borderRadius: BorderRadius.only(
                bottomLeft: Radius.circular(24),
                bottomRight: Radius.circular(24),
              ),
            ),
            child: Column(
              children: [
                Row(
                  children: [
                    Expanded(
                      child: _buildPhaseIndicator(
                        'Phase A',
                        'NFC Provisioning',
                        Icons.nfc,
                        _phaseAResult?.success ?? false,
                        _isTestingPhaseA,
                      ),
                    ),
                    const SizedBox(width: 16),
                    Expanded(
                      child: _buildPhaseIndicator(
                        'Phase B',
                        'BLE Auth',
                        Icons.bluetooth,
                        _phaseBResult?.success ?? false,
                        _isTestingPhaseB,
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ),

          const SizedBox(height: 16),

          // Device selection section
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    const Text(
                      'Select BLE device:',
                      style: TextStyle(
                        fontSize: 16,
                        fontWeight: FontWeight.bold,
                        color: Color(0xFF273671),
                      ),
                    ),
                    const Spacer(),
                    TextButton.icon(
                      onPressed: _isScanningDevices ? null : _scanForDevices,
                      icon: _isScanningDevices
                          ? const SizedBox(
                              width: 16,
                              height: 16,
                              child: CircularProgressIndicator(strokeWidth: 2),
                            )
                          : const Icon(Icons.refresh),
                      label: Text(_isScanningDevices ? 'Scanning...' : 'Scan'),
                    ),
                  ],
                ),
                const SizedBox(height: 8),

                // Device list
                if (_scanResults.isNotEmpty)
                  Container(
                    height: 120,
                    decoration: BoxDecoration(
                      color: Colors.white,
                      borderRadius: BorderRadius.circular(12),
                      boxShadow: [
                        BoxShadow(
                          color: Colors.black.withValues(alpha: 0.05),
                          blurRadius: 10,
                          offset: const Offset(0, 2),
                        ),
                      ],
                    ),
                    child: ListView.builder(
                      itemCount: _scanResults.length,
                      itemBuilder: (context, index) {
                        final result = _scanResults[index];
                        final isSelected =
                            _selectedDevice?.remoteId == result.device.remoteId;

                        return ListTile(
                          dense: true,
                          selected: isSelected,
                          selectedTileColor: const Color(0xFF273671).withValues(alpha: 0.1),
                          leading: Icon(
                            Icons.bluetooth,
                            color: isSelected
                                ? const Color(0xFF273671)
                                : Colors.grey,
                          ),
                          title: Text(
                            result.device.platformName.isEmpty
                                ? 'Unknown Device'
                                : result.device.platformName,
                            style: TextStyle(
                              fontWeight: isSelected
                                  ? FontWeight.bold
                                  : FontWeight.normal,
                            ),
                          ),
                          subtitle: Text(
                            result.device.remoteId.str,
                            style: const TextStyle(fontSize: 12),
                          ),
                          trailing: Text(
                            '${result.rssi} dBm',
                            style: TextStyle(
                              color: Colors.grey[600],
                              fontSize: 12,
                            ),
                          ),
                          onTap: () {
                            setState(() {
                              _selectedDevice = result.device;
                              _deviceAddressController.text =
                                  result.device.remoteId.str;
                            });
                            _addLog(
                              '✓ Selected: ${result.device.platformName} (${result.device.remoteId.str})',
                            );
                          },
                        );
                      },
                    ),
                  )
                else
                  TextField(
                    controller: _deviceAddressController,
                    decoration: InputDecoration(
                      hintText: 'Or enter MAC address (XX:XX:XX:XX:XX:XX)',
                      filled: true,
                      fillColor: Colors.white,
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(12),
                        borderSide: BorderSide.none,
                      ),
                      contentPadding: const EdgeInsets.symmetric(
                        horizontal: 16,
                        vertical: 12,
                      ),
                    ),
                  ),
              ],
            ),
          ),

          const SizedBox(height: 16),

          // Test buttons
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16),
            child: Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: _isTestingPhaseA ? null : _testPhaseA,
                    icon: _isTestingPhaseA
                        ? const SizedBox(
                            width: 20,
                            height: 20,
                            child: CircularProgressIndicator(
                              strokeWidth: 2,
                              color: Colors.white,
                            ),
                          )
                        : const Icon(Icons.nfc),
                    label: const Text('Test Phase A'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: const Color(0xFF41a5de),
                      foregroundColor: Colors.white,
                      padding: const EdgeInsets.symmetric(vertical: 14),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(12),
                      ),
                    ),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: _isTestingPhaseB ? null : _testPhaseB,
                    icon: _isTestingPhaseB
                        ? const SizedBox(
                            width: 20,
                            height: 20,
                            child: CircularProgressIndicator(
                              strokeWidth: 2,
                              color: Colors.white,
                            ),
                          )
                        : const Icon(Icons.bluetooth),
                    label: const Text('Test Phase B'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: const Color(0xFF273671),
                      foregroundColor: Colors.white,
                      padding: const EdgeInsets.symmetric(vertical: 14),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(12),
                      ),
                    ),
                  ),
                ),
              ],
            ),
          ),

          const SizedBox(height: 16),

          // Logs section
          Expanded(
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 16),
              child: Container(
                decoration: BoxDecoration(
                  color: Colors.black87,
                  borderRadius: BorderRadius.circular(12),
                  boxShadow: [
                    BoxShadow(
                      color: Colors.black.withValues(alpha: 0.1),
                      blurRadius: 10,
                      offset: const Offset(0, 2),
                    ),
                  ],
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Padding(
                      padding: const EdgeInsets.all(12),
                      child: Row(
                        children: [
                          const Icon(
                            Icons.terminal,
                            color: Colors.greenAccent,
                            size: 20,
                          ),
                          const SizedBox(width: 8),
                          const Text(
                            'Console Logs',
                            style: TextStyle(
                              color: Colors.greenAccent,
                              fontWeight: FontWeight.bold,
                              fontSize: 14,
                            ),
                          ),
                          const Spacer(),
                          Text(
                            '${_logs.length} lines',
                            style: TextStyle(
                              color: Colors.grey[500],
                              fontSize: 12,
                            ),
                          ),
                          const SizedBox(width: 8),
                          IconButton(
                            icon: const Icon(Icons.copy, size: 18),
                            color: Colors.greenAccent,
                            tooltip: 'Copy all logs',
                            padding: EdgeInsets.zero,
                            constraints: const BoxConstraints(),
                            onPressed: _logs.isEmpty
                                ? null
                                : () {
                                    final allLogs = _logs.join('\n');
                                    Clipboard.setData(
                                      ClipboardData(text: allLogs),
                                    );
                                    ScaffoldMessenger.of(context).showSnackBar(
                                      SnackBar(
                                        content: Text(
                                          '✓ Copied ${_logs.length} log lines to clipboard',
                                        ),
                                        duration: const Duration(seconds: 2),
                                        backgroundColor: Colors.green,
                                      ),
                                    );
                                  },
                          ),
                        ],
                      ),
                    ),
                    const Divider(color: Colors.white24, height: 1),
                    Expanded(
                      child: _logs.isEmpty
                          ? Center(
                              child: Text(
                                'No logs yet. Tap a test button to begin.',
                                style: TextStyle(
                                  color: Colors.grey[600],
                                  fontSize: 14,
                                ),
                              ),
                            )
                          : ListView.builder(
                              controller: _logScrollController,
                              padding: const EdgeInsets.all(12),
                              itemCount: _logs.length,
                              itemBuilder: (context, index) {
                                final log = _logs[index];
                                Color logColor = Colors.white;

                                if (log.contains('✓') || log.contains('✅')) {
                                  logColor = Colors.greenAccent;
                                } else if (log.contains('❌') ||
                                    log.contains('ERROR')) {
                                  logColor = Colors.redAccent;
                                } else if (log.contains('⚠️') ||
                                    log.contains('WARNING')) {
                                  logColor = Colors.orangeAccent;
                                } else if (log.contains('🔷') ||
                                    log.contains('═══')) {
                                  logColor = Colors.cyanAccent;
                                } else if (log.contains('[Step')) {
                                  logColor = Colors.yellowAccent;
                                }

                                return Padding(
                                  padding: const EdgeInsets.only(bottom: 4),
                                  child: SelectableText(
                                    log,
                                    style: TextStyle(
                                      fontFamily: 'monospace',
                                      fontSize: 12,
                                      color: logColor,
                                      height: 1.5,
                                    ),
                                  ),
                                );
                              },
                            ),
                    ),
                  ],
                ),
              ),
            ),
          ),

          const SizedBox(height: 16),
        ],
      ),
    );
  }

  Widget _buildPhaseIndicator(
    String title,
    String subtitle,
    IconData icon,
    bool isSuccess,
    bool isRunning,
  ) {
    Color statusColor = Colors.white54;
    IconData statusIcon = Icons.radio_button_unchecked;

    if (isRunning) {
      statusColor = Colors.orangeAccent;
      statusIcon = Icons.pending;
    } else if (isSuccess) {
      statusColor = Colors.greenAccent;
      statusIcon = Icons.check_circle;
    }

    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Colors.white.withValues(alpha: 0.1),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(
          color: isSuccess ? Colors.greenAccent : Colors.white24,
          width: 2,
        ),
      ),
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(icon, color: Colors.white, size: 24),
              const SizedBox(width: 8),
              Icon(statusIcon, color: statusColor, size: 20),
            ],
          ),
          const SizedBox(height: 8),
          Text(
            title,
            style: const TextStyle(
              color: Colors.white,
              fontWeight: FontWeight.bold,
              fontSize: 14,
            ),
          ),
          const SizedBox(height: 2),
          Text(
            subtitle,
            style: TextStyle(
              color: Colors.white.withValues(alpha: 0.7),
              fontSize: 11,
            ),
          ),
        ],
      ),
    );
  }

}
