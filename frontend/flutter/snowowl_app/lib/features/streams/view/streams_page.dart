import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/l10n/l10n.dart';
import 'package:snow_owl/widgets/glass_panel.dart';
import 'package:snow_owl/features/streams/application/stream_providers.dart';
import 'package:snow_owl/features/streams/domain/stream_models.dart';
import 'package:snow_owl/features/streams/view/stream_viewer_page.dart';
import 'package:snow_owl/features/devices/application/device_providers.dart';

class StreamsPage extends ConsumerWidget {
  const StreamsPage({super.key});

  static const routePath = '/streams';
  static const routeName = 'streams';
  static const navigationIcon = Icons.video_library_outlined;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final streamsAsync = ref.watch(streamsProvider);

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
          bottom: false,
          child: Padding(
            padding: const EdgeInsets.all(24.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    Text(
                      context.videoStreams,
                      style: Theme.of(context).textTheme.headlineMedium,
                    ),
                    const Spacer(),
                    // Connection status indicator
                    _ConnectionStatusIndicator(),
                  ],
                ),
                const SizedBox(height: 10),
                Text(
                  'Browse and manage all video streams from connected devices',
                  style: Theme.of(context).textTheme.bodyLarge?.copyWith(
                        color: Theme.of(context)
                            .colorScheme
                            .onSurface
                            .withValues(alpha: 0.7),
                      ),
                ),
                const SizedBox(height: 24),
                Expanded(
                  child: streamsAsync.when(
                    data: (streams) => _StreamsList(streams: streams),
                    loading: () => const Center(child: CircularProgressIndicator()),
                    error: (error, stackTrace) => _StreamsError(
                      onRetry: () => ref.invalidate(streamsProvider),
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _ConnectionStatusIndicator extends ConsumerWidget {
  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final connectionStatus = ref.watch(backendConnectionStatusProvider);
    
    return Row(
      children: [
        Icon(
          connectionStatus.valueOrNull == true ? Icons.cloud_done : Icons.cloud_off,
          color: connectionStatus.valueOrNull == true ? Colors.green : Colors.red,
          size: 20,
        ),
        const SizedBox(width: 8),
        Text(
          connectionStatus.valueOrNull == true ? 'Connected' : 'Disconnected',
          style: TextStyle(
            color: connectionStatus.valueOrNull == true ? Colors.green : Colors.red,
          ),
        ),
      ],
    );
  }
}

class _StreamsList extends ConsumerWidget {
  const _StreamsList({required this.streams});

  final List<VideoStream> streams;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return GridView.builder(
      gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
        crossAxisCount: 2,
        mainAxisSpacing: 20,
        crossAxisSpacing: 20,
        childAspectRatio: 1.2,
      ),
      itemCount: streams.length,
      itemBuilder: (context, index) {
        return _StreamCard(stream: streams[index]);
      },
    );
  }
}

class _StreamCard extends ConsumerWidget {
  const _StreamCard({required this.stream});

  final VideoStream stream;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final statusColor = switch (stream.status) {
      StreamStatus.online => Colors.green,
      StreamStatus.degraded => Colors.orange,
      StreamStatus.offline => Colors.red,
    };

    return GlassPanel(
      child: Stack(
        children: [
          // Background gradient
          Positioned.fill(
            child: DecoratedBox(
              decoration: BoxDecoration(
                borderRadius: BorderRadius.circular(24),
                gradient: LinearGradient(
                  colors: [
                    Theme.of(context).colorScheme.primary.withValues(alpha: 0.3),
                    Theme.of(context).colorScheme.secondary.withValues(alpha: 0.3),
                  ],
                  begin: Alignment.topLeft,
                  end: Alignment.bottomRight,
                ),
              ),
            ),
          ),
          // Status indicator
          Positioned(
            top: 16,
            right: 16,
            child: Container(
              padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
              decoration: BoxDecoration(
                color: Colors.black54,
                borderRadius: BorderRadius.circular(20),
              ),
              child: Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Icon(Icons.circle, color: statusColor, size: 10),
                  const SizedBox(width: 6),
                  Text(
                    switch (stream.status) {
                      StreamStatus.online => context.online,
                      StreamStatus.degraded => context.jitter,
                      StreamStatus.offline => context.offline,
                    },
                    style: Theme.of(context)
                        .textTheme
                        .labelMedium
                        ?.copyWith(color: Colors.white),
                  ),
                ],
              ),
            ),
          ),
          // Stream content
          Padding(
            padding: const EdgeInsets.all(24),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  stream.name,
                  style: Theme.of(context).textTheme.titleLarge?.copyWith(
                        fontWeight: FontWeight.w700,
                        color: Colors.white,
                      ),
                ),
                const SizedBox(height: 8),
                Text(
                  stream.location,
                  style: Theme.of(context)
                      .textTheme
                      .bodySmall
                      ?.copyWith(color: Colors.white70),
                ),
                const SizedBox(height: 8),
                Text(
                  stream.url,
                  style: Theme.of(context)
                      .textTheme
                      .bodySmall
                      ?.copyWith(color: Colors.white70),
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
                const Spacer(),
                Wrap(
                  spacing: 8,
                  runSpacing: 8,
                  children: [
                    _StreamChip(
                      label: stream.protocol.name.toUpperCase(),
                      icon: Icons.connected_tv_outlined,
                    ),
                    for (final tag in stream.tags.take(2))
                      _StreamChip(label: tag),
                  ],
                ),
                const SizedBox(height: 8),
                SizedBox(
                  width: double.infinity,
                  child: ElevatedButton.icon(
                    onPressed: () {
                      // Set the selected stream
                      ref.read(selectedStreamProvider.notifier).state = stream;
                      
                      // Navigate to stream viewer
                      Navigator.of(context).push(
                        MaterialPageRoute(
                          builder: (context) => StreamViewerPage(stream: stream),
                        ),
                      );
                    },
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.white.withValues(alpha: 0.2),
                      foregroundColor: Colors.white,
                      elevation: 0,
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(18),
                      ),
                    ),
                    icon: const Icon(Icons.play_arrow),
                    label: Text(context.viewStream),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class _StreamChip extends StatelessWidget {
  const _StreamChip({required this.label, this.icon});

  final String label;
  final IconData? icon;

  @override
  Widget build(BuildContext context) {
    return GlassPanel(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
      borderRadius: BorderRadius.circular(16),
      blurSigma: 12,
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          if (icon != null) ...[
            Icon(icon, size: 16, color: Colors.white70),
            const SizedBox(width: 6),
          ],
          Text(
            label,
            style: Theme.of(context)
                .textTheme
                .bodySmall
                ?.copyWith(color: Colors.white),
          ),
        ],
      ),
    );
  }
}

class _StreamsError extends StatelessWidget {
  const _StreamsError({required this.onRetry});

  final VoidCallback onRetry;

  @override
  Widget build(BuildContext context) {
    return Center(
      child: GlassPanel(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              context.failedToLoadStreams,
              style: Theme.of(context).textTheme.titleMedium,
            ),
            const SizedBox(height: 12),
            FilledButton.icon(
              onPressed: onRetry,
              icon: const Icon(Icons.refresh),
              label: Text(context.retry),
            ),
          ],
        ),
      ),
    );
  }
}