import 'package:flutter/foundation.dart';
import 'package:shared_preferences/shared_preferences.dart';

class PkeRolloutFlags {
  const PkeRolloutFlags({
    required this.backgroundMode,
    required this.fastTransaction,
    required this.bondingEnforce,
    required this.rssiMonitorOnly,
    required this.rssiThresholdDbm,
  });

  final bool backgroundMode;
  final bool fastTransaction;
  final bool bondingEnforce;
  final bool rssiMonitorOnly;
  final int rssiThresholdDbm;

  PkeRolloutFlags copyWith({
    bool? backgroundMode,
    bool? fastTransaction,
    bool? bondingEnforce,
    bool? rssiMonitorOnly,
    int? rssiThresholdDbm,
  }) {
    return PkeRolloutFlags(
      backgroundMode: backgroundMode ?? this.backgroundMode,
      fastTransaction: fastTransaction ?? this.fastTransaction,
      bondingEnforce: bondingEnforce ?? this.bondingEnforce,
      rssiMonitorOnly: rssiMonitorOnly ?? this.rssiMonitorOnly,
      rssiThresholdDbm: rssiThresholdDbm ?? this.rssiThresholdDbm,
    );
  }
}

class PkeRolloutFlagsService {
  static const String _kBackgroundMode = 'pke_rollout_background_mode';
  static const String _kFastTransaction = 'pke_rollout_fast_transaction';
  static const String _kBondingEnforce = 'pke_rollout_bonding_enforce';
  static const String _kRssiMonitorOnly = 'pke_rollout_rssi_monitor_only';
  static const String _kRssiThresholdDbm = 'pke_rollout_rssi_threshold_dbm';

  static const PkeRolloutFlags _defaults = PkeRolloutFlags(
    backgroundMode: false,
    fastTransaction: false,
    bondingEnforce: false,
    rssiMonitorOnly: true,
    rssiThresholdDbm: -70,
  );

  Future<PkeRolloutFlags> ensureDefaults() async {
    final prefs = await SharedPreferences.getInstance();

    if (!prefs.containsKey(_kBackgroundMode)) {
      await prefs.setBool(_kBackgroundMode, _defaults.backgroundMode);
    }
    if (!prefs.containsKey(_kFastTransaction)) {
      await prefs.setBool(_kFastTransaction, _defaults.fastTransaction);
    }
    if (!prefs.containsKey(_kBondingEnforce)) {
      await prefs.setBool(_kBondingEnforce, _defaults.bondingEnforce);
    }
    if (!prefs.containsKey(_kRssiMonitorOnly)) {
      await prefs.setBool(_kRssiMonitorOnly, _defaults.rssiMonitorOnly);
    }
    if (!prefs.containsKey(_kRssiThresholdDbm)) {
      await prefs.setInt(_kRssiThresholdDbm, _defaults.rssiThresholdDbm);
    }

    final flags = await getFlags();
    debugPrint(
      '[PKE][ROLL] app flags background=${flags.backgroundMode ? 1 : 0} '
      'fast_tx=${flags.fastTransaction ? 1 : 0} '
      'bonding_enforce=${flags.bondingEnforce ? 1 : 0} '
      'rssi_monitor_only=${flags.rssiMonitorOnly ? 1 : 0} '
      'rssi_threshold_dbm=${flags.rssiThresholdDbm}',
    );
    return flags;
  }

  Future<PkeRolloutFlags> getFlags() async {
    final prefs = await SharedPreferences.getInstance();
    return PkeRolloutFlags(
      backgroundMode: prefs.getBool(_kBackgroundMode) ?? _defaults.backgroundMode,
      fastTransaction: prefs.getBool(_kFastTransaction) ?? _defaults.fastTransaction,
      bondingEnforce: prefs.getBool(_kBondingEnforce) ?? _defaults.bondingEnforce,
      rssiMonitorOnly: prefs.getBool(_kRssiMonitorOnly) ?? _defaults.rssiMonitorOnly,
      rssiThresholdDbm: prefs.getInt(_kRssiThresholdDbm) ?? _defaults.rssiThresholdDbm,
    );
  }

  Future<void> saveFlags(PkeRolloutFlags flags) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool(_kBackgroundMode, flags.backgroundMode);
    await prefs.setBool(_kFastTransaction, flags.fastTransaction);
    await prefs.setBool(_kBondingEnforce, flags.bondingEnforce);
    await prefs.setBool(_kRssiMonitorOnly, flags.rssiMonitorOnly);
    await prefs.setInt(_kRssiThresholdDbm, flags.rssiThresholdDbm);
  }
}
