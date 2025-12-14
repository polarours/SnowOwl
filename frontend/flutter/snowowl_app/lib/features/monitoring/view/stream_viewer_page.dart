import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/widgets/glass_panel.dart';
import 'package:snow_owl/l10n/l10n.dart';
import 'package:snow_owl/features/monitoring/domain/monitoring_models.dart';
import 'package:snow_owl/features/monitoring/services/detection_service.dart';
import 'package:snow_owl/features/monitoring/widgets/detection_overlay.dart';
import 'package:snow_owl/features/shared/video_player_widget.dart';

class StreamViewerPage extends ConsumerStatefulWidget {
  const StreamViewerPage({super.key, required this.stream});

  final MonitoringStream stream;

  static const routePath = '/stream-viewer/:streamId';
  static const routeName = 'stream-viewer';

  @override
  ConsumerState<StreamViewerPage> createState() => _StreamViewerPageState();
}

class _StreamViewerPageState extends ConsumerState<StreamViewerPage> {
  Timer? _refreshTimer;
  final double _aspectRatio = 16 / 9;
  
  bool _motionDetectionEnabled = false;
  bool _intrusionDetectionEnabled = false;
  bool _fireDetectionEnabled = false;
  bool _gasLeakDetectionEnabled = false;
  bool _equipmentDetectionEnabled = false;
  
  List<DetectionResult> _detectionResults = []; // ignore: unused_field
  String? _playerErrorMessage; // ignore: unused_field
  
  late DetectionService _detectionService;

  @override
  void initState() {
    super.initState();
    _detectionService = DetectionService(baseUrl: 'http://localhost:8081');
    
    _refreshTimer = Timer.periodic(const Duration(seconds: 30), (_) {
      _refreshStreamInfo();
    });
    
    _startMockDetectionUpdates();
  }

  Future<void> _refreshStreamInfo() async {
    }

  Future<void> _toggleDetection(String detectionType, bool enabled) async {
    final success = await _detectionService.setDetectionEnabled(detectionType, enabled);
    if (!success) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Failed to update $detectionType detection')),
        );
      }
    }
  }
  
  void _startMockDetectionUpdates() {

    Timer.periodic(const Duration(seconds: 2), (timer) {
      if (!mounted) {
        timer.cancel();
        return;
      }
      
      final mockResults = <DetectionResult>[];
      
      if (_motionDetectionEnabled) {
        mockResults.add(DetectionResult(
          type: 'Motion',
          boundingBox: const Rect.fromLTWH(100, 100, 150, 200),
          confidence: 0.85,
        ),);
      }
      
      if (_intrusionDetectionEnabled) {
        mockResults.add(DetectionResult(
          type: 'Intrusion',
          boundingBox: const Rect.fromLTWH(300, 150, 80, 180),
          confidence: 0.92,
        ),);
      }
      
      if (_fireDetectionEnabled) {
        mockResults.add(DetectionResult(
          type: 'Fire',
          boundingBox: const Rect.fromLTWH(200, 50, 100, 100),
          confidence: 0.78,
        ),);
      }
      
      setState(() {
        _detectionResults = mockResults;
      });
    });
  }

  @override
  void dispose() {
    _refreshTimer?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBody: true,
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            colors: [
              Theme.of(context).colorScheme.surface,
              Theme.of(context).colorScheme.surface,
            ],
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
          ),
        ),
        child: SafeArea(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Padding(
                padding: const EdgeInsets.all(24),
                child: Row(
                  children: [
                    IconButton(
                      icon: const Icon(Icons.arrow_back),
                      onPressed: () => Navigator.of(context).pop(),
                    ),
                    const SizedBox(width: 16),
                    Text(
                      widget.stream.name,
                      style: Theme.of(context).textTheme.headlineSmall,
                    ),
                    const Spacer(),
                    IconButton(
                      icon: const Icon(Icons.refresh),
                      onPressed: () {
                        setState(() {
                          _playerErrorMessage = null;
                        });
                      },
                    ),
                  ],
                ),
              ),
              Expanded(
                child: Center(
                  child: SimpleVideoPlayer(
                    streamUrl: widget.stream.url,
                    aspectRatio: _aspectRatio,
                    onError: (error) {
                      setState(() {
                        _playerErrorMessage = error;
                      });
                    },
                  ),
                ),
              ),
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 24),
                child: GlassPanel(
                  padding: const EdgeInsets.all(16),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Text(
                        'Detection Controls',
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                      const SizedBox(height: 8),
                      Wrap(
                        spacing: 8,
                        runSpacing: 8,
                        children: [
                          FilterChip(
                            label: const Text('Motion'),
                            selected: _motionDetectionEnabled,
                            onSelected: (selected) async {
                              setState(() {
                                _motionDetectionEnabled = selected;
                              });
                              await _toggleDetection('motion', selected);
                            },
                          ),
                          FilterChip(
                            label: const Text('Intrusion'),
                            selected: _intrusionDetectionEnabled,
                            onSelected: (selected) async {
                              setState(() {
                                _intrusionDetectionEnabled = selected;
                              });
                              await _toggleDetection('intrusion', selected);
                            },
                          ),
                          FilterChip(
                            label: const Text('Fire'),
                            selected: _fireDetectionEnabled,
                            onSelected: (selected) async {
                              setState(() {
                                _fireDetectionEnabled = selected;
                              });
                              await _toggleDetection('fire', selected);
                            },
                          ),
                          FilterChip(
                            label: const Text('Gas Leak'),
                            selected: _gasLeakDetectionEnabled,
                            onSelected: (selected) async {
                              setState(() {
                                _gasLeakDetectionEnabled = selected;
                              });
                              await _toggleDetection('gas_leak', selected);
                            },
                          ),
                          FilterChip(
                            label: const Text('Equipment'),
                            selected: _equipmentDetectionEnabled,
                            onSelected: (selected) async {
                              setState(() {
                                _equipmentDetectionEnabled = selected;
                              });
                              await _toggleDetection('equipment', selected);
                            },
                          ),
                        ],
                      ),
                    ],
                  ),
                ),
              ),
              Padding(
                padding: const EdgeInsets.all(24),
                child: GlassPanel(
                  padding: const EdgeInsets.all(16),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Text(
                        widget.stream.name,
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                      const SizedBox(height: 8),
                      Text(
                        '${context.deviceID}: ${widget.stream.id}',
                        style: Theme.of(context).textTheme.bodyMedium,
                      ),
                      const SizedBox(height: 4),
                      Text(
                        '${context.deviceType}: ${widget.stream.protocol}',
                        style: Theme.of(context).textTheme.bodyMedium,
                      ),
                      const SizedBox(height: 4),
                      Text(
                        widget.stream.status == MonitoringStreamStatus.online
                            ? context.deviceOnline
                            : widget.stream.status == MonitoringStreamStatus.degraded
                                ? context.jitter
                                : context.deviceOffline,
                        style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                              color: widget.stream.status == MonitoringStreamStatus.online
                                  ? Colors.green
                                  : widget.stream.status == MonitoringStreamStatus.degraded
                                      ? Colors.orange
                                      : Colors.red,
                            ),
                      ),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}