import 'dart:async';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:smart_car_app/service/uwb_service.dart';

class TestUwbScreen extends StatefulWidget {
  const TestUwbScreen({super.key});

  @override
  State<TestUwbScreen> createState() => _TestUwbScreenState();
}

class _TestUwbScreenState extends State<TestUwbScreen> {
  final UwbService _uwb = UwbService();

  final ScrollController _logScroll = ScrollController();
  final TextEditingController _payloadCtrl = TextEditingController();
  final TextEditingController _sessionIdCtrl = TextEditingController(text: '42');
  final TextEditingController _phoneMacCtrl = TextEditingController(text: '0x0001');
  final TextEditingController _carMacCtrl = TextEditingController(text: '0x0002');
  final TextEditingController _channelCtrl = TextEditingController(text: '9');

  final List<String> _logs = <String>[];
  final List<ScanResult> _devices = <ScanResult>[];

  StreamSubscription<String>? _logSub;
  StreamSubscription<Uint8List>? _infoSub;
  StreamSubscription<UwbOobPayload>? _oobSub;
  StreamSubscription<UwbRangingEvent>? _rangingSub;
  StreamSubscription<List<ScanResult>>? _scanSub;

  bool _isScanning = false;
  bool _isSending = false;
  bool _isStartingUwb = false;
  BluetoothDevice? _selectedDevice;
  UwbOobPayload? _lastOob;
  UwbRangingEvent? _lastRangingEvent;
  Map<dynamic, dynamic>? _lastPrepareSession;

  @override
  void initState() {
    super.initState();
    _logSub = _uwb.logs.listen(_appendLog);
    _infoSub = _uwb.infoNotifications.listen((bytes) {
      _appendLog('INFO NOTIFY: ${UwbService.toHex(bytes)}');
    });
    _oobSub = _uwb.oobPayloads.listen((oob) {
      if (!mounted) return;
      setState(() => _lastOob = oob);
    });
    _rangingSub = _uwb.rangingEvents.listen((event) {
      if (!mounted) return;
      setState(() => _lastRangingEvent = event);
      
      // Add visual feedback on the phone when in unlock zone
      if (event.distanceM != null) {
        if (event.distanceM! <= 2.0) {
          _appendLog('🎯 [UNLOCK ZONE] Distance: ${event.distanceM!.toStringAsFixed(2)}m');
        } else if (event.distanceM! > 3.0) {
          _appendLog('✓ [RESET ZONE] Distance: ${event.distanceM!.toStringAsFixed(2)}m - Ready to unlock again');
        }
      }
    });
    _buildDefaultPayload();
  }

  @override
  void dispose() {
    _logSub?.cancel();
    _infoSub?.cancel();
    _oobSub?.cancel();
    _rangingSub?.cancel();
    _scanSub?.cancel();
    _uwb.dispose();
    _logScroll.dispose();
    _payloadCtrl.dispose();
    _sessionIdCtrl.dispose();
    _phoneMacCtrl.dispose();
    _carMacCtrl.dispose();
    _channelCtrl.dispose();
    super.dispose();
  }

  void _appendLog(String line) {
    if (!mounted) return;
    setState(() => _logs.add(line));
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
      await _uwb.ensureBluetoothReady();
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
    try {
      await _uwb.connect(device);
      if (!mounted) return;
      setState(() => _selectedDevice = device);
      _appendLog(
        'Connected: ${_deviceLabelFromDevice(device)} (${device.remoteId.str})',
      );
    } catch (e) {
      _appendLog('Connect error: $e');
    }
  }

  Future<void> _disconnect() async {
    await _uwb.disconnect();
    if (!mounted) return;
    setState(() {
      _selectedDevice = null;
      _lastPrepareSession = null;
      _lastOob = null;
      _lastRangingEvent = null;
    });
  }

  void _selectDevice(BluetoothDevice device) {
    setState(() => _selectedDevice = device);
    _appendLog(
      'Selected: ${_deviceLabelFromDevice(device)} (${device.remoteId.str})',
    );
  }

  Future<void> _connectSelected() async {
    final device = _selectedDevice;
    if (device == null) {
      _appendLog('Please select a device first');
      return;
    }
    await _connect(device);
  }

  void _buildDefaultPayload() {
    try {
      final sessionId = int.parse(_sessionIdCtrl.text.trim());
      final phoneMac = _parseFlexibleInt(_phoneMacCtrl.text.trim());
      final carMac = _parseFlexibleInt(_carMacCtrl.text.trim());
      final channel = int.parse(_channelCtrl.text.trim());
      _uwb.clearPreparedContext();

      final payload = _uwb.buildDefaultOobPayloadV1(
        sessionId: sessionId,
        phoneMac: phoneMac,
        carMac: carMac,
        channel: channel,
      );
      _payloadCtrl.text = UwbService.toHex(payload);
      final parsed = _uwb.parseOobPayload(payload);
      if (mounted) {
        setState(() {
          _lastOob = parsed;
          _lastPrepareSession = null;
          _lastRangingEvent = null;
        });
      } else {
        _lastOob = parsed;
        _lastPrepareSession = null;
        _lastRangingEvent = null;
      }
      _appendLog('Generated payload V1: ${payload.length} bytes');
    } catch (e) {
      _appendLog('Payload build error: $e');
    }
  }

  Future<void> _sendOob() async {
    setState(() => _isSending = true);
    try {
      final prepared = await _preparePayloadWithLocalUwbAddress();
      final bytes = prepared['payload'] as Uint8List;
      final session = prepared['session'];
      await _uwb.sendOobPayload(bytes);
      _appendLog('OOB cached on ECU using local UWB ${session is Map ? session['localAddress'] ?? '-' : '-'}');
    } catch (e) {
      _appendLog('Send OOB failed: $e');
    } finally {
      if (mounted) {
        setState(() => _isSending = false);
      }
    }
  }

  Future<void> _sendStatusCmd() async {
    try {
      await _uwb.sendAdminCmd(0x33);
    } catch (e) {
      _appendLog('Status command failed: $e');
    }
  }

  Future<void> _joinFromPayload() async {
    setState(() => _isStartingUwb = true);
    try {
      final bytes = UwbService.parseHex(_payloadCtrl.text);
      final result = await _uwb.joinSessionFromOob(bytes);
      if (!mounted) return;
      setState(() {
        final session = result['session'];
        _lastPrepareSession = session is Map ? Map<dynamic, dynamic>.from(session) : null;
      });
      _appendLog('UWB start requested on ECU and phone ranging started');
    } catch (e) {
      _appendLog('Join UWB failed: $e');
    } finally {
      if (mounted) {
        setState(() => _isStartingUwb = false);
      }
    }
  }

  Future<Map<String, Object?>> _preparePayloadWithLocalUwbAddress() async {
    final bytes = UwbService.parseHex(_payloadCtrl.text);
    final prepared = await _uwb.preparePayloadForOob(bytes);
    final patchedPayload = prepared['payload'] as Uint8List;
    final oobMap = Map<dynamic, dynamic>.from(prepared['oob'] as Map);
    final session = prepared['session'];
    final patchedOob = _uwb.parseOobPayload(patchedPayload);

    if (!mounted) {
      return prepared;
    }

    setState(() {
      _payloadCtrl.text = UwbService.toHex(patchedPayload);
      _phoneMacCtrl.text = '0x${patchedOob.phoneMac.toRadixString(16).padLeft(4, '0').toUpperCase()}';
      _lastOob = patchedOob;
      _lastPrepareSession = session is Map ? Map<dynamic, dynamic>.from(session) : null;
    });

    _appendLog('Patched OOB phoneMac to ${oobMap['phoneMacString']} from local UWB address');
    _appendLog('Forced roles: phone=controller, car=controlee');
    return prepared;
  }

  Future<void> _stopRanging() async {
    try {
      await _uwb.stopRanging();
      if (!mounted) return;
      setState(() {
        _lastPrepareSession = null;
      });
      _appendLog('Requested UWB stop');
    } catch (e) {
      _appendLog('Stop ranging failed: $e');
    }
  }

  Map<String, Object?>? _currentPreparedContext() {
    try {
      final bytes = UwbService.parseHex(_payloadCtrl.text);
      return _uwb.getPreparedContext(bytes);
    } catch (_) {
      return null;
    }
  }

  bool _isPreparedPayloadCurrent() {
    return _currentPreparedContext() != null;
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
                        Expanded(child: _buildUwbStatus()),
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
                SizedBox(height: 280, child: _buildDeviceList()),
                SizedBox(height: 360, child: _buildUwbStatus()),
                SizedBox(height: 260, child: _buildLogs()),
              ],
            );
          },
        ),
      ),
    );
  }

  Widget _buildTopControls() {
    final isPreparedFlowReady = _isPreparedPayloadCurrent();
    final preparedLocalAddress = _lastPrepareSession?['localAddress']?.toString();

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
                onPressed: (_selectedDevice != null && !_uwb.isConnected && !_isScanning)
                    ? _connectSelected
                    : null,
                icon: const Icon(Icons.link),
                label: const Text('Connect'),
              ),
              OutlinedButton.icon(
                onPressed: _uwb.isConnected ? _disconnect : null,
                icon: const Icon(Icons.link_off),
                label: const Text('Disconnect'),
              ),
            ],
          ),
          const SizedBox(height: 8),
          Wrap(
            spacing: 8,
            runSpacing: 8,
            children: [
              SizedBox(width: 180, child: _smallField(_sessionIdCtrl, 'Session ID (dec)')),
              SizedBox(width: 190, child: _smallField(_phoneMacCtrl, 'Phone MAC (hex)')),
              SizedBox(width: 180, child: _smallField(_carMacCtrl, 'Car MAC (hex)')),
              SizedBox(width: 120, child: _smallField(_channelCtrl, 'Channel')),
              ElevatedButton(
                onPressed: _buildDefaultPayload,
                child: const Text('Generate Payload'),
              ),
              ElevatedButton(
                onPressed: _uwb.isConnected ? _sendStatusCmd : null,
                child: const Text('Status 0x33'),
              ),
              ElevatedButton(
                onPressed: (_uwb.isConnected && !_isStartingUwb && isPreparedFlowReady) ? _joinFromPayload : null,
                child: Text(_isStartingUwb ? 'Starting UWB...' : 'Join UWB From Payload'),
              ),
              OutlinedButton(
                onPressed: _stopRanging,
                child: const Text('Stop Ranging'),
              ),
            ],
          ),
          const SizedBox(height: 8),
          TextField(
            controller: _payloadCtrl,
            maxLines: 2,
            onChanged: (_) {
              final prepared = _currentPreparedContext();
              if (!mounted) return;
              setState(() {
                final session = prepared?['session'];
                _lastPrepareSession = session is Map ? Map<dynamic, dynamic>.from(session) : null;
              });
            },
            decoration: const InputDecoration(
              border: OutlineInputBorder(),
              labelText: 'OOB payload hex (37 bytes)',
              hintText: '01 01 2A 00 00 00 ...',
            ),
          ),
          const SizedBox(height: 8),
          SizedBox(
            width: double.infinity,
            child: ElevatedButton.icon(
              onPressed: _uwb.isConnected && !_isSending ? _sendOob : null,
              icon: const Icon(Icons.send),
              label: Text(_isSending ? 'Sending...' : 'Send OOB to Admin 0005'),
            ),
          ),
          const SizedBox(height: 4),
          Text(
            isPreparedFlowReady
                ? 'Prepared flow ready. Join will ask ECU to start the cached config, then reuse local UWB ${preparedLocalAddress ?? '-'} and the same phone MAC on the phone side.'
                : 'Press Send OOB first. That step prepares the phone UWB session, patches phone MAC into this payload, and caches the exact config on ECU for Join to start.',
            style: const TextStyle(fontSize: 12, color: Colors.black87),
          ),
          const SizedBox(height: 4),
          Align(
            alignment: Alignment.centerLeft,
            child: Text(
              _uwb.isConnected && _selectedDevice != null
                  ? 'Connected: ${_deviceLabelFromDevice(_selectedDevice!)} (${_selectedDevice!.remoteId.str})'
                  : _selectedDevice != null
                  ? 'Selected: ${_deviceLabelFromDevice(_selectedDevice!)} (${_selectedDevice!.remoteId.str})'
                  : 'No device selected',
              style: const TextStyle(fontWeight: FontWeight.w500),
            ),
          ),
        ],
      ),
    );
  }

  Widget _smallField(TextEditingController ctrl, String label) {
    return TextField(
      controller: ctrl,
      decoration: InputDecoration(
        border: const OutlineInputBorder(),
        labelText: label,
        isDense: true,
      ),
    );
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
                      final isSelected =
                          _selectedDevice?.remoteId.str.toUpperCase() == r.device.remoteId.str.toUpperCase();
                      final isConnected =
                          _uwb.connectedDevice?.remoteId.str.toUpperCase() == r.device.remoteId.str.toUpperCase();
                      return ListTile(
                        dense: true,
                        selected: isSelected,
                        selectedTileColor: const Color(0xFF273671).withAlpha(20),
                        leading: Icon(
                          Icons.bluetooth,
                          color: isSelected ? const Color(0xFF273671) : Colors.grey,
                        ),
                        title: Text(
                          name,
                          style: TextStyle(
                            fontWeight: isSelected ? FontWeight.bold : FontWeight.normal,
                          ),
                        ),
                        subtitle: Text(
                          '${r.device.remoteId.str}  RSSI ${r.rssi} dBm',
                          style: const TextStyle(fontSize: 12),
                        ),
                        trailing: isConnected
                            ? const Icon(Icons.check_circle, color: Colors.green)
                            : isSelected
                                ? const Icon(Icons.radio_button_checked, color: Color(0xFF273671))
                                : _isLikelyEspDevice(r)
                                    ? const Icon(Icons.priority_high, color: Colors.orange)
                                    : null,
                        onTap: () => _selectDevice(r.device),
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
                  padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
                  child: Text(
                    _logs[index],
                    style: const TextStyle(fontFamily: 'monospace', fontSize: 12),
                  ),
                );
              },
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildUwbStatus() {
    final oob = _lastOob;
    final ranging = _lastRangingEvent;
    final localAddress = _lastPrepareSession?['localAddress']?.toString();

    return Card(
      margin: const EdgeInsets.all(8),
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'UWB Session / Ranging',
              style: TextStyle(fontSize: 16, fontWeight: FontWeight.w600),
            ),
            const SizedBox(height: 12),
            _infoRow('Prepared local UWB', localAddress ?? '-'),
            _infoRow('Event type', ranging?.type ?? '-'),
            _infoRow('Distance', _formatDistance(ranging?.distanceM)),
            _infoRow('Azimuth', _formatAngle(ranging?.azimuthDeg)),
            _infoRow('Elevation', _formatAngle(ranging?.elevationDeg)),
            _infoRow('Elapsed', ranging?.elapsedMs == null ? '-' : '${ranging!.elapsedMs} ms'),
            _infoRow('Peer seen', ranging?.positionSeen?.toString() ?? '-'),
            const Divider(height: 24),
            const Text(
              'Last OOB payload',
              style: TextStyle(fontSize: 15, fontWeight: FontWeight.w600),
            ),
            const SizedBox(height: 8),
            Expanded(
              child: oob == null
                  ? const Center(child: Text('No OOB parsed yet'))
                  : SingleChildScrollView(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          _infoRow('Phone role', 'controller'),
                          _infoRow('Car role', oob.carIsControlee ? 'controlee' : 'controller'),
                          _infoRow('Session ID', '${oob.sessionId}'),
                          _infoRow('Phone MAC', oob.phoneMacString),
                          _infoRow('Car MAC', oob.carMacString),
                          _infoRow('Remote address', oob.remoteAddress),
                          _infoRow('Channel', '${oob.channel}'),
                          _infoRow('Preamble index', '${oob.preambleIdx}'),
                          _infoRow('Slot duration', '${oob.slotDuration}'),
                          _infoRow('Ranging interval', '${oob.rangingInterval}'),
                          _infoRow('Static STS IV', UwbService.toHex(oob.staticStsIv)),
                        ],
                      ),
                    ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _infoRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 6),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 120,
            child: Text(
              label,
              style: const TextStyle(fontWeight: FontWeight.w600),
            ),
          ),
          Expanded(child: Text(value)),
        ],
      ),
    );
  }

  int _parseFlexibleInt(String input) {
    if (input.toLowerCase().startsWith('0x')) {
      return int.parse(input.substring(2), radix: 16);
    }
    return int.parse(input);
  }

  String _formatDistance(double? value) {
    if (value == null) return '-';
    return '${value.toStringAsFixed(2)} m';
  }

  String _formatAngle(double? value) {
    if (value == null) return '-';
    return '${value.toStringAsFixed(2)} deg';
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
