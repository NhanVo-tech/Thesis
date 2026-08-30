import 'package:flutter/material.dart';
import 'package:smart_car_app/theme/app_colors.dart';
import 'package:smart_car_app/widgets/app_components.dart';

class CarControlDialog extends StatefulWidget {
  final Map<String, dynamic> car;
  
  const CarControlDialog({super.key, required this.car});

  @override
  State<CarControlDialog> createState() => _CarControlDialogState();
}

class _CarControlDialogState extends State<CarControlDialog> {
  @override
  Widget build(BuildContext context) {
    return Dialog(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
      child: Container(
        padding: const EdgeInsets.all(24),
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(20),
          gradient: AppColors.primaryGradient,
        ),
        child: SingleChildScrollView(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              // Header with vehicle info
              Row(
                children: [
                  Container(
                    width: 50,
                    height: 50,
                    decoration: BoxDecoration(
                      color: Colors.white.withValues(alpha: 0.2),
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: const Icon(
                      Icons.directions_car,
                      color: Colors.white,
                    ),
                  ),
                  const SizedBox(width: 16),
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          widget.car['name'],
                          style: const TextStyle(
                            color: Colors.white,
                            fontSize: 20,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                        Text(
                          widget.car['model'],
                          style: TextStyle(
                            color: Colors.white.withValues(alpha: 0.8),
                            fontSize: 14,
                          ),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 16),
              
              // Vehicle status display
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.white.withValues(alpha: 0.1),
                  borderRadius: BorderRadius.circular(12),
                  border: Border.all(
                    color: Colors.white.withValues(alpha: 0.2),
                  ),
                ),
                child: Row(
                  children: [
                    Expanded(
                      child: _buildStatusItem(
                        'Battery',
                        '${widget.car['batteryLevel'] ?? 85}%',
                        Icons.battery_std,
                      ),
                    ),
                    const VerticalDivider(color: Colors.white24),
                    Expanded(
                      child: _buildStatusItem(
                        'Status',
                        widget.car['isLocked'] ?? true ? 'Locked' : 'Unlocked',
                        widget.car['isLocked'] ?? true ? Icons.lock : Icons.lock_open,
                      ),
                    ),
                    const VerticalDivider(color: Colors.white24),
                    Expanded(
                      child: _buildStatusItem(
                        'Connection',
                        'Online',
                        Icons.circle,
                      ),
                    ),
                  ],
                ),
              ),
              const SizedBox(height: 24),
              
              // Control buttons
              Row(
                children: [
                  Expanded(
                    child: _buildControlButton(
                      icon: widget.car['isLocked'] ? Icons.lock_open : Icons.lock,
                      label: widget.car['isLocked'] ? 'Unlock' : 'Lock',
                      onPressed: () => _handleControl('lock_toggle'),
                    ),
                  ),
                  const SizedBox(width: 12),
                  Expanded(
                    child: _buildControlButton(
                      icon: Icons.flash_on,
                      label: 'Lights',
                      onPressed: () => _handleControl('lights'),
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 12),
              Row(
                children: [
                  Expanded(
                    child: _buildControlButton(
                      icon: Icons.directions_car,
                      label: 'Start Engine',
                      onPressed: () => _showConfirmation(
                        'Start Engine',
                        'Are you sure you want to start the engine?',
                        () => _handleControl('start_engine'),
                      ),
                      isWarning: true,
                    ),
                  ),
                  const SizedBox(width: 12),
                  Expanded(
                    child: _buildControlButton(
                      icon: Icons.luggage,
                      label: 'Trunk',
                      onPressed: () => _showConfirmation(
                        'Open Trunk',
                        'Do you want to open the trunk?',
                        () => _handleControl('trunk'),
                      ),
                      isWarning: true,
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 24),
              
              // Action buttons
              Row(
                children: [
                  Expanded(
                    child: TextButton(
                      onPressed: () => Navigator.pop(context),
                      child: const Text(
                        'Cancel',
                        style: TextStyle(color: Colors.white),
                      ),
                    ),
                  ),
                  Expanded(
                    child: ElevatedButton(
                      onPressed: () => Navigator.pop(context, 'find_car'),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Colors.white,
                        foregroundColor: AppColors.primary,
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(12),
                        ),
                      ),
                      child: const Text('Find My Car'),
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

  /// Handle control actions with feedback
  void _handleControl(String action) {
    // Simulate action with brief delay for UX
    Future.delayed(const Duration(milliseconds: 500), () {
      if (!mounted) return;
      // Show success feedback
      AppSnackBar.showSuccess(context, '$action completed successfully');
      Navigator.pop(context, action);
    });
  }

  /// Show confirmation dialog before critical actions
  void _showConfirmation(String title, String message, VoidCallback onConfirm) {
    showDialog(
      context: context,
      builder: (BuildContext ctx) => Dialog(
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(Icons.warning_rounded, color: Colors.orange[600], size: 48),
              const SizedBox(height: 16),
              Text(
                title,
                style: const TextStyle(
                  fontSize: 20,
                  fontWeight: FontWeight.bold,
                ),
              ),
              const SizedBox(height: 12),
              Text(
                message,
                textAlign: TextAlign.center,
                style: TextStyle(
                  color: Colors.grey[700],
                  fontSize: 14,
                ),
              ),
              const SizedBox(height: 24),
              Row(
                children: [
                  Expanded(
                    child: TextButton(
                      onPressed: () => Navigator.pop(ctx),
                      child: const Text('Cancel'),
                    ),
                  ),
                  const SizedBox(width: 12),
                  Expanded(
                    child: ElevatedButton(
                      onPressed: () {
                        Navigator.pop(ctx);
                        onConfirm();
                      },
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Colors.orange[600],
                      ),
                      child: const Text(
                        'Confirm',
                        style: TextStyle(color: Colors.white),
                      ),
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

  /// Build status item
  Widget _buildStatusItem(String label, String value, IconData icon) {
    return Column(
      children: [
        Icon(icon, color: Colors.white, size: 16),
        const SizedBox(height: 4),
        Text(
          value,
          style: const TextStyle(
            color: Colors.white,
            fontSize: 13,
            fontWeight: FontWeight.bold,
          ),
        ),
        const SizedBox(height: 2),
        Text(
          label,
          style: TextStyle(
            color: Colors.white.withValues(alpha: 0.7),
            fontSize: 11,
          ),
        ),
      ],
    );
  }

  Widget _buildControlButton({
    required IconData icon,
    required String label,
    required VoidCallback onPressed,
    bool isWarning = false,
  }) {
    return ElevatedButton(
      onPressed: onPressed,
      style: ElevatedButton.styleFrom(
        backgroundColor: isWarning 
            ? Colors.orange.withValues(alpha: 0.2)
            : Colors.white.withValues(alpha: 0.2),
        foregroundColor: isWarning ? Colors.orange[300] : Colors.white,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(12),
          side: BorderSide(
            color: isWarning 
                ? Colors.orange.withValues(alpha: 0.3)
                : Colors.transparent,
          ),
        ),
        padding: const EdgeInsets.symmetric(vertical: 16),
      ),
      child: Column(
        children: [
          Icon(icon, size: 24),
          const SizedBox(height: 4),
          Text(label, style: const TextStyle(fontSize: 12)),
        ],
      ),
    );
  }
}

class AddDigitalKeyDialog extends StatefulWidget {
  final List<Map<String, dynamic>> cars;
  
  const AddDigitalKeyDialog({super.key, required this.cars});

  @override
  State<AddDigitalKeyDialog> createState() => _AddDigitalKeyDialogState();
}

class _AddDigitalKeyDialogState extends State<AddDigitalKeyDialog> {
  final _formKey = GlobalKey<FormState>();
  final _nameController = TextEditingController();
  String? _selectedCarId;
  String _keyType = 'Shared';
  final List<String> _selectedPermissions = ['unlock', 'lock'];
  DateTime _validUntil = DateTime.now().add(const Duration(days: 30));

  final List<Map<String, String>> _availablePermissions = [
    {'id': 'unlock', 'name': 'Unlock'},
    {'id': 'lock', 'name': 'Lock'},
    {'id': 'start_engine', 'name': 'Start Engine'},
    {'id': 'trunk', 'name': 'Trunk Access'},
    {'id': 'lights', 'name': 'Lights Control'},
    {'id': 'air_conditioning', 'name': 'Air Conditioning'},
  ];

  @override
  Widget build(BuildContext context) {
    return Dialog(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      insetPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 24),
      child: ConstrainedBox(
        constraints: BoxConstraints(
          maxHeight: MediaQuery.of(context).size.height * 0.85,
        ),
        child: Container(
          padding: const EdgeInsets.all(24),
          child: Form(
            key: _formKey,
            child: SingleChildScrollView(
              child: Column(
                mainAxisSize: MainAxisSize.min,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
              const Text(
                'Create Digital Key',
                style: TextStyle(
                  fontSize: 20,
                  fontWeight: FontWeight.bold,
                  color: AppColors.primary,
                ),
              ),
              const SizedBox(height: 20),
              TextFormField(
                controller: _nameController,
                decoration: const InputDecoration(
                  labelText: 'Key Name',
                  border: OutlineInputBorder(),
                  prefixIcon: Icon(Icons.vpn_key),
                ),
                validator: (value) {
                  if (value == null || value.isEmpty) {
                    return 'Please enter a key name';
                  }
                  return null;
                },
              ),
              const SizedBox(height: 16),
              DropdownButtonFormField<String>(
                initialValue: _selectedCarId,
                decoration: const InputDecoration(
                  labelText: 'Select Vehicle',
                  border: OutlineInputBorder(),
                  prefixIcon: Icon(Icons.directions_car),
                ),
                items: widget.cars.map((car) {
                  return DropdownMenuItem<String>(
                    value: car['id'],
                    child: Text(car['name']),
                  );
                }).toList(),
                onChanged: (value) {
                  setState(() {
                    _selectedCarId = value;
                  });
                },
                validator: (value) {
                  if (value == null) {
                    return 'Please select a vehicle';
                  }
                  return null;
                },
              ),
              const SizedBox(height: 16),
              DropdownButtonFormField<String>(
                initialValue: _keyType,
                decoration: const InputDecoration(
                  labelText: 'Key Type',
                  border: OutlineInputBorder(),
                  prefixIcon: Icon(Icons.category),
                ),
                items: const [
                  DropdownMenuItem(value: 'Owner', child: Text('Owner')),
                  DropdownMenuItem(value: 'Shared', child: Text('Shared')),
                  DropdownMenuItem(value: 'Temporary', child: Text('Temporary')),
                ],
                onChanged: (value) {
                  setState(() {
                    _keyType = value!;
                  });
                },
              ),
              const SizedBox(height: 16),
              const Text(
                'Permissions',
                style: TextStyle(
                  fontSize: 16,
                  fontWeight: FontWeight.bold,
                  color: AppColors.primary,
                ),
              ),
              const SizedBox(height: 8),
              Wrap(
                spacing: 8,
                children: _availablePermissions.map((permission) {
                  return FilterChip(
                    label: Text(permission['name']!),
                    selected: _selectedPermissions.contains(permission['id']),
                    onSelected: (selected) {
                      setState(() {
                        if (selected) {
                          _selectedPermissions.add(permission['id']!);
                        } else {
                          _selectedPermissions.remove(permission['id']);
                        }
                      });
                    },
                    selectedColor: AppColors.secondary.withOpacity(0.3),
                  );
                }).toList(),
              ),
              const SizedBox(height: 16),
              Row(
                children: [
                  const Icon(Icons.calendar_today, color: AppColors.textPrimary),
                  const SizedBox(width: 8),
                  Text(
                    'Valid until: ${_validUntil.day}/${_validUntil.month}/${_validUntil.year}',
                    style: const TextStyle(fontSize: 16),
                  ),
                  const Spacer(),
                  TextButton(
                    onPressed: _selectDate,
                    child: const Text('Change'),
                  ),
                ],
              ),
              const SizedBox(height: 24),
              Row(
                children: [
                  Expanded(
                    child: TextButton(
                      onPressed: () => Navigator.pop(context),
                      child: const Text('Cancel'),
                    ),
                  ),
                  const SizedBox(width: 12),
                  Expanded(
                    child: ElevatedButton(
                      onPressed: _createKey,
                      style: ElevatedButton.styleFrom(
                        backgroundColor: AppColors.primary,
                        foregroundColor: Colors.white,
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(12),
                        ),
                      ),
                      child: const Text('Create Key'),
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    ),
  ),
);
  }

  void _selectDate() async {
    final DateTime? picked = await showDatePicker(
      context: context,
      initialDate: _validUntil,
      firstDate: DateTime.now(),
      lastDate: DateTime.now().add(const Duration(days: 365)),
    );
    if (picked != null && picked != _validUntil) {
      setState(() {
        _validUntil = picked;
      });
    }
  }

  void _createKey() {
    if (_formKey.currentState!.validate() && _selectedPermissions.isNotEmpty) {
      final newKey = {
        'carId': _selectedCarId,
        'name': _nameController.text,
        'type': _keyType,
        'status': 'Active',
        'permissions': _selectedPermissions,
        'validUntil': _validUntil,
      };
      
      Navigator.pop(context, newKey);
    } else if (_selectedPermissions.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Please select at least one permission')),
      );
    }
  }
}