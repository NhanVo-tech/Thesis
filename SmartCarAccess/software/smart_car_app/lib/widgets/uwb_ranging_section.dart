import 'dart:async';
import 'dart:ui' show FontFeature;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:smart_car_app/service/uwb_multi_service.dart';

/// UWB multi-anchor ranging section. Locked/greyed until [sessionReady] is true
/// (consumes the existing BLE secure-session signal — no parallel auth). Shows
/// live d0/d1/d2 and a status (ACTIVE/STOPPED/ERROR). Auto-stops and resets to
/// the locked state on error or session loss.
class UwbRangingSection extends StatefulWidget {
  const UwbRangingSection({
    super.key,
    required this.sessionReady,
    this.onStart,
    this.onStop,
  });

  final bool sessionReady;

  /// Optional callback fired before phone-side UWB ranging starts. Use it to
  /// notify the ECU (e.g. CCC tunnel 0x84) so the anchors begin ranging too.
  final Future<void> Function()? onStart;

  /// Optional callback fired before phone-side UWB ranging stops. Use it to
  /// notify the ECU (e.g. CCC tunnel 0x85) so the anchors stop ranging too.
  final Future<void> Function()? onStop;

  @override
  State<UwbRangingSection> createState() => _UwbRangingSectionState();
}

class _UwbRangingSectionState extends State<UwbRangingSection> {
  final UwbMultiService _svc = UwbMultiService();
  StreamSubscription<UwbMultiRange>? _rangeSub;
  StreamSubscription<UwbMultiStatus>? _statusSub;
  StreamSubscription<String>? _logSub;

  bool _ranging = false;
  bool _busy = false;
  String _status = 'STOPPED';
  final List<double?> _d = <double?>[null, null, null];
  final List<String> _logs = <String>[];

  @override
  void initState() {
    super.initState();
    _rangeSub = _svc.ranges.listen((UwbMultiRange r) {
      if (!mounted) return;
      setState(() {
        _d[0] = r.valid(0) ? r.d0 : null;
        _d[1] = r.valid(1) ? r.d1 : null;
        _d[2] = r.valid(2) ? r.d2 : null;
      });
    });
    _statusSub = _svc.status.listen((UwbMultiStatus s) {
      if (!mounted) return;
      setState(() => _status = s.status);
      if (s.status == 'ERROR') _autoStop();
    });
    _logSub = _svc.logs.listen((String line) {
      if (!mounted) return;
      setState(() {
        _logs.add(line);
        if (_logs.length > 12) _logs.removeAt(0);
      });
    });
  }

  @override
  void didUpdateWidget(covariant UwbRangingSection old) {
    super.didUpdateWidget(old);
    if (old.sessionReady && !widget.sessionReady && _ranging) {
      _autoStop(); // session lost -> reset to locked
    }
  }

  @override
  void dispose() {
    _rangeSub?.cancel();
    _statusSub?.cancel();
    _logSub?.cancel();
    if (_ranging) _svc.stop();
    _svc.dispose();
    super.dispose();
  }

  Future<void> _start() async {
    setState(() => _busy = true);
    try {
      await widget.onStart?.call();
    } catch (_) {}
    final ok = await _svc.start();
    if (!mounted) return;
    setState(() {
      _busy = false;
      _ranging = ok;
      _status = ok ? 'ACTIVE' : 'ERROR';
    });
  }

  Future<void> _stop() async {
    setState(() => _busy = true);
    try {
      await widget.onStop?.call();
    } catch (_) {}
    await _svc.stop();
    if (!mounted) return;
    setState(() {
      _busy = false;
      _ranging = false;
      _status = 'STOPPED';
      _d[0] = _d[1] = _d[2] = null;
    });
  }

  void _autoStop() {
    _svc.stop();
    if (!mounted) return;
    setState(() {
      _ranging = false;
      _d[0] = _d[1] = _d[2] = null;
    });
  }

  Future<void> _copyLogs() async {
    if (_logs.isEmpty) return;
    final allLogs = _logs.join('\n');
    await Clipboard.setData(ClipboardData(text: allLogs));
    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content: Text('Copied UWB logs to clipboard'),
        duration: Duration(seconds: 2),
      ),
    );
  }

  Color _statusColor() {
    switch (_status) {
      case 'ACTIVE':
        return Colors.green;
      case 'ERROR':
        return Colors.red;
      default:
        return Colors.grey;
    }
  }

  String _fmt(double? d) => d == null ? '---' : '${d.toStringAsFixed(2)} m';

  @override
  Widget build(BuildContext context) {
    final bool locked = !widget.sessionReady;
    return Opacity(
      opacity: locked ? 0.5 : 1.0,
      child: IgnorePointer(
        ignoring: locked,
        child: Card(
          margin: const EdgeInsets.all(12),
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: <Widget>[
                Row(
                  children: <Widget>[
                    Icon(
                      locked ? Icons.lock : Icons.sensors,
                      color: locked
                          ? Colors.grey
                          : Theme.of(context).colorScheme.primary,
                    ),
                    const SizedBox(width: 8),
                    const Text(
                      'UWB Multi-Anchor Ranging',
                      style: TextStyle(
                          fontWeight: FontWeight.bold, fontSize: 16),
                    ),
                    const Spacer(),
                    Container(
                      padding: const EdgeInsets.symmetric(
                          horizontal: 10, vertical: 4),
                      decoration: BoxDecoration(
                        color: _statusColor().withValues(alpha: 0.15),
                        borderRadius: BorderRadius.circular(12),
                      ),
                      child: Text(
                        _status,
                        style: TextStyle(
                            color: _statusColor(),
                            fontWeight: FontWeight.w600),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 12),
                if (locked)
                  const Text(
                    'Complete BLE authentication to unlock UWB ranging.',
                    style: TextStyle(color: Colors.grey),
                  )
                else ...<Widget>[
                  for (int i = 0; i < 3; i++)
                    Padding(
                      padding: const EdgeInsets.symmetric(vertical: 4),
                      child: Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: <Widget>[
                          Text('Anchor $i'),
                          Text(
                            _fmt(_d[i]),
                            style: const TextStyle(
                              fontFeatures: <FontFeature>[
                                FontFeature.tabularFigures(),
                              ],
                            ),
                          ),
                        ],
                      ),
                    ),
                  const SizedBox(height: 12),
                  SizedBox(
                    width: double.infinity,
                    child: ElevatedButton.icon(
                      onPressed:
                          _busy ? null : (_ranging ? _stop : _start),
                      icon: Icon(_ranging ? Icons.stop : Icons.play_arrow),
                      label: Text(_ranging ? 'Stop Ranging' : 'Start Ranging'),
                    ),
                  ),
                  if (_logs.isNotEmpty) ...[
                    const SizedBox(height: 12),
                    const Divider(height: 1),
                    const SizedBox(height: 8),
                    Row(
                      children: [
                        const Text(
                          'UWB Log',
                          style: TextStyle(
                            fontSize: 12,
                            fontWeight: FontWeight.w600,
                          ),
                        ),
                        const SizedBox(width: 8),
                        Text(
                          '${_logs.length} line(s)',
                          style: const TextStyle(
                            fontSize: 11,
                            color: Colors.grey,
                          ),
                        ),
                        const Spacer(),
                        IconButton(
                          icon: const Icon(Icons.copy_outlined, size: 18),
                          tooltip: 'Copy logs',
                          padding: EdgeInsets.zero,
                          constraints: const BoxConstraints(),
                          onPressed: _copyLogs,
                        ),
                      ],
                    ),
                    const SizedBox(height: 4),
                    for (final line in _logs)
                      Padding(
                        padding: const EdgeInsets.only(bottom: 2),
                        child: Text(
                          line,
                          style: const TextStyle(
                            fontFamily: 'monospace',
                            fontSize: 11,
                            color: Colors.black87,
                          ),
                        ),
                      ),
                  ],
                ],
              ],
            ),
          ),
        ),
      ),
    );
  }
}
