import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import 'package:smart_car_app/service/ble_phase_test.dart';
import 'package:smart_car_app/service/ble_runtime_permissions.dart';
import 'package:smart_car_app/service/background_service_control.dart';
import 'package:smart_car_app/service/pke_auth_orchestrator.dart';
import 'package:smart_car_app/widgets/uwb_ranging_section.dart';

/// High-level state of the unified vehicle-access flow.
enum _AccessStage {
  idle,
  scanning,
  connecting,
  authenticating,
  secured,
  error,
}

/// Unified BLE + UWB screen.
///
/// Combines the previous "Test Phase A/B" (BLE authentication) and
/// "Test UWB End-to-End" screens into a single guided flow that follows the
/// real system order:
///   1. Scan & select the vehicle (ESP32) over BLE.
///   2. Authenticate (Phase B). The first time performs a full handshake;
///      afterwards the vehicle is trusted and reconnects use the fast path
///      (no full re-authentication).
///   3. Start / stop UWB multi-anchor ranging once the secure session is up.
///
/// NFC provisioning (Phase A) is intentionally excluded from this screen.
class BleUwbScreen extends StatefulWidget {
  const BleUwbScreen({super.key});

  @override
  State<BleUwbScreen> createState() => _BleUwbScreenState();
}

class _BleUwbScreenState extends State<BleUwbScreen> {
  static const Color _primary = Color(0xFF273671);
  static const Color _accent = Color(0xFF41a5de);

  final BlePhaseTestService _ble = BlePhaseTestService();
  final BleRuntimePermissionService _permissions =
      BleRuntimePermissionService();
  final BackgroundServiceControlService _bgService =
      BackgroundServiceControlService();

  final ScrollController _logScroll = ScrollController();
  final List<String> _logs = <String>[];
  final List<ScanResult> _devices = <ScanResult>[];

  StreamSubscription<List<ScanResult>>? _scanSub;

  BluetoothDevice? _selectedDevice;
  String? _trustedAddress;
  _AccessStage _stage = _AccessStage.idle;
  bool _sessionReady = false;
  bool _bgEnabled = true;

  @override
  void initState() {
    super.initState();
    _bootstrap();
  }

  @override
  void dispose() {
    _scanSub?.cancel();
    _logScroll.dispose();
    _ble.disconnect();
    super.dispose();
  }

  Future<void> _bootstrap() async {
    final trusted = await PkeAuthOrchestrator.loadPreferredDeviceAddress();
    final bgEnabled = await _bgService.isEnabled();
    if (!mounted) return;
    setState(() {
      _trustedAddress = trusted;
      _bgEnabled = bgEnabled;
    });
    if (trusted != null) {
      _log('Trusted vehicle remembered: $trusted');
    }
  }

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------

  bool get _isBusy =>
      _stage == _AccessStage.scanning ||
      _stage == _AccessStage.connecting ||
      _stage == _AccessStage.authenticating;

  bool _isTrusted(BluetoothDevice? device) {
    if (device == null || _trustedAddress == null) return false;
    return device.remoteId.str.toUpperCase() ==
        _trustedAddress!.toUpperCase();
  }

  void _log(String line) {
    if (!mounted) return;
    setState(() {
      _logs.add(
        '[${DateTime.now().toIso8601String().substring(11, 19)}] $line',
      );
    });
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

  void _notify(String message, {bool error = false}) {
    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: error ? Colors.red.shade600 : _primary,
        behavior: SnackBarBehavior.floating,
        duration: const Duration(seconds: 3),
      ),
    );
  }

  String _deviceLabel(BluetoothDevice device) {
    final name = device.platformName.trim();
    return name.isEmpty ? 'Unknown device' : name;
  }

  bool _looksLikeVehicle(ScanResult r) {
    final name = _deviceLabel(r.device).toLowerCase();
    return name.contains('esp') ||
        name.contains('ecu') ||
        name.contains('yolo') ||
        name.contains('car') ||
        name.contains('smart');
  }

  // ---------------------------------------------------------------------------
  // BLE actions
  // ---------------------------------------------------------------------------

  Future<bool> _ensurePermissions() async {
    final status = await _permissions.ensureReady(requestIfNeeded: true);
    if (!status.ready) {
      _log('Permission issue: ${status.toUserMessage()}');
      _notify(status.toUserMessage(), error: true);
    }
    return status.ready;
  }

  Future<void> _scan() async {
    if (!await _ensurePermissions()) return;

    setState(() {
      _stage = _AccessStage.scanning;
      _devices.clear();
    });
    _log('Scanning for nearby vehicles...');

    final Map<String, ScanResult> seen = <String, ScanResult>{};
    try {
      await _scanSub?.cancel();
      _scanSub = FlutterBluePlus.scanResults.listen((results) {
        for (final r in results) {
          if (r.device.platformName.trim().isEmpty) continue;
          final key = r.device.remoteId.str.toUpperCase();
          final existing = seen[key];
          if (existing == null || r.rssi >= existing.rssi) {
            seen[key] = r;
          }
        }
        if (!mounted) return;
        final sorted = seen.values.toList()
          ..sort((a, b) {
            final ap = _looksLikeVehicle(a) ? 1 : 0;
            final bp = _looksLikeVehicle(b) ? 1 : 0;
            if (ap != bp) return bp.compareTo(ap);
            return b.rssi.compareTo(a.rssi);
          });
        setState(() {
          _devices
            ..clear()
            ..addAll(sorted);
        });
      });

      await FlutterBluePlus.stopScan();
      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 8));
      await Future<void>.delayed(const Duration(seconds: 8));
      await FlutterBluePlus.stopScan();
      _log('Found ${_devices.length} device(s)');
    } catch (e) {
      _log('Scan error: $e');
      _notify('Scan failed: $e', error: true);
    } finally {
      await _scanSub?.cancel();
      _scanSub = null;
      if (mounted && _stage == _AccessStage.scanning) {
        setState(() => _stage = _AccessStage.idle);
      }
    }
  }

  Future<void> _authenticate() async {
    final device = _selectedDevice;
    if (device == null) {
      _notify('Select a vehicle first', error: true);
      return;
    }
    if (!await _ensurePermissions()) return;

    final trusted = _isTrusted(device);
    setState(() => _stage = _AccessStage.authenticating);
    _log(trusted
        ? 'Quick unlock with trusted vehicle ${device.remoteId.str}...'
        : 'Authenticating with vehicle ${device.remoteId.str}...');

    try {
      final result = await _ble.testPhaseB(
        deviceAddress: device.remoteId.str,
        device: device,
        onProgress: (step, message) => _log('[$step] $message'),
      );

      if (!mounted) return;

      if (result.success) {
        await PkeAuthOrchestrator.savePreferredDeviceAddress(
          device.remoteId.str,
        );
        setState(() {
          _trustedAddress = device.remoteId.str;
          _sessionReady = true;
          _stage = _AccessStage.secured;
        });
        _log('Secure session established. UWB ranging unlocked.');
        _notify(trusted
            ? 'Vehicle unlocked (trusted, fast path)'
            : 'Authenticated. Vehicle is now trusted.');
      } else {
        setState(() => _stage = _AccessStage.error);
        _log('Authentication failed: ${result.message}');
        _notify('Authentication failed: ${result.message}', error: true);
      }
    } catch (e) {
      if (!mounted) return;
      setState(() => _stage = _AccessStage.error);
      _log('Authentication error: $e');
      _notify('Authentication error: $e', error: true);
    }
  }

  Future<void> _endSession() async {
    _log('Ending secure session...');
    try {
      await _ble.disconnect();
    } catch (_) {}
    if (!mounted) return;
    setState(() {
      _sessionReady = false;
      _stage = _AccessStage.idle;
    });
    _log('Session ended. BLE disconnected, UWB stopped.');
    _notify('Session ended');
  }

  Future<void> _forgetVehicle() async {
    await PkeAuthOrchestrator.savePreferredDeviceAddress('');
    if (!mounted) return;
    setState(() => _trustedAddress = null);
    _log('Trusted vehicle forgotten. Next connect needs full authentication.');
    _notify('Trusted vehicle removed');
  }

  Future<void> _toggleBgService() async {
    try {
      final enabled = await _bgService.toggleService();
      if (!mounted) return;
      setState(() => _bgEnabled = enabled);
      _notify(enabled
          ? 'Background auto-unlock service enabled'
          : 'Background auto-unlock service disabled');
    } catch (e) {
      _notify('Could not change background service: $e', error: true);
    }
  }

  // ---------------------------------------------------------------------------
  // UI
  // ---------------------------------------------------------------------------

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFFF5F7FA),
      appBar: AppBar(
        backgroundColor: _primary,
        elevation: 0,
        iconTheme: const IconThemeData(color: Colors.white),
        title: const Text(
          'Vehicle Access',
          style: TextStyle(fontWeight: FontWeight.bold, color: Colors.white),
        ),
        actions: [
          IconButton(
            icon: const Icon(Icons.help_outline),
            tooltip: 'Background service guide',
            onPressed: _showBackgroundGuide,
          ),
          IconButton(
            icon: const Icon(Icons.delete_outline),
            tooltip: 'Clear logs',
            onPressed: () => setState(_logs.clear),
          ),
        ],
      ),
      body: ListView(
        padding: const EdgeInsets.only(bottom: 24),
        children: [
          _buildStatusHeader(),
          _buildStep1Ble(),
          _buildStep2Uwb(),
          _buildBackgroundCard(),
          _buildLogs(),
        ],
      ),
    );
  }

  Widget _buildStatusHeader() {
    late final String label;
    late final IconData icon;
    late final Color color;
    switch (_stage) {
      case _AccessStage.idle:
        label = 'Ready to connect';
        icon = Icons.bluetooth;
        color = Colors.white70;
        break;
      case _AccessStage.scanning:
        label = 'Scanning for vehicles...';
        icon = Icons.bluetooth_searching;
        color = Colors.amberAccent;
        break;
      case _AccessStage.connecting:
        label = 'Connecting...';
        icon = Icons.link;
        color = Colors.amberAccent;
        break;
      case _AccessStage.authenticating:
        label = 'Authenticating...';
        icon = Icons.lock_clock;
        color = Colors.amberAccent;
        break;
      case _AccessStage.secured:
        label = 'Secure session active';
        icon = Icons.verified_user;
        color = Colors.greenAccent;
        break;
      case _AccessStage.error:
        label = 'Something went wrong';
        icon = Icons.error_outline;
        color = Colors.redAccent;
        break;
    }

    return Container(
      width: double.infinity,
      padding: const EdgeInsets.fromLTRB(20, 16, 20, 24),
      decoration: const BoxDecoration(
        color: _primary,
        borderRadius: BorderRadius.only(
          bottomLeft: Radius.circular(24),
          bottomRight: Radius.circular(24),
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(icon, color: color, size: 28),
              const SizedBox(width: 12),
              Expanded(
                child: Text(
                  label,
                  style: TextStyle(
                    color: color,
                    fontSize: 18,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ),
              if (_isBusy)
                const SizedBox(
                  width: 18,
                  height: 18,
                  child: CircularProgressIndicator(
                    strokeWidth: 2,
                    color: Colors.white,
                  ),
                ),
            ],
          ),
          const SizedBox(height: 12),
          Row(
            children: [
              _buildStagePill('1. BLE Auth',
                  _sessionReady || _stage == _AccessStage.authenticating),
              const SizedBox(width: 8),
              _buildStagePill('2. UWB Ranging', _sessionReady),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildStagePill(String label, bool active) {
    return Expanded(
      child: Container(
        padding: const EdgeInsets.symmetric(vertical: 8),
        alignment: Alignment.center,
        decoration: BoxDecoration(
          color: active
              ? Colors.greenAccent.withValues(alpha: 0.2)
              : Colors.white.withValues(alpha: 0.08),
          borderRadius: BorderRadius.circular(10),
          border: Border.all(
            color: active ? Colors.greenAccent : Colors.white24,
          ),
        ),
        child: Text(
          label,
          style: TextStyle(
            color: active ? Colors.greenAccent : Colors.white70,
            fontWeight: FontWeight.w600,
            fontSize: 13,
          ),
        ),
      ),
    );
  }

  Widget _buildStep1Ble() {
    final selected = _selectedDevice;
    final trusted = _isTrusted(selected);
    return Card(
      margin: const EdgeInsets.fromLTRB(12, 16, 12, 6),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.bluetooth, color: _primary),
                const SizedBox(width: 8),
                const Expanded(
                  child: Text(
                    'Step 1 — BLE authentication',
                    style: TextStyle(
                      fontWeight: FontWeight.bold,
                      fontSize: 16,
                      color: _primary,
                    ),
                  ),
                ),
                TextButton.icon(
                  onPressed: _isBusy ? null : _scan,
                  icon: _stage == _AccessStage.scanning
                      ? const SizedBox(
                          width: 16,
                          height: 16,
                          child: CircularProgressIndicator(strokeWidth: 2),
                        )
                      : const Icon(Icons.refresh),
                  label: Text(
                    _stage == _AccessStage.scanning ? 'Scanning' : 'Scan',
                  ),
                ),
              ],
            ),
            const SizedBox(height: 4),
            if (_trustedAddress != null)
              _buildTrustedBanner(),
            const SizedBox(height: 8),
            _buildDeviceList(),
            const SizedBox(height: 12),
            if (selected != null)
              Text(
                'Selected: ${_deviceLabel(selected)} (${selected.remoteId.str})',
                style: const TextStyle(fontWeight: FontWeight.w500),
              ),
            const SizedBox(height: 12),
            if (!_sessionReady)
              SizedBox(
                width: double.infinity,
                child: ElevatedButton.icon(
                  onPressed: (_isBusy || selected == null)
                      ? null
                      : _authenticate,
                  icon: Icon(trusted ? Icons.lock_open : Icons.verified_user),
                  label: Text(
                    trusted
                        ? 'Quick unlock (trusted)'
                        : 'Authenticate & secure session',
                  ),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: trusted ? Colors.green.shade600 : _primary,
                    foregroundColor: Colors.white,
                    padding: const EdgeInsets.symmetric(vertical: 14),
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(12),
                    ),
                  ),
                ),
              )
            else
              SizedBox(
                width: double.infinity,
                child: OutlinedButton.icon(
                  onPressed: _endSession,
                  icon: const Icon(Icons.link_off),
                  label: const Text('End session'),
                  style: OutlinedButton.styleFrom(
                    foregroundColor: Colors.red.shade600,
                    padding: const EdgeInsets.symmetric(vertical: 14),
                    side: BorderSide(color: Colors.red.shade200),
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(12),
                    ),
                  ),
                ),
              ),
          ],
        ),
      ),
    );
  }

  Widget _buildTrustedBanner() {
    return Container(
      margin: const EdgeInsets.only(top: 8),
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
      decoration: BoxDecoration(
        color: Colors.green.withValues(alpha: 0.08),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: Colors.green.withValues(alpha: 0.3)),
      ),
      child: Row(
        children: [
          const Icon(Icons.shield, color: Colors.green, size: 20),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              'Trusted vehicle: $_trustedAddress\n'
              'Reconnects use the fast path — no full re-authentication.',
              style: const TextStyle(fontSize: 12),
            ),
          ),
          TextButton(
            onPressed: _forgetVehicle,
            child: const Text('Forget'),
          ),
        ],
      ),
    );
  }

  Widget _buildDeviceList() {
    if (_devices.isEmpty) {
      return Container(
        height: 90,
        alignment: Alignment.center,
        decoration: BoxDecoration(
          color: const Color(0xFFF0F2F5),
          borderRadius: BorderRadius.circular(12),
        ),
        child: Text(
          _stage == _AccessStage.scanning
              ? 'Searching...'
              : 'Tap Scan to discover nearby vehicles',
          style: const TextStyle(color: Colors.grey),
        ),
      );
    }

    return Container(
      constraints: const BoxConstraints(maxHeight: 200),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: const Color(0xFFE0E3E8)),
      ),
      child: ListView.separated(
        shrinkWrap: true,
        itemCount: _devices.length,
        separatorBuilder: (_, __) => const Divider(height: 1),
        itemBuilder: (context, index) {
          final r = _devices[index];
          final isSelected = _selectedDevice?.remoteId == r.device.remoteId;
          final trusted = _isTrusted(r.device);
          return ListTile(
            dense: true,
            selected: isSelected,
            selectedTileColor: _primary.withValues(alpha: 0.08),
            leading: Icon(
              trusted ? Icons.shield : Icons.bluetooth,
              color: trusted
                  ? Colors.green
                  : (isSelected ? _primary : Colors.grey),
            ),
            title: Text(
              _deviceLabel(r.device),
              style: TextStyle(
                fontWeight: isSelected ? FontWeight.bold : FontWeight.normal,
              ),
            ),
            subtitle: Text(
              '${r.device.remoteId.str}   RSSI ${r.rssi} dBm'
              '${trusted ? '   • trusted' : ''}',
              style: const TextStyle(fontSize: 12),
            ),
            trailing: isSelected
                ? const Icon(Icons.radio_button_checked, color: _primary)
                : (_looksLikeVehicle(r)
                    ? const Icon(Icons.directions_car, color: _accent)
                    : null),
            onTap: _isBusy
                ? null
                : () {
                    setState(() => _selectedDevice = r.device);
                    _log('Selected ${_deviceLabel(r.device)} '
                        '(${r.device.remoteId.str})');
                  },
          );
        },
      ),
    );
  }

  Widget _buildStep2Uwb() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Padding(
          padding: EdgeInsets.fromLTRB(20, 12, 20, 0),
          child: Text(
            'Step 2 — UWB multi-anchor ranging',
            style: TextStyle(
              fontWeight: FontWeight.bold,
              fontSize: 16,
              color: _primary,
            ),
          ),
        ),
        UwbRangingSection(
          sessionReady: _sessionReady,
          onStart: () async {
            final ok = await _ble.requestRangingStart();
            if (!ok) {
              _log('Warning: ECU did not acknowledge ranging start');
            } else {
              _log('Requested anchors to start ranging');
            }
          },
          onStop: () async {
            final ok = await _ble.requestRangingStop();
            if (!ok) {
              _log('Warning: ECU did not acknowledge ranging stop');
            } else {
              _log('Requested anchors to stop ranging');
            }
          },
        ),
      ],
    );
  }

  Widget _buildBackgroundCard() {
    return Card(
      margin: const EdgeInsets.fromLTRB(12, 6, 12, 6),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
        child: Column(
          children: [
            SwitchListTile(
              contentPadding: EdgeInsets.zero,
              value: _bgEnabled,
              activeThumbColor: Colors.green,
              onChanged: (_) => _toggleBgService(),
              secondary: Icon(
                _bgEnabled ? Icons.cloud_done : Icons.cloud_off,
                color: _bgEnabled ? Colors.green : Colors.grey,
              ),
              title: const Text(
                'Background auto-unlock',
                style: TextStyle(fontWeight: FontWeight.bold),
              ),
              subtitle: Text(
                _bgEnabled
                    ? 'App keeps watching for your trusted vehicle in the background.'
                    : 'Disabled — you must open the app to unlock.',
                style: const TextStyle(fontSize: 12),
              ),
            ),
            Align(
              alignment: Alignment.centerLeft,
              child: TextButton.icon(
                onPressed: _showBackgroundGuide,
                icon: const Icon(Icons.info_outline, size: 18),
                label: const Text('How does background unlock work?'),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildLogs() {
    return Container(
      margin: const EdgeInsets.fromLTRB(12, 6, 12, 6),
      decoration: BoxDecoration(
        color: Colors.black87,
        borderRadius: BorderRadius.circular(16),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Padding(
            padding: const EdgeInsets.all(12),
            child: Row(
              children: [
                const Icon(Icons.terminal, color: Colors.greenAccent, size: 20),
                const SizedBox(width: 8),
                const Text(
                  'Activity log',
                  style: TextStyle(
                    color: Colors.greenAccent,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                const Spacer(),
                Text(
                  '${_logs.length} lines',
                  style: TextStyle(color: Colors.grey[500], fontSize: 12),
                ),
                const SizedBox(width: 8),
                IconButton(
                  icon: const Icon(Icons.copy, size: 18),
                  color: Colors.greenAccent,
                  padding: EdgeInsets.zero,
                  constraints: const BoxConstraints(),
                  tooltip: 'Copy logs',
                  onPressed: _logs.isEmpty
                      ? null
                      : () {
                          Clipboard.setData(
                            ClipboardData(text: _logs.join('\n')),
                          );
                          _notify('Logs copied to clipboard');
                        },
                ),
              ],
            ),
          ),
          const Divider(color: Colors.white24, height: 1),
          SizedBox(
            height: 220,
            child: _logs.isEmpty
                ? Center(
                    child: Text(
                      'No activity yet.',
                      style: TextStyle(color: Colors.grey[600]),
                    ),
                  )
                : ListView.builder(
                    controller: _logScroll,
                    padding: const EdgeInsets.all(12),
                    itemCount: _logs.length,
                    itemBuilder: (context, index) {
                      final line = _logs[index];
                      Color color = Colors.white;
                      if (line.contains('failed') ||
                          line.contains('error') ||
                          line.contains('Error')) {
                        color = Colors.redAccent;
                      } else if (line.contains('Secure session') ||
                          line.contains('unlocked') ||
                          line.contains('established')) {
                        color = Colors.greenAccent;
                      } else if (line.contains('[Step') ||
                          line.contains('Attempt')) {
                        color = Colors.yellowAccent;
                      }
                      return Padding(
                        padding: const EdgeInsets.only(bottom: 4),
                        child: SelectableText(
                          line,
                          style: TextStyle(
                            fontFamily: 'monospace',
                            fontSize: 12,
                            color: color,
                            height: 1.4,
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

  void _showBackgroundGuide() {
    showModalBottomSheet<void>(
      context: context,
      isScrollControlled: true,
      shape: const RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(20)),
      ),
      builder: (context) {
        return DraggableScrollableSheet(
          expand: false,
          initialChildSize: 0.7,
          maxChildSize: 0.9,
          builder: (context, controller) {
            return SingleChildScrollView(
              controller: controller,
              padding: const EdgeInsets.all(20),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Center(
                    child: Container(
                      width: 40,
                      height: 4,
                      margin: const EdgeInsets.only(bottom: 16),
                      decoration: BoxDecoration(
                        color: Colors.grey.shade300,
                        borderRadius: BorderRadius.circular(2),
                      ),
                    ),
                  ),
                  const Text(
                    'Background auto-unlock',
                    style: TextStyle(
                      fontSize: 20,
                      fontWeight: FontWeight.bold,
                      color: _primary,
                    ),
                  ),
                  const SizedBox(height: 12),
                  const Text(
                    'When enabled, the app runs a small foreground service that '
                    'keeps scanning for your trusted vehicle. As you walk up to '
                    'the car it authenticates over BLE automatically and hands '
                    'off to UWB — no need to open the app.',
                    style: TextStyle(fontSize: 14, height: 1.5),
                  ),
                  const SizedBox(height: 20),
                  _guideStep(
                    '1',
                    'Authenticate once',
                    'Open this screen, scan, and authenticate with your '
                        'vehicle. It becomes a trusted device.',
                  ),
                  _guideStep(
                    '2',
                    'Enable background auto-unlock',
                    'Turn on the switch above. A persistent notification shows '
                        'the service is watching for your car.',
                  ),
                  _guideStep(
                    '3',
                    'Allow the permissions',
                    'Grant Nearby devices / Bluetooth and Location "Allow all '
                        'the time" so scanning works while the app is closed.',
                  ),
                  _guideStep(
                    '4',
                    'Disable battery optimisation',
                    'Exempt the app from Doze / battery optimisation so Android '
                        'does not kill the service in the background.',
                  ),
                  _guideStep(
                    '5',
                    'Walk up to your car',
                    'The service reconnects using the fast path, verifies '
                        'distance with UWB, and unlocks — automatically.',
                  ),
                  const SizedBox(height: 12),
                  Container(
                    padding: const EdgeInsets.all(12),
                    decoration: BoxDecoration(
                      color: _accent.withValues(alpha: 0.08),
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: const Row(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Icon(Icons.lightbulb_outline, color: _accent),
                        SizedBox(width: 8),
                        Expanded(
                          child: Text(
                            'Tip: If auto-unlock stops working after a while, '
                            'check that battery optimisation is still disabled '
                            'and the trusted vehicle is still remembered.',
                            style: TextStyle(fontSize: 13, height: 1.4),
                          ),
                        ),
                      ],
                    ),
                  ),
                  const SizedBox(height: 20),
                  SizedBox(
                    width: double.infinity,
                    child: ElevatedButton(
                      onPressed: () => Navigator.of(context).pop(),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: _primary,
                        foregroundColor: Colors.white,
                        padding: const EdgeInsets.symmetric(vertical: 14),
                      ),
                      child: const Text('Got it'),
                    ),
                  ),
                ],
              ),
            );
          },
        );
      },
    );
  }

  Widget _guideStep(String number, String title, String body) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 16),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          CircleAvatar(
            radius: 14,
            backgroundColor: _primary,
            child: Text(
              number,
              style: const TextStyle(
                color: Colors.white,
                fontWeight: FontWeight.bold,
                fontSize: 13,
              ),
            ),
          ),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  title,
                  style: const TextStyle(
                    fontWeight: FontWeight.bold,
                    fontSize: 15,
                  ),
                ),
                const SizedBox(height: 2),
                Text(
                  body,
                  style: const TextStyle(fontSize: 13, height: 1.4),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
