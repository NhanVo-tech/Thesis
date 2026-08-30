import 'package:flutter/material.dart';
import 'package:flutter/foundation.dart';
import 'package:smart_car_app/screen/location.dart';
import 'package:smart_car_app/screen/profile.dart';
import 'package:smart_car_app/screen/test_phase_ab.dart';
import 'package:smart_car_app/screen/test_uwb.dart';
import 'package:smart_car_app/screen/master_card_flow.dart';
import 'package:smart_car_app/screen/settings.dart';
import 'package:smart_car_app/screen/notifications.dart';
import 'package:smart_car_app/screen/ai_test_harness_v2.dart';
import 'package:smart_car_app/widgets/car_dialogs.dart';
import 'package:smart_car_app/widgets/dashboard_widgets.dart';
import 'package:smart_car_app/widgets/key_components.dart';
import 'package:smart_car_app/service/car_service.dart';
import 'package:smart_car_app/service/ble_runtime_permissions.dart';
import 'package:smart_car_app/service/doze_exemption_service.dart';
import 'package:smart_car_app/service/initial_data_helper.dart';
import 'package:smart_car_app/service/master_card_provisioning.dart';
import 'package:smart_car_app/service/language_service.dart';
import 'package:smart_car_app/service/background_service_control.dart';
import 'package:flutter/services.dart';
import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:smart_car_app/theme/app_colors.dart';
import 'package:smart_car_app/widgets/app_components.dart';

class Dashboard extends StatefulWidget {
  const Dashboard({super.key});

  @override
  State<Dashboard> createState() => _DashboardState();
}

class _DashboardState extends State<Dashboard> {
  int _currentIndex = 0;
  final CarService _carService = CarService();
  final MasterCardProvisioningService _masterCardService =
      MasterCardProvisioningService();
  late LanguageService _languageService;
  final BleRuntimePermissionService _blePermissionService =
      BleRuntimePermissionService();
  final DozeExemptionService _dozeExemptionService = DozeExemptionService();
  late BackgroundServiceControlService _bgServiceControl;
  bool _bgServiceEnabled = true;

  List<Map<String, dynamic>> _cars = [];
  List<Map<String, dynamic>> _digitalKeys = [];
  bool _isLoading = true;
  DateTime? _lastSyncTime;
  bool _permissionCheckRunning = false;
  bool _dozeCheckRunning = false;
  bool _permissionWarningShown = false;
  bool _dozeWarningShown = false;
  BleRuntimePermissionStatus? _blePermissionStatus;
  DozeExemptionStatus? _dozeExemptionStatus;
  final Map<String, MasterCardPayload> _pendingVehicleProvision = {};

  @override
  void initState() {
    super.initState();
    _languageService = LanguageService.instance;
    _languageService.addListener(_onLanguageChanged);
    _bgServiceControl = BackgroundServiceControlService();
    _initializeBgServiceState();
    _loadData();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _checkBleRuntimePermissions(requestIfNeeded: true);
      _checkDozeExemptionStatus(showWarning: true);
    });
    // Show sample data dialog after a short delay
    Future.delayed(const Duration(milliseconds: 500), () {
      if (mounted) {
        InitialDataHelper.addSampleDataIfNeeded(context);
      }
    });
  }

  Future<void> _initializeBgServiceState() async {
    final enabled = await _bgServiceControl.isEnabled();
    if (mounted) {
      setState(() {
        _bgServiceEnabled = enabled;
      });
    }
  }

  Future<void> _handleBgServiceToggle() async {
    try {
      final newState = await _bgServiceControl.toggleService();
      if (mounted) {
        setState(() {
          _bgServiceEnabled = newState;
        });
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(
              _bgServiceEnabled
                  ? 'Background service enabled'
                  : 'Background service disabled',
            ),
            duration: const Duration(seconds: 2),
          ),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Error toggling service: $e'),
            backgroundColor: Colors.red,
          ),
        );
      }
    }
  }

  void _onLanguageChanged() {
    print('Dashboard: Language changed, rebuilding UI'); // Debug log
    if (mounted) {
      setState(() {});
    }
  }

  @override
  void dispose() {
    _languageService.removeListener(_onLanguageChanged);
    super.dispose();
  }
  Future<void> _checkDozeExemptionStatus({bool showWarning = false}) async {
    if (_dozeCheckRunning) {
      return;
    }

    _dozeCheckRunning = true;
    try {
      final status = await _dozeExemptionService.getStatus();
      if (!mounted) {
        return;
      }

      setState(() {
        _dozeExemptionStatus = status;
      });

      if (showWarning && status.needsExemption && !_dozeWarningShown) {
        _dozeWarningShown = true;
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(status.toUserMessage()),
            backgroundColor: Colors.orange,
          ),
        );
      }
    } finally {
      _dozeCheckRunning = false;
    }
  }

  Future<void> _checkBleRuntimePermissions({
    bool requestIfNeeded = false,
  }) async {
    if (_permissionCheckRunning) {
      return;
    }

    _permissionCheckRunning = true;
    try {
      final status = await _blePermissionService.ensureReady(
        requestIfNeeded: requestIfNeeded,
      );
      if (!mounted) {
        return;
      }

      setState(() {
        _blePermissionStatus = status;
      });

      if (status.isDegraded && !_permissionWarningShown) {
        _permissionWarningShown = true;
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(status.toUserMessage()),
            backgroundColor: Colors.orange,
          ),
        );
      }
    } finally {
      _permissionCheckRunning = false;
    }
  }

  void _loadData() {
    // Listen to cars stream
    _carService.getUserCars().listen(
      (cars) {
        if (mounted) {
          _lastSyncTime = DateTime.now();
          final normalizedCars = cars.map((car) {
            // Convert color string to Color object if needed
            if (car['color'] is String) {
              switch (car['color']) {
                case 'blue':
                  car['color'] = Colors.blue;
                  break;
                case 'purple':
                  car['color'] = Colors.purple;
                  break;
                case 'orange':
                  car['color'] = Colors.orange;
                  break;
                case 'green':
                  car['color'] = Colors.green;
                  break;
                default:
                  car['color'] = Colors.blue;
              }
            }
            return car;
          }).toList();
          setState(() {
            _cars = normalizedCars;
            _isLoading = false;
          });
          _refreshPendingProvision(normalizedCars);
        }
      },
      onError: (error) {
        if (mounted) {
          setState(() {
            _isLoading = false;
          });
          AppSnackBar.showError(context, '${_languageService.translate('failed_to_load_vehicles')}: $error');
        }
      },
    ); // Listen to digital keys stream
    _carService.getUserDigitalKeys().listen(
      (keys) {
        if (mounted) {
          setState(() {
            _digitalKeys = keys;
          });
        }
      },
      onError: (error) {
        if (mounted) {
          AppSnackBar.showError(context, '${_languageService.translate('failed_to_load_keys')}: $error');
        }
      },
    );
  }

  /// Helper method to format last sync time
  String _getLastSyncText() {
    if (_lastSyncTime == null) return _languageService.translate('loading');
    
    final now = DateTime.now();
    final difference = now.difference(_lastSyncTime!);
    
    if (difference.inSeconds < 60) {
      return _languageService.translate('just_now');
    } else if (difference.inMinutes < 60) {
      return '${difference.inMinutes}m ${_languageService.translate('ago')}';
    } else if (difference.inHours < 24) {
      return '${difference.inHours}h ${_languageService.translate('ago')}';
    } else {
      return '${difference.inDays}d ${_languageService.translate('ago')}';
    }
  }

  /// Refresh data manually
  Future<void> _refreshData() async {
    if (!mounted) return;
    setState(() => _isLoading = true);
    _loadData();
    await Future.delayed(const Duration(seconds: 1));
    if (mounted) {
      setState(() => _isLoading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    print('Dashboard: Building UI, current language: ${_languageService.currentLanguage}');
    return Scaffold(
      backgroundColor: AppColors.background,
      appBar: _buildAppBar(),
      body: SafeArea(
        child: _currentIndex == 3
            ? ProfileScreen()
            : _currentIndex == 2
            ? const LocationContent()
            : _currentIndex == 4
            ? const AITestHarnessV2()
            : _buildDashboardContent(),
      ),
      bottomNavigationBar: _buildBottomNavigationBar(),
      floatingActionButton: _currentIndex == 0
          ? FloatingActionButton(
              onPressed: _showAddCarDialog,
              backgroundColor: AppColors.primary,
              child: const Icon(Icons.add, color: Colors.white),
            )
          : _currentIndex == 1
          ? FloatingActionButton(
              onPressed: _showAddDigitalKeyDialog,
              backgroundColor: AppColors.primary,
              child: const Icon(Icons.vpn_key, color: Colors.white),
            )
          : null,
    );
  }

  PreferredSizeWidget _buildAppBar() {
    String title = '';
    switch (_currentIndex) {
      case 0:
        title = _languageService.translate('home');
        break;
      case 1:
        title = _languageService.translate('digital_keys');
        break;
      case 2:
        title = _languageService.translate('location');
        break;
      case 3:
        title = _languageService.translate('profile');
        break;
      case 4:
        title = 'AI Test Harness';
        break;
    }

    return AppBar(
      title: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Text(
            title,
            style: const TextStyle(
              fontSize: 24,
              fontWeight: FontWeight.bold,
              color: AppColors.textPrimary,
            ),
          ),
          if (_currentIndex == 0)
            Text(
              '${_languageService.translate('updated')} ${_getLastSyncText()}',
              style: const TextStyle(
                fontSize: 12,
                color: AppColors.textSecondary,
                fontWeight: FontWeight.w400,
              ),
            ),
        ],
      ),
      backgroundColor: Colors.transparent,
      elevation: 0,
      centerTitle: false,
      actions: [
        IconButton(
          icon: Icon(
            _bgServiceEnabled ? Icons.cloud_done : Icons.cloud_off,
            color: _bgServiceEnabled ? Colors.green : Colors.grey,
          ),
          tooltip: _bgServiceEnabled ? 'BLE Service On' : 'BLE Service Off',
          onPressed: _handleBgServiceToggle,
        ),
        IconButton(
          icon: const Icon(Icons.science_outlined, color: AppColors.textPrimary),
          tooltip: 'Test Phase A/B',
          onPressed: () {
            Navigator.of(context).push(
              MaterialPageRoute(builder: (_) => const TestPhaseABScreen()),
            );
          },
        ),
        IconButton(
          icon: const Icon(Icons.radar_outlined, color: Color(0xFF273671)),
          tooltip: 'Test UWB/OOB',
          onPressed: () {
            Navigator.of(context).push(
              MaterialPageRoute(builder: (_) => const TestUwbScreen()),
            );
          },
        ),
        IconButton(
          icon: const Icon(
            Icons.notifications_outlined,
            color: AppColors.textPrimary,
            semanticLabel: 'Notifications',
          ),
          tooltip: 'Notifications',
          onPressed: () {
            Navigator.of(context).push(
              MaterialPageRoute(builder: (_) => const NotificationsScreen()),
            );
          },
        ),
        IconButton(
          icon: const Icon(
            Icons.settings_outlined,
            color: AppColors.textPrimary,
            semanticLabel: 'Settings',
          ),
          tooltip: 'Settings',
          onPressed: () {
            Navigator.of(context).push(
              MaterialPageRoute(builder: (_) => const SettingsScreen()),
            );
          },
        ),
      ],
    );
  }

  Widget _buildDashboardContent() {
    if (_isLoading) {
      return const Center(
        child: CircularProgressIndicator(color: AppColors.primary),
      );
    }

    return RefreshIndicator(
      onRefresh: _refreshData,
      color: AppColors.primary,
      backgroundColor: Colors.white,
      child: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        physics: const AlwaysScrollableScrollPhysics(),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            if (_blePermissionStatus?.isDegraded == true) ...[
              _buildBlePermissionWarningCard(),
              const SizedBox(height: 16),
            ],
            if (_dozeExemptionStatus?.needsExemption == true) ...[
              _buildDozeExemptionCard(),
              const SizedBox(height: 16),
            ],
            if (_currentIndex == 0) ...[
              _buildQuickStats(),
              const SizedBox(height: 24),
              _buildMyVehicles(),
            ] else ...[
              _buildDigitalKeysSection(),
            ],
          ],
        ),
      ),
    );
  }

  Widget _buildQuickStats() {
    return Column(
      children: [
        Row(
          children: [
            Expanded(
              child: StatCard(
                title: _languageService.translate('total_cars'),
                value: '${_cars.length}',
                icon: Icons.directions_car,
                color: AppColors.primary,
              ),
            ),
            const SizedBox(width: 16),
            Expanded(
              child: StatCard(
                title: _languageService.translate('active_keys'),
                value:
                    '${_digitalKeys.where((k) => k['status'] == 'Active').length}',
                icon: Icons.vpn_key,
                color: AppColors.secondary,
              ),
            ),
          ],
        ),
        const SizedBox(height: 16),
        Row(
          children: [
            Expanded(
              child: StatCard(
                title: _languageService.translate('available'),
                value:
                    '${_cars.where((c) => c['keyStatus'] == 'Active').length}',
                icon: Icons.check_circle,
                color: Colors.green,
              ),
            ),
            const SizedBox(width: 16),
            Expanded(
              child: QuickActionCard(
                title: _languageService.translate('quick_actions'),
                subtitle: _languageService.translate('control_vehicles'),
                icon: Icons.touch_app,
                color: const Color(0xFF41a5de),
                onTap: () {
                  setState(() {
                    _currentIndex = 1;
                  });
                },
              ),
            ),
          ],
        ),
      ],
    );
  }

  Widget _buildBlePermissionWarningCard() {
    final status = _blePermissionStatus;
    if (status == null || !status.isDegraded) {
      return const SizedBox.shrink();
    }

    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(14),
      decoration: BoxDecoration(
        color: Colors.orange.withOpacity(0.1),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: Colors.orange.withOpacity(0.6)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Row(
            children: [
              Icon(Icons.warning_amber_rounded, color: Colors.orange),
              SizedBox(width: 8),
              Expanded(
                child: Text(
                  'Background BLE is in degraded mode',
                  style: TextStyle(
                    fontWeight: FontWeight.w700,
                    color: Color(0xFF273671),
                  ),
                ),
              ),
            ],
          ),
          const SizedBox(height: 8),
          Text(
            status.toUserMessage(),
            style: const TextStyle(color: Color(0xFF273671)),
          ),
          const SizedBox(height: 10),
          Wrap(
            spacing: 8,
            runSpacing: 8,
            children: [
              ElevatedButton.icon(
                onPressed: () =>
                    _checkBleRuntimePermissions(requestIfNeeded: true),
                icon: const Icon(Icons.security),
                label: const Text('Grant permissions'),
                style: ElevatedButton.styleFrom(
                  backgroundColor: const Color(0xFF273671),
                  foregroundColor: Colors.white,
                ),
              ),
              OutlinedButton.icon(
                onPressed: _blePermissionService.openSettings,
                icon: const Icon(Icons.settings),
                label: const Text('Open settings'),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildDozeExemptionCard() {
    final status = _dozeExemptionStatus;
    if (status == null || !status.needsExemption) {
      return const SizedBox.shrink();
    }

    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(14),
      decoration: BoxDecoration(
        color: const Color(0xFF41a5de).withOpacity(0.1),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: const Color(0xFF41a5de).withOpacity(0.6)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Row(
            children: [
              Icon(Icons.battery_alert_outlined, color: Color(0xFF273671)),
              SizedBox(width: 8),
              Expanded(
                child: Text(
                  'Battery optimization may block background unlock',
                  style: TextStyle(
                    fontWeight: FontWeight.w700,
                    color: Color(0xFF273671),
                  ),
                ),
              ),
            ],
          ),
          const SizedBox(height: 8),
          Text(
            status.toUserMessage(),
            style: const TextStyle(color: Color(0xFF273671)),
          ),
          const SizedBox(height: 10),
          Wrap(
            spacing: 8,
            runSpacing: 8,
            children: [
              ElevatedButton.icon(
                onPressed: () async {
                  final launched = await _dozeExemptionService
                      .requestExemption();
                  if (!mounted) return;
                  if (!launched) {
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(
                        content: Text('Could not open doze exemption request.'),
                        backgroundColor: Colors.red,
                      ),
                    );
                    return;
                  }
                  await Future.delayed(const Duration(milliseconds: 400));
                  if (!mounted) return;
                  await _checkDozeExemptionStatus();
                },
                icon: const Icon(Icons.battery_saver),
                label: const Text('Request exemption'),
                style: ElevatedButton.styleFrom(
                  backgroundColor: const Color(0xFF273671),
                  foregroundColor: Colors.white,
                ),
              ),
              OutlinedButton.icon(
                onPressed: () async {
                  await _dozeExemptionService.openBatteryOptimizationSettings();
                },
                icon: const Icon(Icons.tune),
                label: const Text('Battery settings'),
              ),
              TextButton.icon(
                onPressed: _checkDozeExemptionStatus,
                icon: const Icon(Icons.refresh),
                label: const Text('Refresh status'),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildMyVehicles() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          _languageService.translate('my_vehicles'),
          style: const TextStyle(
            fontSize: 20,
            fontWeight: FontWeight.bold,
            color: Color(0xFF273671),
          ),
        ),
        const SizedBox(height: 16),
        _cars.isEmpty
            ? Container(
                padding: const EdgeInsets.all(32),
                decoration: BoxDecoration(
                  color: Colors.white,
                  borderRadius: BorderRadius.circular(16),
                  boxShadow: [
                    BoxShadow(
                      color: Colors.black.withOpacity(0.05),
                      blurRadius: 10,
                      offset: const Offset(0, 2),
                    ),
                  ],
                ),
                child: Center(
                  child: Column(
                    children: [
                      Icon(
                        Icons.directions_car_outlined,
                        size: 64,
                        color: Colors.grey[400],
                      ),
                      const SizedBox(height: 16),
                      Text(
                        'No vehicles added yet',
                        style: TextStyle(
                          fontSize: 18,
                          fontWeight: FontWeight.w500,
                          color: Colors.grey[600],
                        ),
                      ),
                      const SizedBox(height: 8),
                      Text(
                        'Tap the + button to add your first vehicle',
                        style: TextStyle(fontSize: 14, color: Colors.grey[500]),
                      ),
                      const SizedBox(height: 16),
                      OutlinedButton.icon(
                        onPressed: () {
                          InitialDataHelper.addSampleDataIfNeeded(context);
                        },
                        icon: const Icon(Icons.auto_awesome),
                        label: Text(_languageService.translate('add_sample_data')),
                        style: OutlinedButton.styleFrom(
                          foregroundColor: const Color(0xFF273671),
                        ),
                      ),
                    ],
                  ),
                ),
              )
            : ListView.builder(
                shrinkWrap: true,
                physics: const NeverScrollableScrollPhysics(),
                itemCount: _cars.length,
                itemBuilder: (context, index) {
                  final car = _cars[index];
                  final carId = car['id'] as String?;
                  final provisioned = car['provisioned'] == true;
                  final pendingPayload = (carId != null)
                      ? _pendingVehicleProvision[carId]
                      : null;

                  return Column(
                    children: [
                      VehicleStatusCard(
                        vehicle: car,
                        onTap: () => _handleVehicleControlTap(car),
                        onLockToggle: () => _toggleLock(car),
                        onControl: () => _handleVehicleControlTap(car),
                        onDelete: () => _confirmDeleteVehicle(car),
                        isProvisioned: provisioned,
                      ),
                      if (!provisioned && carId != null) ...[
                        Padding(
                          padding: const EdgeInsets.only(bottom: 16),
                          child: SizedBox(
                            width: double.infinity,
                            child: ElevatedButton.icon(
                              onPressed: () async {
                                if (pendingPayload != null) {
                                  await _startProvisionForVehicle(
                                    car,
                                    pendingPayload,
                                  );
                                  return;
                                }
                                await _scanMasterCardForVehicle(
                                  carId,
                                  car['name']?.toString() ?? 'Vehicle',
                                );
                              },
                              icon: const Icon(Icons.nfc),
                              label: Text(
                                pendingPayload != null
                                    ? 'Provision for this vehicle'
                                    : 'Scan master card',
                              ),
                              style: ElevatedButton.styleFrom(
                                backgroundColor: const Color(0xFF273671),
                                foregroundColor: Colors.white,
                                shape: RoundedRectangleBorder(
                                  borderRadius: BorderRadius.circular(12),
                                ),
                              ),
                            ),
                          ),
                        ),
                      ],
                    ],
                  );
                },
              ),
      ],
    );
  }

  Widget _buildDigitalKeysSection() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // Add Key button only, no title
        Align(
          alignment: Alignment.centerRight,
          child: SizedBox(
            width: 200, // Make the button even narrower
            child: ElevatedButton.icon(
              onPressed: _showAddDigitalKeyDialog,
              icon: const Icon(Icons.add),
              label: Text(_languageService.translate('add_key')),
              style: ElevatedButton.styleFrom(
                backgroundColor: const Color(0xFF273671),
                foregroundColor: Colors.white,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(12),
                ),
              ),
            ),
          ),
        ),
        const SizedBox(height: 16),
        _digitalKeys.isEmpty
            ? Container(
                padding: const EdgeInsets.all(32),
                decoration: BoxDecoration(
                  color: Colors.white,
                  borderRadius: BorderRadius.circular(16),
                  boxShadow: [
                    BoxShadow(
                      color: Colors.black.withOpacity(0.05),
                      blurRadius: 10,
                      offset: const Offset(0, 2),
                    ),
                  ],
                ),
                child: Center(
                  child: Column(
                    children: [
                      Icon(
                        Icons.vpn_key_outlined,
                        size: 64,
                        color: Colors.grey[400],
                      ),
                      const SizedBox(height: 16),
                      Text(
                        _languageService.translate('no_keys_created'),
                        style: TextStyle(
                          fontSize: 18,
                          fontWeight: FontWeight.w500,
                          color: Colors.grey[600],
                        ),
                      ),
                      const SizedBox(height: 8),
                      Text(
                        _languageService.translate('create_keys_description'),
                        style: TextStyle(fontSize: 14, color: Colors.grey[500]),
                      ),
                    ],
                  ),
                ),
              )
            : ListView.builder(
                shrinkWrap: true,
                physics: const NeverScrollableScrollPhysics(),
                itemCount: _digitalKeys.length,
                itemBuilder: (context, index) {
                  return _buildDigitalKeyCard(_digitalKeys[index]);
                },
              ),
      ],
    );
  }

  Widget _buildDigitalKeyCard(Map<String, dynamic> key) {
    final car = _cars.firstWhere(
      (c) => c['id'] == key['carId'],
      orElse: () => {},
    );
    final permissions = (key['permissions'] as List<dynamic>).cast<String>().toList();
    final validUntil = DateTime.parse(key['validUntil'] ?? DateTime.now().add(const Duration(days: 30)).toIso8601String());

    return Container(
      margin: const EdgeInsets.only(bottom: 16),
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(16),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withValues(alpha: 0.05),
            blurRadius: 10,
            offset: const Offset(0, 2),
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Header with key info and badges
          Row(
            children: [
              Container(
                width: 50,
                height: 50,
                decoration: BoxDecoration(
                  color: AppColors.primary.withValues(alpha: 0.1),
                  borderRadius: BorderRadius.circular(12),
                ),
                child: const Icon(
                  Icons.vpn_key,
                  color: AppColors.primary,
                  size: 24,
                ),
              ),
              const SizedBox(width: 16),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      key['name'],
                      style: const TextStyle(
                        fontSize: 16,
                        fontWeight: FontWeight.bold,
                        color: AppColors.primary,
                      ),
                    ),
                    Text(
                      car.isNotEmpty ? car['name'] : 'Unknown Vehicle',
                      style: TextStyle(color: Colors.grey[600], fontSize: 14),
                    ),
                  ],
                ),
              ),
              // Key type badge
              KeyTypeBadge(keyType: key['type'] ?? 'Guest'),
              const SizedBox(width: 8),
              IconButton(
                onPressed: () => _confirmDeleteDigitalKey(key),
                icon: const Icon(Icons.delete_outline),
                color: Colors.red[400],
                tooltip: 'Delete key',
              ),
            ],
          ),
          const SizedBox(height: 12),
          
          // Status and expiration
          Row(
            children: [
              _buildStatusChip(key['status']),
              const SizedBox(width: 8),
              Expanded(
                child: ExpirationBadge(expirationDate: validUntil),
              ),
            ],
          ),
          const SizedBox(height: 12),
          
          // Permissions header
          Text(
            '${_languageService.translate('permissions')} (${permissions.length})',
            style: const TextStyle(
              fontSize: 12,
              fontWeight: FontWeight.bold,
              color: AppColors.textPrimary,
            ),
          ),
          const SizedBox(height: 8),
          
          // Permissions display
          PermissionIndicator(
            permissions: permissions,
            compact: false,
          ),
          const SizedBox(height: 12),
          
          // Quick actions
          Row(
            children: [
              Expanded(
                child: KeyShareButton(
                  keyId: key['id'],
                  onShare: () => _showShareKeyDialog(key),
                ),
              ),
              const SizedBox(width: 8),
              ElevatedButton.icon(
                onPressed: () => _showKeyDetails(key),
                icon: const Icon(Icons.info_outline, size: 16),
                label: Text(_languageService.translate('details'), style: TextStyle(fontSize: 12)),
                style: ElevatedButton.styleFrom(
                  backgroundColor: Colors.grey[200],
                  foregroundColor: AppColors.primary,
                  padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
                  shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(8),
                  ),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  /// Show share key dialog with QR code
  void _showShareKeyDialog(Map<String, dynamic> key) {
    showDialog(
      context: context,
      builder: (ctx) => Dialog(
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Text(
                'Share ${key['name']}',
                style: const TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                ),
              ),
              const SizedBox(height: 16),
              Container(
                width: 150,
                height: 150,
                decoration: BoxDecoration(
                  color: Colors.grey[200],
                  borderRadius: BorderRadius.circular(8),
                ),
                child: const Center(
                  child: Text('QR Code\n(To be generated)', textAlign: TextAlign.center),
                ),
              ),
              const SizedBox(height: 16),
              Row(
                children: [
                  Expanded(
                    child: ElevatedButton.icon(
                      onPressed: () {
                        AppSnackBar.showSuccess(context, 'QR code copied to clipboard');
                        Navigator.pop(ctx);
                      },
                      icon: const Icon(Icons.copy),
                      label: Text(_languageService.translate('copy_qr')),
                    ),
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: ElevatedButton.icon(
                      onPressed: () {
                        AppSnackBar.showSuccess(context, 'Share intent opened');
                        Navigator.pop(ctx);
                      },
                      icon: const Icon(Icons.share),
                      label: Text(_languageService.translate('share')),
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }

  /// Show detailed key information
  void _showKeyDetails(Map<String, dynamic> key) {
    final permissions = (key['permissions'] as List<dynamic>).cast<String>().toList();
    showDialog(
      context: context,
      builder: (ctx) => Dialog(
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: SingleChildScrollView(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisSize: MainAxisSize.min,
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text(
                      key['name'],
                      style: const TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    IconButton(
                      onPressed: () => Navigator.pop(ctx),
                      icon: const Icon(Icons.close),
                    ),
                  ],
                ),
                const SizedBox(height: 16),
                _buildDetailRow('Type', KeyTypeBadge(keyType: key['type'] ?? 'Guest')),
                const SizedBox(height: 12),
                _buildDetailRow('Status', _buildStatusChip(key['status'])),
                const SizedBox(height: 12),
                _buildDetailRow('Created', Text(_formatDate(key['createdAt'] ?? DateTime.now().toIso8601String()))),
                const SizedBox(height: 12),
                _buildDetailRow('Valid Until', Text(_formatDate(key['validUntil'] ?? DateTime.now().toIso8601String()))),
                const SizedBox(height: 16),
                const Text('Permissions:', style: TextStyle(fontWeight: FontWeight.bold)),
                const SizedBox(height: 8),
                PermissionIndicator(permissions: permissions),
              ],
            ),
          ),
        ),
      ),
    );
  }

  /// Helper to build detail rows in dialogs
  Row _buildDetailRow(String label, Widget value) {
    return Row(
      crossAxisAlignment: CrossAxisAlignment.center,
      children: [
        SizedBox(
          width: 80,
          child: Text(
            label,
            style: const TextStyle(
              fontWeight: FontWeight.bold,
              color: AppColors.textSecondary,
            ),
          ),
        ),
        Expanded(child: value),
      ],
    );
  }

  Widget _buildStatusChip(String status) {
    Color color = status == 'Active' ? Colors.green : Colors.orange;
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
      decoration: BoxDecoration(
        color: color.withOpacity(0.1),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: color.withOpacity(0.3)),
      ),
      child: Text(
        status,
        style: TextStyle(
          color: color,
          fontWeight: FontWeight.bold,
          fontSize: 12,
        ),
      ),
    );
  }

  BottomNavigationBar _buildBottomNavigationBar() {
    return BottomNavigationBar(
      currentIndex: _currentIndex,
      onTap: (index) {
        setState(() {
          _currentIndex = index;
        });
      },
      selectedItemColor: const Color(0xFF273671),
      unselectedItemColor: Colors.grey,
      items: [
        BottomNavigationBarItem(
          icon: const Icon(Icons.home_outlined),
          label: _languageService.translate('home'),
        ),
        BottomNavigationBarItem(
          icon: const Icon(Icons.vpn_key),
          label: '', // Remove label as requested
        ),
        BottomNavigationBarItem(
          icon: const Icon(Icons.map_outlined),
          label: _languageService.translate('location'),
        ),
        BottomNavigationBarItem(
          icon: const Icon(Icons.person),
          label: _languageService.translate('profile'),
        ),
        BottomNavigationBarItem(
          icon: const Icon(Icons.science),
          label: 'Test',
        ),
      ],
    );
  }

  void _toggleLock(Map<String, dynamic> car) async {
    try {
      await _carService.toggleCarLock(car['id'], car['isLocked'] ?? false);

      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(
            '${car['name']} has been ${!(car['isLocked'] ?? false) ? 'locked' : 'unlocked'}',
          ),
          backgroundColor: const Color(0xFF273671),
        ),
      );
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Failed to toggle lock: $e'),
          backgroundColor: Colors.red,
        ),
      );
    }
  }

  void _showCarControlDialog(Map<String, dynamic> car) async {
    final result = await showDialog<String>(
      context: context,
      builder: (context) => CarControlDialog(car: car),
    );

    if (result != null) {
      String message = '';
      try {
        switch (result) {
          case 'lock_toggle':
            await _carService.toggleCarLock(
              car['id'],
              car['isLocked'] ?? false,
            );
            message =
                '${car['name']} has been ${!(car['isLocked'] ?? false) ? 'locked' : 'unlocked'}';
            break;
          case 'lights':
            await _carService.triggerLights(car['id']);
            message = '${car['name']} lights have been toggled';
            break;
          case 'start_engine':
            if (car['engineStatus'] == 'On') {
              await _carService.stopEngine(car['id']);
              message = '${car['name']} engine has been stopped';
            } else {
              await _carService.startEngine(car['id']);
              message = '${car['name']} engine has been started';
            }
            break;
          case 'trunk':
            await _carService.openTrunk(car['id']);
            message = '${car['name']} trunk has been opened';
            break;
          case 'find_car':
            await _carService.findCar(car['id']);
            message =
                'Finding your ${car['name']}... Horn will sound and lights will flash';
            break;
        }

        if (message.isNotEmpty) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text(message),
              backgroundColor: const Color(0xFF273671),
            ),
          );
        }
      } catch (e) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Failed to execute action: $e'),
            backgroundColor: Colors.red,
          ),
        );
      }
    }
  }

  void _handleVehicleControlTap(Map<String, dynamic> car) {
    if (car['provisioned'] != true) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text(
            'Vehicle is not provisioned yet. Please provision it first.',
          ),
          backgroundColor: Colors.orange,
        ),
      );
      return;
    }
    _showCarControlDialog(car);
  }

  Future<void> _confirmDeleteVehicle(Map<String, dynamic> car) async {
    final name = car['name']?.toString() ?? 'this vehicle';
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Delete Vehicle'),
        content: Text(
          'Are you sure you want to delete $name? This will remove its digital keys too.',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () => Navigator.pop(context, true),
            style: ElevatedButton.styleFrom(
              backgroundColor: Colors.red,
              foregroundColor: Colors.white,
            ),
            child: const Text('Delete'),
          ),
        ],
      ),
    );

    if (confirmed != true) return;
    try {
      await _carService.deleteCar(car['id']);
      final carId = car['id']?.toString();
      if (carId != null) {
        await _masterCardService.clearPendingPayload(carId);
      }
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Vehicle deleted.'),
          backgroundColor: Color(0xFF273671),
        ),
      );
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Failed to delete vehicle: $e'),
          backgroundColor: Colors.red,
        ),
      );
    }
  }

  Future<void> _confirmDeleteDigitalKey(Map<String, dynamic> key) async {
    final name = key['name']?.toString() ?? 'this key';
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Delete Digital Key'),
        content: Text('Are you sure you want to delete $name?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () => Navigator.pop(context, true),
            style: ElevatedButton.styleFrom(
              backgroundColor: Colors.red,
              foregroundColor: Colors.white,
            ),
            child: const Text('Delete'),
          ),
        ],
      ),
    );

    if (confirmed != true) return;
    try {
      await _carService.deleteDigitalKey(key['id']);
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Digital key deleted.'),
          backgroundColor: Color(0xFF273671),
        ),
      );
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Failed to delete key: $e'),
          backgroundColor: Colors.red,
        ),
      );
    }
  }

  void _showAddDigitalKeyDialog() async {
    if (_cars.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text(
            'Please add a vehicle first before creating digital keys',
          ),
          backgroundColor: Colors.orange,
        ),
      );
      return;
    }

    final result = await showDialog<Map<String, dynamic>>(
      context: context,
      builder: (context) => AddDigitalKeyDialog(cars: _cars),
    );

    if (result != null) {
      try {
        final carId = result['carId']?.toString();
        final selectedCar = _cars.firstWhere(
          (c) => c['id'] == carId,
          orElse: () => {},
        );
        if (selectedCar.isEmpty || selectedCar['provisioned'] != true) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text(
                'Vehicle is not provisioned yet. Please provision it first.',
              ),
              backgroundColor: Colors.orange,
            ),
          );
          return;
        }

        await _carService.addDigitalKey(result);

        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('Digital key created successfully!'),
            backgroundColor: Color(0xFF273671),
          ),
        );
      } catch (e) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('${_languageService.translate('failed_create_key')}: $e'),
            backgroundColor: Colors.red,
          ),
        );
      }
    }
  }

  Future<void> _scanMasterCardForVehicle(String carId, String carName) async {
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (dialogContext) => AlertDialog(
        title: const Text('Scan Master Card'),
        content: const SizedBox(
          height: 140,
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              CircularProgressIndicator(),
              SizedBox(height: 16),
              Text(
                'Hold the master card flat against the back of your phone.',
                textAlign: TextAlign.center,
              ),
              SizedBox(height: 8),
              Text(
                'Keep it still until you feel a vibration.',
                textAlign: TextAlign.center,
              ),
            ],
          ),
        ),
        actions: [
          TextButton(
            onPressed: () async {
              await _masterCardService.cancelReadMasterCard();
              if (Navigator.of(dialogContext).canPop()) {
                Navigator.of(dialogContext).pop();
              }
              if (!mounted) return;
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(
                  content: Text('Master card scan cancelled.'),
                  backgroundColor: Colors.orange,
                ),
              );
            },
            child: const Text('Cancel scan'),
          ),
        ],
      ),
    );

    try {
      final payload = await _masterCardService.readMasterCard(
        timeout: const Duration(seconds: 60),
      );
      await HapticFeedback.mediumImpact();
      if (!mounted) return;
      setState(() {
        _pendingVehicleProvision[carId] = payload;
      });
      await _masterCardService.savePendingPayload(carId, payload);
      Navigator.of(context).pop();
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(
            'Master card scanned. Tap "Provision for this vehicle" to continue.',
          ),
          backgroundColor: const Color(0xFF273671),
        ),
      );
    } catch (e) {
      if (!mounted) return;
      Navigator.of(context).pop();
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Master card scan failed: $e'),
          backgroundColor: Colors.red,
        ),
      );
    }
  }

  Future<void> _startProvisionForVehicle(
    Map<String, dynamic> car,
    MasterCardPayload payload,
  ) async {
    final carId = car['id']?.toString();
    final carName = car['name']?.toString() ?? 'Vehicle';
    final flowStartedAtMs = DateTime.now().millisecondsSinceEpoch;
    final result = await Navigator.of(context).push<bool>(
      MaterialPageRoute(
        builder: (_) =>
            MasterCardFlowScreen(payload: payload, targetName: carName),
      ),
    );

    if (result == true && carId != null) {
      ProvisioningVehicleBinding? binding;
      try {
        binding = await _masterCardService.getProvisioningVehicleBinding();
        if (binding == null) {
          throw StateError('No local vehicle binding found after provisioning');
        }

        final validBinding = _masterCardService.validateBindingConsistency(
          binding,
        );
        if (!validBinding) {
          throw StateError(
            'Provisioning WRITE DATA payload integrity check failed',
          );
        }

        final vehicleIdMatches = listEquals(
          binding.vehicleId,
          payload.vehicleId,
        );
        if (!vehicleIdMatches) {
          final scannedIdHex = _bytesToHex(payload.vehicleId);
          final boundIdHex = _bytesToHex(binding.vehicleId);
          debugPrint(
            '[Provision] Vehicle ID mismatch: scanned=$scannedIdHex bound=$boundIdHex',
          );

          if (!mounted) return;
          final proceed = await showDialog<bool>(
            context: context,
            barrierDismissible: false,
            builder: (dialogContext) {
              return AlertDialog(
                title: const Text('Vehicle ID mismatch detected'),
                content: Column(
                  mainAxisSize: MainAxisSize.min,
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text(
                      'Scanned master-card vehicle ID does not match the ECU binding from WRITE DATA.',
                    ),
                    const SizedBox(height: 12),
                    Text('Scanned ID: $scannedIdHex'),
                    const SizedBox(height: 6),
                    Text('ECU bound ID: $boundIdHex'),
                    const SizedBox(height: 12),
                    const Text(
                      'Continue to provision with ECU binding and record both IDs?',
                    ),
                  ],
                ),
                actions: [
                  TextButton(
                    onPressed: () => Navigator.of(dialogContext).pop(false),
                    child: const Text('Cancel'),
                  ),
                  ElevatedButton(
                    onPressed: () => Navigator.of(dialogContext).pop(true),
                    child: const Text('Continue'),
                  ),
                ],
              );
            },
          );

          if (proceed != true) {
            if (mounted) {
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(
                  content: Text(
                    'Provisioning canceled due to vehicle ID mismatch.',
                  ),
                  backgroundColor: Colors.orange,
                ),
              );
            }
            return;
          }
        }

        final updatedAtMs = binding.updatedAtMs;
        if (updatedAtMs != null) {
          const staleToleranceMs = 5 * 1000;
          if (updatedAtMs < (flowStartedAtMs - staleToleranceMs)) {
            throw StateError(
              'Provisioning binding is stale; please run provisioning again',
            );
          }
        }

        await _carService.registerOwnerProvisioningRecord(
          carDocId: carId,
          vehicleId: binding.vehicleId,
          vehiclePubKey: binding.vehiclePubKey,
          devicePubKey: binding.devicePubKey,
          writeDataPayload: binding.writeDataPayload,
          writeDataUpdatedAtMs: binding.updatedAtMs,
          scannedVehicleId: payload.vehicleId,
        );
      } on FirebaseException catch (e) {
        if (mounted) {
          final msg = e.message ?? 'unknown firebase error';
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text(
                'Provisioned locally, but cloud registration failed [${e.code}]: $msg',
              ),
              backgroundColor: Colors.orange,
            ),
          );
        }
      } catch (e) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text('Provisioning integrity check failed: $e'),
              backgroundColor: Colors.red,
            ),
          );
        }
        return;
      }

      if (binding == null) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text(
                'Provisioning binding missing after successful flow.',
              ),
              backgroundColor: Colors.red,
            ),
          );
        }
        return;
      }

      await _carService.updateCar(carId, {'provisioned': true});
      await _masterCardService.clearProvisioningVehicleBinding();
      await _masterCardService.clearPendingPayload(carId);
      if (!mounted) return;
      setState(() {
        _pendingVehicleProvision.remove(carId);
      });
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Vehicle provisioned successfully.'),
          backgroundColor: Color(0xFF273671),
        ),
      );
    }
  }

  String _formatDate(dynamic dateValue) {
    if (dateValue == null) return 'No expiration';

    DateTime date;
    if (dateValue is Timestamp) {
      date = dateValue.toDate();
    } else if (dateValue is DateTime) {
      date = dateValue;
    } else if (dateValue is String) {
      try {
        date = DateTime.parse(dateValue);
      } catch (e) {
        return dateValue;
      }
    } else {
      return 'Invalid date';
    }

    return '${date.day}/${date.month}/${date.year}';
  }

  String _bytesToHex(List<int> bytes) {
    final sb = StringBuffer();
    for (final b in bytes) {
      sb.write(b.toRadixString(16).padLeft(2, '0').toUpperCase());
    }
    return sb.toString();
  }

  Future<void> _refreshPendingProvision(List<Map<String, dynamic>> cars) async {
    final Map<String, bool> provisionedMap = {};
    for (final car in cars) {
      final carId = car['id']?.toString();
      if (carId == null) continue;
      provisionedMap[carId] = car['provisioned'] == true;
    }

    final Map<String, MasterCardPayload> nextPending = {};
    for (final car in cars) {
      final carId = car['id']?.toString();
      if (carId == null) continue;
      if (provisionedMap[carId] == true) {
        await _masterCardService.clearPendingPayload(carId);
        continue;
      }
      final stored = await _masterCardService.loadPendingPayload(carId);
      if (stored != null) {
        nextPending[carId] = stored;
      }
    }

    if (!mounted) return;
    setState(() {
      _pendingVehicleProvision
        ..clear()
        ..addAll(nextPending);
    });
  }

  void _showAddCarDialog() {
    final nameController = TextEditingController();
    final modelController = TextEditingController();
    final locationController = TextEditingController();
    String selectedColor = 'blue';

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text(
          'Add New Vehicle',
          style: TextStyle(
            fontWeight: FontWeight.bold,
            color: Color(0xFF273671),
          ),
        ),
        content: SingleChildScrollView(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              TextField(
                controller: nameController,
                decoration: const InputDecoration(
                  labelText: 'Vehicle Name',
                  border: OutlineInputBorder(),
                  prefixIcon: Icon(Icons.directions_car),
                ),
              ),
              const SizedBox(height: 16),
              TextField(
                controller: modelController,
                decoration: const InputDecoration(
                  labelText: 'Model/Year',
                  border: OutlineInputBorder(),
                  prefixIcon: Icon(Icons.calendar_today),
                ),
              ),
              const SizedBox(height: 16),
              TextField(
                controller: locationController,
                decoration: const InputDecoration(
                  labelText: 'Location',
                  border: OutlineInputBorder(),
                  prefixIcon: Icon(Icons.location_on),
                ),
              ),
              const SizedBox(height: 16),
              DropdownButtonFormField<String>(
                initialValue: selectedColor,
                decoration: const InputDecoration(
                  labelText: 'Color Theme',
                  border: OutlineInputBorder(),
                  prefixIcon: Icon(Icons.color_lens),
                ),
                items: const [
                  DropdownMenuItem(value: 'blue', child: Text('Blue')),
                  DropdownMenuItem(value: 'purple', child: Text('Purple')),
                  DropdownMenuItem(value: 'orange', child: Text('Orange')),
                  DropdownMenuItem(value: 'green', child: Text('Green')),
                ],
                onChanged: (value) {
                  selectedColor = value!;
                },
              ),
            ],
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () async {
              if (nameController.text.isEmpty ||
                  modelController.text.isEmpty ||
                  locationController.text.isEmpty) {
                ScaffoldMessenger.of(context).showSnackBar(
                  const SnackBar(
                    content: Text('Please fill in all fields'),
                    backgroundColor: Colors.orange,
                  ),
                );
                return;
              }

              try {
                final carData = _carService.createDefaultCarData(
                  name: nameController.text,
                  model: modelController.text,
                  location: locationController.text,
                  color: selectedColor,
                );

                final createdCarId = await _carService.addCar(carData);

                if (!mounted) return;

                Navigator.pop(context);
                ScaffoldMessenger.of(this.context).showSnackBar(
                  const SnackBar(
                    content: Text('Vehicle added successfully!'),
                    backgroundColor: Color(0xFF273671),
                  ),
                );

                // Wait for dialog pop transition before opening the scanner dialog.
                await Future.delayed(const Duration(milliseconds: 150));
                if (!mounted) return;

                await _scanMasterCardForVehicle(
                  createdCarId,
                  nameController.text,
                );
              } catch (e) {
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(
                    content: Text('Failed to add vehicle: $e'),
                    backgroundColor: Colors.red,
                  ),
                );
              }
            },
            style: ElevatedButton.styleFrom(
              backgroundColor: const Color(0xFF273671),
              foregroundColor: Colors.white,
            ),
            child: const Text('Add Vehicle'),
          ),
        ],
      ),
    );
  }
}
