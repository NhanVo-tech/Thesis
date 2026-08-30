import 'dart:async';

import 'package:flutter/material.dart';
import 'package:geolocator/geolocator.dart';
import 'package:url_launcher/url_launcher.dart';
import 'package:smart_car_app/theme/app_colors.dart';
import 'package:smart_car_app/widgets/location_components.dart';

import '../service/ble_phase_test.dart';
import '../service/pke_auth_orchestrator.dart';
import '../service/ble_runtime_permissions.dart';
import '../service/gps_service.dart';

class LocationContent extends StatefulWidget {
  const LocationContent({super.key});

  @override
  State<LocationContent> createState() => _LocationContentState();
}

class _LocationContentState extends State<LocationContent> {
  final BlePhaseTestService _bleService = BlePhaseTestService();
  final BleRuntimePermissionService _blePermissionService =
      BleRuntimePermissionService();
  final GpsService _gpsService = GpsService();
  final TextEditingController _deviceAddressController =
      TextEditingController();

  Timer? _autoSyncTimer;
  Position? _lastPosition;
  String _address = 'Unknown area';
  String _locationInfo = 'Waiting for first GPS fix';
  String _statusMessage = 'Location screen ready';
  String _bleStatus = 'Disconnected';
  bool _isBusy = false;
  bool _isAuthenticated = false;
  DateTime? _lastRefreshAt;
  DateTime? _lastBleSyncAt;
  int _sentPacketCount = 0;

  @override
  void initState() {
    super.initState();
    _loadSavedDeviceAddress();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) {
        return;
      }
      _refreshLocationAndSync();
      _startAutoSync();
    });
  }

  @override
  void dispose() {
    _autoSyncTimer?.cancel();
    _deviceAddressController.dispose();
    _bleService.disconnect();
    super.dispose();
  }

  Future<void> _loadSavedDeviceAddress() async {
    final savedAddress =
        await PkeAuthOrchestrator.loadPreferredDeviceAddress() ?? '';
    if (!mounted) {
      _deviceAddressController.text = savedAddress;
      return;
    }

    setState(() {
      _deviceAddressController.text = savedAddress;
      if (savedAddress.isNotEmpty) {
        _bleStatus = 'Saved ESP32 MAC ready';
      }
    });
  }

  void _startAutoSync() {
    _autoSyncTimer?.cancel();
    _autoSyncTimer = Timer.periodic(const Duration(seconds: 30), (_) {
      if (!mounted || _isBusy) {
        return;
      }
      _refreshLocationAndSync(silent: true);
    });
  }

  Future<void> _saveDeviceAddress({bool showFeedback = true}) async {
    final deviceAddress = _deviceAddressController.text.trim();
    await PkeAuthOrchestrator.savePreferredDeviceAddress(deviceAddress);

    if (showFeedback && mounted) {
      _showSnackBar('Saved ESP32 MAC address', AppColors.success);
    }
  }

  Future<void> _connectBle() async {
    final deviceAddress = _deviceAddressController.text.trim();
    if (deviceAddress.isEmpty) {
      _showSnackBar('Enter ESP32 MAC address first', AppColors.error);
      return;
    }

    final permissionStatus = await _blePermissionService.ensureReady(
      requestIfNeeded: true,
    );
    if (!permissionStatus.ready) {
      if (mounted) {
        setState(() {
          _bleStatus = 'Missing runtime permissions';
          _statusMessage = permissionStatus.toUserMessage();
        });
      }
      _showSnackBar(permissionStatus.toUserMessage(), Colors.orange);
      return;
    }

    await _saveDeviceAddress(showFeedback: false);

    setState(() {
      _isBusy = true;
      _statusMessage = 'Connecting to ESP32 over BLE...';
      _bleStatus = 'Authenticating...';
      _isAuthenticated = false;
    });

    try {
      final result = await _bleService.testPhaseB(
        deviceAddress: deviceAddress,
        timeout: const Duration(seconds: 30),
      );

      if (!mounted) {
        return;
      }

      setState(() {
        _isAuthenticated = result.success;
        _bleStatus = result.success
            ? 'Authenticated and ready'
            : 'Authentication failed';
        _statusMessage = result.success
            ? 'BLE ready. GPS will sync to ESP32 every 30 seconds.'
            : result.message;
      });

      _showSnackBar(
        result.success ? 'BLE connected successfully' : 'BLE authentication failed',
        result.success ? AppColors.success : AppColors.error,
      );

      if (result.success) {
        await _refreshLocationAndSync(silent: true, forceSync: true);
      }
    } catch (e) {
      if (!mounted) {
        return;
      }

      setState(() {
        _isAuthenticated = false;
        _bleStatus = 'BLE error';
        _statusMessage = 'BLE connection failed: $e';
      });
      _showSnackBar('BLE connection failed: $e', AppColors.error);
    } finally {
      if (mounted) {
        setState(() {
          _isBusy = false;
        });
      }
    }
  }

  Future<void> _refreshLocationAndSync({
    bool silent = false,
    bool forceSync = false,
  }) async {
    if (_isBusy && !silent) {
      return;
    }

    if (mounted) {
      setState(() {
        _isBusy = true;
        _statusMessage = silent
            ? 'Refreshing location in background...'
            : 'Getting current location...';
      });
    }

    try {
      final hasPermission = await _gpsService.checkAndRequestPermissions();
      if (!hasPermission) {
        if (mounted) {
          setState(() {
            _statusMessage = 'Location permission denied';
            _locationInfo =
                'Enable location permission to keep GPS sync active';
          });
        }
        if (!silent) {
          _showSnackBar('Location permission denied', AppColors.error);
        }
        return;
      }

      final position = await _gpsService.getCurrentPosition(
        timeout: const Duration(seconds: 15),
      );

      if (position == null) {
        if (mounted) {
          setState(() {
            _statusMessage = 'Failed to get GPS position';
          });
        }
        if (!silent) {
          _showSnackBar('Failed to get GPS position', AppColors.error);
        }
        return;
      }

      final address =
          await _gpsService.getAddressFromPosition(position) ?? 'Unknown area';
      final refreshedAt = DateTime.now();

      if (mounted) {
        setState(() {
          _lastPosition = position;
          _address = address;
          _lastRefreshAt = refreshedAt;
          _locationInfo =
              'Place:     $address\n'
              'Latitude:  ${position.latitude.toStringAsFixed(6)}\n'
              'Longitude: ${position.longitude.toStringAsFixed(6)}\n'
              'Altitude:  ${position.altitude.toStringAsFixed(1)} m\n'
              'Accuracy:  ${position.accuracy.toStringAsFixed(1)} m\n'
              'Timestamp: ${position.timestamp}';
          _statusMessage = 'Location updated';
        });
      }

      final shouldSendToBle = _isAuthenticated || forceSync;
      if (!shouldSendToBle) {
        if (mounted) {
          setState(() {
            _bleStatus = _deviceAddressController.text.trim().isEmpty
                ? 'Enter ESP32 MAC to enable BLE sync'
                : 'Connect BLE to start sync';
          });
        }
        return;
      }

      final packet = _gpsService.buildEncryptedLocationPacket(
        position,
        _bleService.sessionEncKey!,
        _bleService.sessionMacKey!,
      );

      if (packet == null) {
        if (mounted) {
          setState(() {
            _bleStatus = 'Failed to prepare BLE packet';
          });
        }
        return;
      }

      final sent = await _bleService.sendGpsPacket(packet);
      if (!mounted) {
        return;
      }

      if (sent) {
        setState(() {
          _lastBleSyncAt = DateTime.now();
          _sentPacketCount += 1;
          _bleStatus = 'Last GPS packet sent successfully';
          _statusMessage = 'Location refreshed and sent to ESP32';
        });
      } else {
        setState(() {
          _bleStatus = 'BLE send failed';
          _statusMessage = 'Location updated locally, but BLE send failed';
        });
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          _statusMessage = 'Location error: $e';
          _bleStatus = 'Sync interrupted';
        });
      }
      if (!silent) {
        _showSnackBar('Location error: $e', AppColors.error);
      }
    } finally {
      if (mounted) {
        setState(() {
          _isBusy = false;
        });
      }
    }
  }

  Future<void> _openInGoogleMaps() async {
    final position = _lastPosition;
    if (position == null) {
      _showSnackBar('No location available yet', AppColors.warning);
      return;
    }

    final uri = Uri.parse(
      'https://www.google.com/maps/search/?api=1&query=${position.latitude},${position.longitude}',
    );

    final launched = await launchUrl(uri, mode: LaunchMode.externalApplication);
    if (!launched && mounted) {
      _showSnackBar('Could not open Google Maps', AppColors.error);
    }
  }

  String _formatDateTime(DateTime? value) {
    if (value == null) {
      return 'Not yet';
    }

    final hh = value.hour.toString().padLeft(2, '0');
    final mm = value.minute.toString().padLeft(2, '0');
    final ss = value.second.toString().padLeft(2, '0');
    return '$hh:$mm:$ss';
  }

  void _showSnackBar(String message, Color backgroundColor) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text(message), backgroundColor: backgroundColor),
    );
  }

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          _buildOverviewCard(),
          const SizedBox(height: 16),
          _buildBleSetupCard(),
          const SizedBox(height: 16),
          _buildLocationCard(),
          const SizedBox(height: 16),
          _buildActionButtons(),
        ],
      ),
    );
  }

  Widget _buildOverviewCard() {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        gradient: const LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [AppColors.primary, AppColors.secondary],
        ),
        borderRadius: BorderRadius.circular(20),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Row(
            children: [
              Icon(Icons.map_outlined, color: Colors.white),
              SizedBox(width: 8),
              Text(
                'Live Location Sync',
                style: TextStyle(
                  color: Colors.white,
                  fontSize: 20,
                  fontWeight: FontWeight.bold,
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          Text(
            _statusMessage,
            style: const TextStyle(color: Colors.white, fontSize: 14),
          ),
          const SizedBox(height: 16),
          Wrap(
            spacing: 8,
            runSpacing: 8,
            children: [
              _buildInfoChip('Auto refresh', '30s'),
              _buildInfoChip(
                'BLE',
                _isAuthenticated ? 'Ready' : 'Disconnected',
              ),
              _buildInfoChip('Packets sent', '$_sentPacketCount'),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildBleSetupCard() {
    return _buildSectionCard(
      title: 'ESP32 BLE',
      icon: Icons.bluetooth_connected,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          TextField(
            controller: _deviceAddressController,
            decoration: const InputDecoration(
              labelText: 'ESP32 MAC address',
              hintText: 'XX:XX:XX:XX:XX:XX',
              border: OutlineInputBorder(),
              prefixIcon: Icon(Icons.memory_outlined),
            ),
            textCapitalization: TextCapitalization.characters,
          ),
          const SizedBox(height: 12),
          Text(
            'Status: $_bleStatus',
            style: TextStyle(
              color: _isAuthenticated ? Colors.green[700] : Colors.grey[700],
              fontWeight: FontWeight.w600,
            ),
          ),
          const SizedBox(height: 12),
          Row(
            children: [
              Expanded(
                child: OutlinedButton.icon(
                  onPressed: _isBusy ? null : _saveDeviceAddress,
                  icon: const Icon(Icons.save_outlined),
                  label: const Text('Save MAC'),
                  style: OutlinedButton.styleFrom(
                    padding: const EdgeInsets.symmetric(vertical: 14),
                    side: const BorderSide(width: 1.5),
                  ),
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: ElevatedButton.icon(
                  onPressed: _isBusy ? null : _connectBle,
                  icon: const Icon(Icons.bluetooth_searching),
                  label: const Text('Connect BLE'),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: AppColors.primary,
                    foregroundColor: Colors.white,
                    padding: const EdgeInsets.symmetric(vertical: 14),
                  ),
                ),
              ),
            ],
          ),
          const SizedBox(height: 8),
          Text(
            'Once BLE authentication succeeds, this screen sends GPS data to ESP32 every 30 seconds.',
            style: TextStyle(fontSize: 12, color: Colors.grey[600]),
          ),
        ],
      ),
    );
  }

  Widget _buildLocationCard() {
    return _buildSectionCard(
      title: 'Current Location',
      icon: Icons.location_on_outlined,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // GPS Status Indicator
          GpsStatusIndicator(
            position: _lastPosition,
            isConnected: _lastRefreshAt != null,
            lastUpdate: _lastRefreshAt,
          ),
          const SizedBox(height: 16),

          // Location Satellite Card
          LocationSatelliteCard(
            position: _lastPosition,
            address: _address,
          ),
          const SizedBox(height: 16),

          // Accuracy Meter
          AccuracyMeter(accuracyMeters: _lastPosition?.accuracy),
          const SizedBox(height: 12),

          // Speed and Heading
          SpeedAndHeadingCard(position: _lastPosition),
          const SizedBox(height: 16),

          // Detailed Location Info
          Container(
            width: double.infinity,
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              color: const Color(0xFFF5F7FB),
              borderRadius: BorderRadius.circular(16),
            ),
            child: Text(
              _locationInfo,
              style: const TextStyle(fontFamily: 'monospace', height: 1.5, fontSize: 11),
            ),
          ),
          const SizedBox(height: 12),

          // Last Update Times
          Row(
            children: [
              Expanded(
                child: _buildMetaTile(
                  'Last refresh',
                  _formatDateTime(_lastRefreshAt),
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: _buildMetaTile(
                  'Last BLE sync',
                  _formatDateTime(_lastBleSyncAt),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildActionButtons() {
    return Row(
      children: [
        Expanded(
          child: OutlinedButton.icon(
            onPressed: _isBusy ? null : _refreshLocationAndSync,
            icon: const Icon(Icons.refresh),
            label: const Text('Refresh now'),
            style: OutlinedButton.styleFrom(
              padding: const EdgeInsets.symmetric(vertical: 14),
              side: const BorderSide(width: 1.5),
            ),
          ),
        ),
        const SizedBox(width: 12),
        Expanded(
          child: ElevatedButton.icon(
            onPressed: _openInGoogleMaps,
            icon: const Icon(Icons.map),
            label: const Text('Google Maps'),
            style: ElevatedButton.styleFrom(
              backgroundColor: const Color(0xFF41a5de),
              foregroundColor: Colors.white,
              padding: const EdgeInsets.symmetric(vertical: 14),
            ),
          ),
        ),
      ],
    );
  }

  Widget _buildSectionCard({
    required String title,
    required IconData icon,
    required Widget child,
  }) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(20),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.05),
            blurRadius: 12,
            offset: const Offset(0, 4),
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(icon, color: const Color(0xFF273671)),
              const SizedBox(width: 8),
              Text(
                title,
                style: const TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                  color: Color(0xFF273671),
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),
          child,
        ],
      ),
    );
  }

  Widget _buildInfoChip(String label, String value) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.16),
        borderRadius: BorderRadius.circular(999),
      ),
      child: Text(
        '$label: $value',
        style: const TextStyle(
          color: Colors.white,
          fontWeight: FontWeight.w600,
        ),
      ),
    );
  }

  Widget _buildMetaTile(String label, String value) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: const Color(0xFFF5F7FB),
        borderRadius: BorderRadius.circular(14),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(label, style: TextStyle(fontSize: 12, color: Colors.grey[600])),
          const SizedBox(height: 4),
          Text(
            value,
            style: const TextStyle(
              fontSize: 14,
              fontWeight: FontWeight.w700,
              color: Color(0xFF273671),
            ),
          ),
        ],
      ),
    );
  }
}
