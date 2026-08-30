import 'package:flutter/material.dart';
import 'package:geolocator/geolocator.dart';
import 'package:smart_car_app/theme/app_colors.dart';

/// GPS status indicator with color coding
class GpsStatusIndicator extends StatelessWidget {
  final Position? position;
  final bool isConnected;
  final DateTime? lastUpdate;

  const GpsStatusIndicator({
    super.key,
    this.position,
    this.isConnected = false,
    this.lastUpdate,
  });

  Color _getStatusColor() {
    if (!isConnected) return Colors.grey;
    if (position == null) return Colors.orange;
    if (position!.accuracy > 50) return Colors.orange;
    if (position!.accuracy > 20) return Colors.amber;
    return Colors.green;
  }

  String _getStatusText() {
    if (!isConnected) return 'Disconnected';
    if (position == null) return 'Waiting for GPS...';
    if (position!.accuracy > 50) return 'Low Accuracy';
    if (position!.accuracy > 20) return 'Medium Accuracy';
    return 'High Accuracy';
  }

  @override
  Widget build(BuildContext context) {
    final statusColor = _getStatusColor();
    return Row(
      children: [
        Container(
          width: 12,
          height: 12,
          decoration: BoxDecoration(
            color: statusColor,
            shape: BoxShape.circle,
            boxShadow: [
              BoxShadow(
                color: statusColor.withValues(alpha: 0.5),
                blurRadius: 4,
                spreadRadius: 2,
              ),
            ],
          ),
        ),
        const SizedBox(width: 8),
        Text(
          _getStatusText(),
          style: TextStyle(
            fontSize: 12,
            fontWeight: FontWeight.bold,
            color: statusColor,
          ),
        ),
      ],
    );
  }
}

/// Accuracy meter showing GPS accuracy visually
class AccuracyMeter extends StatelessWidget {
  final double? accuracyMeters;

  const AccuracyMeter({super.key, this.accuracyMeters});

  Color _getAccuracyColor() {
    if (accuracyMeters == null) return Colors.grey;
    if (accuracyMeters! <= 10) return Colors.green;
    if (accuracyMeters! <= 25) return Colors.amber;
    return Colors.orange;
  }

  double _getAccuracyPercent() {
    if (accuracyMeters == null) return 0;
    return (1 - (accuracyMeters! / 100)).clamp(0.0, 1.0);
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            const Text('Accuracy', style: TextStyle(fontSize: 12, fontWeight: FontWeight.bold)),
            Text(
              '${accuracyMeters?.toStringAsFixed(1) ?? "—"} m',
              style: TextStyle(fontSize: 12, color: _getAccuracyColor(), fontWeight: FontWeight.bold),
            ),
          ],
        ),
        const SizedBox(height: 6),
        ClipRRect(
          borderRadius: BorderRadius.circular(4),
          child: LinearProgressIndicator(
            value: _getAccuracyPercent(),
            minHeight: 6,
            backgroundColor: Colors.grey[300],
            valueColor: AlwaysStoppedAnimation<Color>(_getAccuracyColor()),
          ),
        ),
      ],
    );
  }
}

/// Location card with satellite count and altitude
class LocationSatelliteCard extends StatelessWidget {
  final Position? position;
  final String address;

  const LocationSatelliteCard({
    super.key,
    this.position,
    this.address = 'Unknown location',
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            AppColors.primary.withValues(alpha: 0.1),
            AppColors.secondary.withValues(alpha: 0.1),
          ],
        ),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: AppColors.primary.withValues(alpha: 0.2)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            address,
            style: const TextStyle(
              fontSize: 14,
              fontWeight: FontWeight.bold,
              color: AppColors.textPrimary,
            ),
            maxLines: 2,
            overflow: TextOverflow.ellipsis,
          ),
          const SizedBox(height: 12),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildStatColumn(
                'Latitude',
                position?.latitude.toStringAsFixed(6) ?? '—',
              ),
              _buildStatColumn(
                'Longitude',
                position?.longitude.toStringAsFixed(6) ?? '—',
              ),
              _buildStatColumn(
                'Altitude',
                '${position?.altitude.toStringAsFixed(1) ?? "—"} m',
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildStatColumn(String label, String value) {
    return Column(
      children: [
        Text(
          label,
          style: TextStyle(fontSize: 10, color: Colors.grey[600]),
        ),
        const SizedBox(height: 4),
        Text(
          value,
          style: const TextStyle(
            fontSize: 12,
            fontWeight: FontWeight.bold,
            color: AppColors.primary,
            fontFamily: 'monospace',
          ),
        ),
      ],
    );
  }
}

/// Speed and heading display card
class SpeedAndHeadingCard extends StatelessWidget {
  final Position? position;

  const SpeedAndHeadingCard({super.key, this.position});

  @override
  Widget build(BuildContext context) {
    final speedMps = position?.speed ?? 0;
    final speedKmh = (speedMps * 3.6).toStringAsFixed(1);
    final heading = (position?.heading ?? 0).toStringAsFixed(0);

    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(10),
        border: Border.all(color: Colors.grey[200]!),
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceAround,
        children: [
          _buildSpeedWidget('Speed', speedKmh, 'km/h'),
          Container(height: 40, width: 1, color: Colors.grey[300]),
          _buildHeadingWidget('Heading', heading, '°'),
        ],
      ),
    );
  }

  Widget _buildSpeedWidget(String label, String value, String unit) {
    return Column(
      children: [
        Text(label, style: TextStyle(fontSize: 11, color: Colors.grey[600])),
        const SizedBox(height: 4),
        Row(
          children: [
            Text(value, style: const TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
            const SizedBox(width: 4),
            Text(unit, style: TextStyle(fontSize: 11, color: Colors.grey[600])),
          ],
        ),
      ],
    );
  }

  Widget _buildHeadingWidget(String label, String value, String unit) {
    return Column(
      children: [
        Text(label, style: TextStyle(fontSize: 11, color: Colors.grey[600])),
        const SizedBox(height: 4),
        Row(
          children: [
            Transform.rotate(
              angle: (double.parse(value) * 3.14159 / 180),
              child: const Icon(Icons.navigation, size: 16, color: AppColors.primary),
            ),
            const SizedBox(width: 6),
            Text(value, style: const TextStyle(fontSize: 14, fontWeight: FontWeight.bold)),
          ],
        ),
      ],
    );
  }
}

/// Location history tile
class LocationHistoryTile extends StatelessWidget {
  final String address;
  final Position position;
  final DateTime timestamp;
  final int index;

  const LocationHistoryTile({
    super.key,
    required this.address,
    required this.position,
    required this.timestamp,
    required this.index,
  });

  String _getTimeAgo() {
    final duration = DateTime.now().difference(timestamp);
    if (duration.inSeconds < 60) return '${duration.inSeconds}s ago';
    if (duration.inMinutes < 60) return '${duration.inMinutes}m ago';
    if (duration.inHours < 24) return '${duration.inHours}h ago';
    return '${duration.inDays}d ago';
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(12),
      margin: const EdgeInsets.only(bottom: 8),
      decoration: BoxDecoration(
        color: Colors.grey[50],
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: Colors.grey[200]!),
      ),
      child: Row(
        children: [
          Container(
            width: 32,
            height: 32,
            decoration: BoxDecoration(
              color: AppColors.primary.withValues(alpha: 0.1),
              borderRadius: BorderRadius.circular(8),
            ),
            child: Center(
              child: Text(
                '$index',
                style: const TextStyle(
                  fontSize: 12,
                  fontWeight: FontWeight.bold,
                  color: AppColors.primary,
                ),
              ),
            ),
          ),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  address,
                  style: const TextStyle(fontSize: 12, fontWeight: FontWeight.bold),
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
                Text(
                  '${position.latitude.toStringAsFixed(4)}, ${position.longitude.toStringAsFixed(4)}',
                  style: TextStyle(fontSize: 10, color: Colors.grey[600], fontFamily: 'monospace'),
                ),
              ],
            ),
          ),
          Text(
            _getTimeAgo(),
            style: TextStyle(fontSize: 10, color: Colors.grey[600]),
          ),
        ],
      ),
    );
  }
}

/// BLE device scanning tile
class BleDeviceScanTile extends StatelessWidget {
  final String deviceName;
  final String deviceAddress;
  final int rssiStrength;
  final bool isConnected;
  final VoidCallback onTap;

  const BleDeviceScanTile({
    super.key,
    required this.deviceName,
    required this.deviceAddress,
    required this.rssiStrength,
    required this.isConnected,
    required this.onTap,
  });

  int _getSignalBars() {
    if (rssiStrength > -50) return 4;
    if (rssiStrength > -70) return 3;
    if (rssiStrength > -85) return 2;
    return 1;
  }

  Color _getSignalColor() {
    if (rssiStrength > -70) return Colors.green;
    if (rssiStrength > -85) return Colors.orange;
    return Colors.red;
  }

  @override
  Widget build(BuildContext context) {
    final signalBars = _getSignalBars();
    final signalColor = _getSignalColor();

    return Container(
      padding: const EdgeInsets.all(12),
      margin: const EdgeInsets.only(bottom: 8),
      decoration: BoxDecoration(
        color: isConnected ? AppColors.primary.withValues(alpha: 0.05) : Colors.white,
        borderRadius: BorderRadius.circular(8),
        border: Border.all(
          color: isConnected ? AppColors.primary : Colors.grey[200]!,
        ),
      ),
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          onTap: onTap,
          borderRadius: BorderRadius.circular(8),
          child: Row(
            children: [
              Column(
                children: List.generate(4, (i) {
                  return Container(
                    width: 2,
                    height: 4 + (i * 3).toDouble(),
                    margin: const EdgeInsets.symmetric(horizontal: 1),
                    decoration: BoxDecoration(
                      color: i < signalBars ? signalColor : Colors.grey[300],
                      borderRadius: BorderRadius.circular(1),
                    ),
                  );
                }),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      deviceName,
                      style: TextStyle(
                        fontSize: 12,
                        fontWeight: FontWeight.bold,
                        color: isConnected ? AppColors.primary : AppColors.textPrimary,
                      ),
                    ),
                    Text(
                      deviceAddress,
                      style: TextStyle(
                        fontSize: 10,
                        color: Colors.grey[600],
                        fontFamily: 'monospace',
                      ),
                    ),
                  ],
                ),
              ),
              Column(
                children: [
                  if (isConnected)
                    Container(
                      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                      decoration: BoxDecoration(
                        color: Colors.green.withValues(alpha: 0.1),
                        borderRadius: BorderRadius.circular(4),
                      ),
                      child: const Text(
                        'Connected',
                        style: TextStyle(fontSize: 10, color: Colors.green, fontWeight: FontWeight.bold),
                      ),
                    ),
                  if (!isConnected) Text('${rssiStrength} dBm', style: TextStyle(fontSize: 10, color: Colors.grey[600])),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }
}
