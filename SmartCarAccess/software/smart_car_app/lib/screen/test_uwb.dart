import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:smart_car_app/widgets/uwb_ranging_section.dart';

/// Test screen for multi-anchor UWB ranging (multicast DS-TWR).
///
/// Connect BLE first (this keeps the ESP32 FSM secure channel alive so the
/// PC bridge stays ranged), then use the "UWB Multi-Anchor Ranging" card to
/// start/stop ranging against the 3 anchors.
class TestUwbScreen extends StatefulWidget {
  const TestUwbScreen({super.key});

  @override
  State<TestUwbScreen> createState() => _TestUwbScreenState();
}

class _TestUwbScreenState extends State<TestUwbScreen> {
  final ScrollController _logScroll = ScrollController();
  final List<String> _logs = <String>[];
  final List<ScanResult> _devices = <ScanResult>[];

  StreamSubscription<List<ScanResult>>? _scanSub;
  StreamSubscription<BluetoothConnectionState>? _connSub;

  BluetoothDevice? _selectedDevice;
  bool _isScanning = false;
  bool _isConnecting = false;

  bool get _isConnected =>
      _selectedDevice != null && _selectedDevice!.isConnected;

  @override
  void dispose() {
    _scanSub?.cancel();
    _connSub?.cancel();
    _logScroll.dispose();
    super.dispose();
  }

  void _appendLog(String line) {
    if (!mounted) return;
    setState(
      () => _logs.add(
        '[${DateTime.now().toIso8601String().substring(11, 19)}] $line',
      ),
    );
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (_logScroll.hasClients) {
        _logScroll.animateTo(
          _logScroll.position.maxScrollExtent,
          duration: const Duration(milliseconds: 250),
          curve: Curves.easeOut,
        );
      }
    });
  }

  Future<void> _scan() async {
    setState(() {
      _isScanning = true;
      _devices.clear();
    });

    try {
      _appendLog('Scanning for BLE devices...');
      await _scanSub?.cancel();
      final Map<String, ScanResult> seen = <String, ScanResult>{};
      _scanSub = FlutterBluePlus.scanResults.listen((results) {
        for (final result in results) {
          final key = result.device.remoteId.str.toUpperCase();
          final existing = seen[key];
          if (existing == null || result.rssi >= existing.rssi) {
            seen[key] = result;
          }
        }

        if (!mounted) return;
        final devices = seen.values.toList()..sort(_compareScanResults);
        setState(() {
          _devices
            ..clear()
            ..addAll(devices);
        });
      });

      await FlutterBluePlus.stopScan();
      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 5));
      await Future<void>.delayed(const Duration(seconds: 10));
      await FlutterBluePlus.stopScan();
      _appendLog('Found ${_devices.length} BLE device(s)');
    } catch (e) {
      _appendLog('Scan error: $e');
    } finally {
      await _scanSub?.cancel();
      _scanSub = null;
      if (mounted) {
        setState(() => _isScanning = false);
      }
    }
  }

  Future<void> _connect(BluetoothDevice device) async {
    if (_isConnected && _selectedDevice?.remoteId == device.remoteId) {
      _appendLog('Already connected');
      return;
    }

    await _disconnect();
    setState(() => _isConnecting = true);
    try {
      _appendLog(
        'Connecting to ${_deviceLabelFromDevice(device)} '
        '(${device.remoteId.str})...',
      );
      await device.connect(timeout: const Duration(seconds: 12));
      if (!mounted) return;
      setState(() => _selectedDevice = device);
      _appendLog('Connected');

      _connSub?.cancel();
      _connSub = device.connectionState.listen((state) {
        _appendLog('Connection state: $state');
        if (!mounted) return;
        setState(() {});
      });
    } catch (e) {
      _appendLog('Connect error: $e');
    } finally {
      if (mounted) {
        setState(() => _isConnecting = false);
      }
    }
  }

  Future<void> _disconnect() async {
    final device = _selectedDevice;
    if (device != null && device.isConnected) {
      try {
        await device.disconnect();
      } catch (_) {}
    }
    if (mounted) {
      setState(() => _selectedDevice = null);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Test UWB End-to-End'),
        actions: [
          IconButton(
            onPressed: () {
              setState(() => _logs.clear());
            },
            icon: const Icon(Icons.delete_outline),
            tooltip: 'Clear logs',
          ),
        ],
      ),
      body: SafeArea(
        child: LayoutBuilder(
          builder: (context, constraints) {
            final isWide = constraints.maxWidth >= 980;
            if (isWide) {
              return Column(
                children: [
                  _buildTopControls(),
                  Expanded(
                    child: Row(
                      children: [
                        Expanded(child: _buildDeviceList()),
                        Expanded(child: _buildRanging()),
                        Expanded(child: _buildLogs()),
                      ],
                    ),
                  ),
                ],
              );
            }

            return ListView(
              padding: EdgeInsets.zero,
              children: [
                _buildTopControls(),
                _buildRanging(),
                SizedBox(height: 280, child: _buildDeviceList()),
                SizedBox(height: 260, child: _buildLogs()),
              ],
            );
          },
        ),
      ),
    );
  }

  Widget _buildTopControls() {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(12),
      color: const Color(0xFFF4F6F8),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Expanded(
                child: Text(
                  'Select BLE device:',
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                  style: const TextStyle(
                    fontSize: 16,
                    fontWeight: FontWeight.bold,
                    color: Color(0xFF273671),
                  ),
                ),
              ),
              const Spacer(),
              Text(
                _isScanning ? 'Scanning...' : '${_devices.length} found',
                maxLines: 1,
                overflow: TextOverflow.ellipsis,
                style: const TextStyle(fontWeight: FontWeight.w500),
              ),
            ],
          ),
          const SizedBox(height: 8),
          Wrap(
            spacing: 8,
            runSpacing: 8,
            children: [
              ElevatedButton.icon(
                onPressed: _isScanning ? null : _scan,
                icon: const Icon(Icons.bluetooth_searching),
                label: Text(_isScanning ? 'Scanning...' : 'Scan'),
              ),
              ElevatedButton.icon(
                onPressed: (_selectedDevice != null &&
                        !_isConnected &&
                        !_isScanning &&
                        !_isConnecting)
                    ? () => _connect(_selectedDevice!)
                    : null,
                icon: _isConnecting
                    ? const SizedBox(
                        width: 16,
                        height: 16,
                        child: CircularProgressIndicator(strokeWidth: 2),
                      )
                    : const Icon(Icons.link),
                label: Text(_isConnecting ? 'Connecting...' : 'Connect'),
              ),
              OutlinedButton.icon(
                onPressed: _isConnected ? _disconnect : null,
                icon: const Icon(Icons.link_off),
                label: const Text('Disconnect'),
              ),
            ],
          ),
          const SizedBox(height: 8),
          Text(
            _isConnected && _selectedDevice != null
                ? 'Connected: ${_deviceLabelFromDevice(_selectedDevice!)} '
                    '(${_selectedDevice!.remoteId.str})'
                : _selectedDevice != null
                    ? 'Selected: ${_deviceLabelFromDevice(_selectedDevice!)} '
                        '(${_selectedDevice!.remoteId.str})'
                    : 'No device selected',
            style: const TextStyle(fontWeight: FontWeight.w500),
          ),
        ],
      ),
    );
  }

  Widget _buildRanging() {
    return UwbRangingSection(sessionReady: _isConnected);
  }

  Widget _buildDeviceList() {
    return Card(
      margin: const EdgeInsets.all(8),
      child: Column(
        children: [
          ListTile(
            dense: true,
            title: const Text('Discovered devices'),
            subtitle: Text(
              _devices.isEmpty
                  ? 'Tap Scan to search for nearby BLE devices'
                  : 'Tap one device to select, then press Connect',
            ),
          ),
          const Divider(height: 1),
          Expanded(
            child: _devices.isEmpty
                ? const Center(child: Text('No devices'))
                : ListView.separated(
                    itemCount: _devices.length,
                    separatorBuilder: (_, __) => const Divider(height: 1),
                    itemBuilder: (context, index) {
                      final r = _devices[index];
                      final name = _deviceLabel(r);
                      final isSelected = _selectedDevice?.remoteId.str
                              .toUpperCase() ==
                          r.device.remoteId.str.toUpperCase();
                      final isConnected = _isConnected &&
                          _selectedDevice?.remoteId.str.toUpperCase() ==
                              r.device.remoteId.str.toUpperCase();
                      return ListTile(
                        dense: true,
                        selected: isSelected,
                        selectedTileColor:
                            const Color(0xFF273671).withValues(alpha: 0.1),
                        leading: Icon(
                          Icons.bluetooth,
                          color: isSelected
                              ? const Color(0xFF273671)
                              : Colors.grey,
                        ),
                        title: Text(
                          name,
                          style: TextStyle(
                            fontWeight:
                                isSelected ? FontWeight.bold : FontWeight.normal,
                          ),
                        ),
                        subtitle: Text(
                          '${r.device.remoteId.str}  RSSI ${r.rssi} dBm',
                          style: const TextStyle(fontSize: 12),
                        ),
                        trailing: isConnected
                            ? const Icon(Icons.check_circle, color: Colors.green)
                            : isSelected
                                ? const Icon(
                                    Icons.radio_button_checked,
                                    color: Color(0xFF273671),
                                  )
                                : _isLikelyEspDevice(r)
                                    ? const Icon(
                                        Icons.priority_high,
                                        color: Colors.orange,
                                      )
                                    : null,
                        onTap: () {
                          setState(() => _selectedDevice = r.device);
                          _appendLog(
                            'Selected: ${_deviceLabelFromDevice(r.device)} '
                            '(${r.device.remoteId.str})',
                          );
                        },
                      );
                    },
                  ),
          ),
        ],
      ),
    );
  }

  Widget _buildLogs() {
    return Card(
      margin: const EdgeInsets.all(8),
      child: Column(
        children: [
          ListTile(
            dense: true,
            title: const Text('Logs / Notifications'),
            subtitle: Text('${_logs.length} line(s)'),
            trailing: IconButton(
              icon: const Icon(Icons.copy_outlined),
              tooltip: 'Copy logs',
              onPressed: _logs.isEmpty
                  ? null
                  : () async {
                      final allLogs = _logs.join('\n');
                      await Clipboard.setData(ClipboardData(text: allLogs));
                      if (!mounted) return;
                      ScaffoldMessenger.of(context).showSnackBar(
                        const SnackBar(
                          content: Text('Copied logs to clipboard'),
                          duration: Duration(seconds: 2),
                        ),
                      );
                    },
            ),
          ),
          const Divider(height: 1),
          Expanded(
            child: ListView.builder(
              controller: _logScroll,
              itemCount: _logs.length,
              itemBuilder: (context, index) {
                return Padding(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 12,
                    vertical: 4,
                  ),
                  child: Text(
                    _logs[index],
                    style: const TextStyle(
                      fontFamily: 'monospace',
                      fontSize: 12,
                    ),
                  ),
                );
              },
            ),
          ),
        ],
      ),
    );
  }

  String _deviceLabel(ScanResult result) {
    final advName = _advertisedName(result);
    if (advName.isNotEmpty) {
      return advName;
    }
    return _deviceLabelFromDevice(result.device);
  }

  String _deviceLabelFromDevice(BluetoothDevice device) {
    final name = device.platformName.trim();
    if (name.isNotEmpty) {
      return name;
    }
    return 'Unknown Device';
  }

  String _advertisedName(ScanResult result) {
    try {
      final adv = result.advertisementData as dynamic;
      final name = (adv.advName ?? adv.localName ?? '').toString().trim();
      if (name.isNotEmpty && name.toLowerCase() != 'null') {
        return name;
      }
    } catch (_) {}
    return '';
  }

  bool _isLikelyEspDevice(ScanResult result) {
    final name = _deviceLabel(result).toLowerCase();
    return name.contains('esp') ||
        name.contains('ecu') ||
        name.contains('yolo') ||
        name.contains('car') ||
        name.contains('smart');
  }

  int _compareScanResults(ScanResult a, ScanResult b) {
    final aPriority = _isLikelyEspDevice(a) ? 1 : 0;
    final bPriority = _isLikelyEspDevice(b) ? 1 : 0;
    if (aPriority != bPriority) {
      return bPriority.compareTo(aPriority);
    }

    final aHasName = _deviceLabel(a) != 'Unknown Device' ? 1 : 0;
    final bHasName = _deviceLabel(b) != 'Unknown Device' ? 1 : 0;
    if (aHasName != bHasName) {
      return bHasName.compareTo(aHasName);
    }

    return b.rssi.compareTo(a.rssi);
  }
}
