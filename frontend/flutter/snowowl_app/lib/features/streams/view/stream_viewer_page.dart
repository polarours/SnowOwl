import 'dart:async';
import 'package:flutter/material.dart';
import 'package:media_kit/media_kit.dart';
import 'package:media_kit_video/media_kit_video.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:snow_owl/widgets/glass_panel.dart';
import 'package:snow_owl/l10n/l10n.dart';
import 'package:snow_owl/features/streams/application/stream_providers.dart';
import 'package:snow_owl/features/streams/domain/stream_models.dart';
import 'package:snow_owl/features/streams/services/stream_api_service.dart';
class StreamViewerPage extends ConsumerStatefulWidget {
  const StreamViewerPage({super.key, required this.stream});

  final VideoStream stream;

  static const routePath = '/stream-viewer/:streamId';
  static const routeName = 'stream-viewer';

  @override
  ConsumerState<StreamViewerPage> createState() => _StreamViewerPageState();
}

class _StreamViewerPageState extends ConsumerState<StreamViewerPage> {
  Player? _player;
  VideoController? _controller;
  StreamApiService? _apiService; // ignore: unused_field
  bool _isPlayerInitialized = false;
  String? _errorMessage;
  final double _aspectRatio = 16 / 9;

  @override
  void initState() {
    super.initState();
    _apiService = StreamApiService(baseUrl: 'http://localhost:8081');
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _initializePlayer();
    });
  }

  Future<void> _initializePlayer() async {
    try {
      setState(() {
        _isPlayerInitialized = false;
        _errorMessage = null;
      });

      // Dispose of existing player if any
      await _player?.dispose();

      // Create new player
      _player = Player();
      _controller = VideoController(_player!);

      // Open the stream
      await _player!.open(Media(widget.stream.url));

      if (mounted) {
        setState(() {
          _isPlayerInitialized = true;
        });
      }
  } catch (e) {
      // print('Error initializing player: $e\nStack trace: $stackTrace');
      if (mounted) {
        setState(() {
          _errorMessage = 'Player initialization failed: ${e.toString()}';
          _isPlayerInitialized = false;
        });
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Player initialization failed: ${e.toString()}')),
        );
      }
    }
  }

  @override
  void dispose() {
    _player?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final detectionConfig = ref.watch(streamDetectionConfigProvider);

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
              // Header with back button and stream info
              Padding(
                padding: const EdgeInsets.all(24),
                child: Row(
                  children: [
                    IconButton(
                      icon: const Icon(Icons.arrow_back),
                      onPressed: () => Navigator.of(context).pop(),
                    ),
                    const SizedBox(width: 16),
                    Expanded(
                      child: Text(
                        widget.stream.name,
                        style: Theme.of(context).textTheme.headlineSmall,
                        maxLines: 1,
                        overflow: TextOverflow.ellipsis,
                      ),
                    ),
                    IconButton(
                      icon: const Icon(Icons.refresh),
                      onPressed: _initializePlayer,
                    ),
                  ],
                ),
              ),
              // Video player area
              Expanded(
                child: Center(
                  child: _isPlayerInitialized && _controller != null
                      ? AspectRatio(
                          aspectRatio: _aspectRatio,
                          child: Video(controller: _controller!),
                        )
                      : GlassPanel(
                          padding: const EdgeInsets.all(24),
                          child: Column(
                            mainAxisSize: MainAxisSize.min,
                            children: [
                              _errorMessage != null
                                  ? const Icon(Icons.error, size: 48, color: Colors.red)
                                  : const CircularProgressIndicator(),
                              const SizedBox(height: 16),
                              Text(_errorMessage ?? 'Loading video stream...'),
                              const SizedBox(height: 16),
                              FilledButton.icon(
                                onPressed: _initializePlayer,
                                icon: const Icon(Icons.refresh),
                                label: Text(context.retry),
                              ),
                            ],
                          ),
                        ),
                ),
              ),
              // Detection controls
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 24),
                child: GlassPanel(
                  padding: const EdgeInsets.all(16),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Text(
                        context.detectionControls,
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                      const SizedBox(height: 8),
                      Wrap(
                        spacing: 8,
                        runSpacing: 8,
                        children: [
                          FilterChip(
                            label: const Text('Motion'),
                            selected: detectionConfig.motionDetection,
                            onSelected: (selected) {
                              ref
                                  .read(streamDetectionConfigProvider.notifier)
                                  .toggleMotionDetection();
                              // TODO: Send command to backend to enable/disable motion detection
                            },
                          ),
                          FilterChip(
                            label: const Text('Intrusion'),
                            selected: detectionConfig.intrusionDetection,
                            onSelected: (selected) {
                              ref
                                  .read(streamDetectionConfigProvider.notifier)
                                  .toggleIntrusionDetection();
                              // TODO: Send command to backend to enable/disable intrusion detection
                            },
                          ),
                          FilterChip(
                            label: const Text('Fire'),
                            selected: detectionConfig.fireDetection,
                            onSelected: (selected) {
                              ref
                                  .read(streamDetectionConfigProvider.notifier)
                                  .toggleFireDetection();
                              // TODO: Send command to backend to enable/disable fire detection
                            },
                          ),
                          FilterChip(
                            label: const Text('Gas Leak'),
                            selected: detectionConfig.gasLeakDetection,
                            onSelected: (selected) {
                              ref
                                  .read(streamDetectionConfigProvider.notifier)
                                  .toggleGasLeakDetection();
                              // TODO: Send command to backend to enable/disable gas leak detection
                            },
                          ),
                          FilterChip(
                            label: const Text('Equipment'),
                            selected: detectionConfig.equipmentDetection,
                            onSelected: (selected) {
                              ref
                                  .read(streamDetectionConfigProvider.notifier)
                                  .toggleEquipmentDetection();
                              // TODO: Send command to backend to enable/disable equipment detection
                            },
                          ),
                        ],
                      ),
                    ],
                  ),
                ),
              ),
              // Stream information panel
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
                        '${context.streamId}: ${widget.stream.id}',
                        style: Theme.of(context).textTheme.bodyMedium,
                      ),
                      const SizedBox(height: 4),
                      Text(
                        '${context.streamUrl}: ${widget.stream.url}',
                        style: Theme.of(context).textTheme.bodyMedium,
                      ),
                      const SizedBox(height: 4),
                      Text(
                        '${context.protocol}: ${widget.stream.protocol.name.toUpperCase()}',
                        style: Theme.of(context).textTheme.bodyMedium,
                      ),
                      const SizedBox(height: 4),
                      Text(
                        widget.stream.status == StreamStatus.online
                            ? context.deviceOnline
                            : widget.stream.status == StreamStatus.degraded
                                ? context.jitter
                                : context.deviceOffline,
                        style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                              color: widget.stream.status == StreamStatus.online
                                  ? Colors.green
                                  : widget.stream.status == StreamStatus.degraded
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