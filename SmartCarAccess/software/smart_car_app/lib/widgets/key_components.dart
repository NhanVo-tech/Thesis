import 'package:flutter/material.dart';
import 'package:smart_car_app/theme/app_colors.dart';

/// Displays different key types with visual badges
class KeyTypeBadge extends StatelessWidget {
  final String keyType; // 'Owner', 'Guest', 'Temporary'
  
 const KeyTypeBadge({super.key, required this.keyType});

  Color _getTypColor() {
    switch (keyType.toLowerCase()) {
      case 'owner':
        return Colors.amber[600]!;
      case 'guest':
        return AppColors.primary;
      case 'temporary':
        return AppColors.warning;
      default:
        return Colors.grey[500]!;
    }
  }

  IconData _getTypeIcon() {
    switch (keyType.toLowerCase()) {
      case 'owner':
        return Icons.workspace_premium;
      case 'guest':
        return Icons.person;
      case 'temporary':
        return Icons.schedule;
      default:
        return Icons.vpn_key;
    }
  }

  @override
  Widget build(BuildContext context) {
    final color = _getTypColor();
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.1),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(
          color: color.withValues(alpha: 0.3),
        ),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(_getTypeIcon(), color: color, size: 14),
          const SizedBox(width: 4),
          Text(
            keyType,
            style: TextStyle(
              color: color,
              fontWeight: FontWeight.bold,
              fontSize: 12,
            ),
          ),
        ],
      ),
    );
  }
}

/// Displays key expiration status with visual indicators
class ExpirationBadge extends StatelessWidget {
  final DateTime expirationDate;
  
  const ExpirationBadge({super.key, required this.expirationDate});

  Color _getStatusColor() {
    final daysLeft = expirationDate.difference(DateTime.now()).inDays;
    if (daysLeft <= 0) return AppColors.error;
    if (daysLeft <= 7) return AppColors.warning;
    return AppColors.success;
  }

  String _getStatusText() {
    final daysLeft = expirationDate.difference(DateTime.now()).inDays;
    if (daysLeft <= 0) return 'Expired';
    if (daysLeft == 1) return '1 day left';
    if (daysLeft <= 7) return '$daysLeft days left';
    return 'Valid';
  }

  @override
  Widget build(BuildContext context) {
    final color = _getStatusColor();
    final daysLeft = expirationDate.difference(DateTime.now()).inDays;
    
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.1),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: color.withValues(alpha: 0.3),
        ),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(
            daysLeft <= 0 ? Icons.error_outline : Icons.schedule,
            color: color,
            size: 14,
          ),
          const SizedBox(width: 4),
          Text(
            _getStatusText(),
            style: TextStyle(
              color: color,
              fontWeight: FontWeight.bold,
              fontSize: 11,
            ),
          ),
        ],
      ),
    );
  }
}

/// Quick share button with QR code option
class KeyShareButton extends StatelessWidget {
  final String keyId;
  final VoidCallback onShare;
  
  const KeyShareButton({
    super.key,
    required this.keyId,
    required this.onShare,
  });

  @override
  Widget build(BuildContext context) {
    return ElevatedButton.icon(
      onPressed: onShare,
      icon: const Icon(Icons.share, size: 16),
      label: const Text('Share', style: TextStyle(fontSize: 12)),
      style: ElevatedButton.styleFrom(
        backgroundColor: AppColors.secondary,
        foregroundColor: Colors.white,
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(8),
        ),
      ),
    );
  }
}

/// Permission level indicator
class PermissionIndicator extends StatelessWidget {
  final List<String> permissions;
  final bool compact;
  
  const PermissionIndicator({
    super.key,
    required this.permissions,
    this.compact = false,
  });

  @override
  Widget build(BuildContext context) {
    if (compact) {
      return Container(
        padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
        decoration: BoxDecoration(
          color: AppColors.primary.withValues(alpha: 0.1),
          borderRadius: BorderRadius.circular(12),
        ),
        child: Text(
          '${permissions.length} permissions',
          style: TextStyle(
            fontSize: 11,
            color: AppColors.primary,
            fontWeight: FontWeight.bold,
          ),
        ),
      );
    }

    return Wrap(
      spacing: 6,
      runSpacing: 6,
      children: permissions.map((permission) {
        return Container(
          padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
          decoration: BoxDecoration(
            color: AppColors.secondary.withValues(alpha: 0.1),
            borderRadius: BorderRadius.circular(8),
            border: Border.all(
              color: AppColors.secondary.withValues(alpha: 0.3),
            ),
          ),
          child: Text(
            permission.replaceAll('_', ' '),
            style: TextStyle(
              fontSize: 10,
              color: AppColors.secondary,
              fontWeight: FontWeight.w600,
            ),
          ),
        );
      }).toList(),
    );
  }
}
