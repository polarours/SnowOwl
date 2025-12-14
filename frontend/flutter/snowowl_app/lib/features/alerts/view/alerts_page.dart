import 'package:flutter/material.dart';
import 'package:flutter_animate/flutter_animate.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/theme/app_colors.dart';
import 'package:snow_owl/widgets/glass_panel.dart';
import 'package:snow_owl/features/alerts/application/alerts_providers.dart';
import 'package:snow_owl/features/alerts/domain/alert_models.dart';
import 'package:snow_owl/l10n/l10n.dart'; 
class AlertsPage extends ConsumerWidget {
  const AlertsPage({super.key});

  static const routePath = '/alerts';
  static const routeName = 'alerts';
  static const navigationIcon = Icons.notifications_active_outlined;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final alerts = ref.watch(recentAlertsProvider);
    final l10n = context; 

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
            padding: const EdgeInsets.fromLTRB(24, 24, 24, 32),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    Text(
                      l10n.alerts,
                      style: Theme.of(context).textTheme.headlineMedium,
                    ),
                    const Spacer(),
                    FilledButton.icon(
                      onPressed: () => ref.invalidate(recentAlertsProvider),
                      icon: const Icon(Icons.refresh),
                      label: Text(l10n.reload), 
                    ),
                  ],
                ),
                const SizedBox(height: 10),
                Text(
                  'Real-time tracking of security, network, and streaming status changes with rapid coordination and traceability.',
                  style: Theme.of(context)
                      .textTheme
                      .bodyLarge
                      ?.copyWith(
          color: Theme.of(context)
              .colorScheme
              .onSurface
              .withValues(alpha: 0.7),),
                ),
                const SizedBox(height: 18),
                Expanded(
                  child: alerts.when(
                    data: (items) => ListView.separated(
                      itemCount: items.length,
                      separatorBuilder: (_, __) => const SizedBox(height: 16),
                      itemBuilder: (context, index) =>
                          _AlertTile(event: items[index], index: index),
                    ),
                    loading: () =>
                        const Center(child: CircularProgressIndicator()),
                    error: (error, stackTrace) => _AlertsError(
                      onRetry: () {
                        ref.invalidate(recentAlertsProvider);
                      },
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

class _AlertTile extends StatelessWidget {
  const _AlertTile({required this.event, required this.index});

  final AlertEvent event;
  final int index;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final time =
        '${event.timestamp.hour.toString().padLeft(2, '0')}:${event.timestamp.minute.toString().padLeft(2, '0')}';

    return GlassPanel(
      padding: const EdgeInsets.all(24),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Container(
            width: 48,
            height: 48,
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(16),
              gradient: LinearGradient(
                colors: [
                  event.severity.tint.withOpacityPercent(0.18),
                  event.severity.tint.withOpacityPercent(0.04),
                ],
                begin: Alignment.topLeft,
                end: Alignment.bottomRight,
              ),
            ),
            child: Icon(
              switch (event.severity) {
                AlertSeverity.critical => Icons.warning_rounded,
                AlertSeverity.warning => Icons.error_outline,
                AlertSeverity.info => Icons.info_outline,
              },
              color: event.severity.tint,
            ),
          ),
          const SizedBox(width: 18),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    Text(_getLocalizedTitle(context, event.title), style: theme.textTheme.titleLarge),
                    const SizedBox(width: 12),
                    GlassPanel(
                      padding: const EdgeInsets.symmetric(
                        horizontal: 10,
                        vertical: 6,
                      ),
                      borderRadius: BorderRadius.circular(14),
                      blurSigma: 12,
                      child: Text(
                        event.severity.label,
                        style: theme.textTheme.labelSmall
                            ?.copyWith(color: event.severity.tint),
                      ),
                    ),
                    const Spacer(),
                    Text(
                      time,
                      style: theme.textTheme.bodySmall
                          ?.copyWith(
                              color: theme.colorScheme.onSurface.withValues(alpha: 0.54),),
                    ),
                  ],
                ),
                const SizedBox(height: 8),
                Text(
                  _getLocalizedDescription(context, event.description),
                  style: theme.textTheme.bodyMedium
                      ?.copyWith(
                          color: theme.colorScheme.onSurface.withValues(alpha: 0.7),),
                ),
                const SizedBox(height: 12),
                Wrap(
                  spacing: 12,
                  children: [
                    Chip(
                      avatar: const Icon(Icons.location_on, size: 16),
                      label: Text(_getLocalizedLocation(context, event.location)),
                      backgroundColor: theme.colorScheme.onSurface.withValues(alpha: 0.1),
                    ),
                    Chip(
                      avatar: const Icon(Icons.device_hub, size: 16),
                      label: Text(event.relatedDevice),
                      backgroundColor: theme.colorScheme.onSurface.withValues(alpha: 0.1),
                    ),
                  ],
                ),
                const SizedBox(height: 12),
                Row(
                  children: [
                    TextButton.icon(
                      onPressed: () {},
                      icon: Icon(
                        event.acknowledged
                            ? Icons.check_circle
                            : Icons.circle_outlined,
                      ),
                      label: Text(event.acknowledged ? context.acknowledged : context.acknowledge),
                    ),
                    const SizedBox(width: 12),
                    TextButton.icon(
                      onPressed: () {},
                      icon: const Icon(Icons.play_circle_outline),
                      label: Text(context.playback),
                    ),
                    const Spacer(),
                    IconButton(
                      onPressed: () {},
                      icon: const Icon(Icons.more_horiz_rounded),
                    ),
                  ],
                ),
              ],
            ),
          ),
        ],
      ),
    ).animate().moveX(
      begin: 24,
      end: 0,
      delay: (index * 60).ms,
    ).fadeIn(
      duration: 240.ms,
      delay: (index * 60).ms,
    );
  }
  
  String _getLocalizedTitle(BuildContext context, String key) {
    switch (key) {
      case 'intrusionDetectionGate1':
        return context.intrusionDetectionGate1;
      case 'temperatureAnomalyRecovered':
        return context.temperatureAnomalyRecovered;
      case 'streamBitrateFluctuation':
        return context.streamBitrateFluctuation;
      case 'cameraFirmwareUpdateAvailable':
        return context.cameraFirmwareUpdateAvailable;
      default:
        return key;
    }
  }
  
  String _getLocalizedDescription(BuildContext context, String key) {
    switch (key) {
      case 'aiDetectedSuspiciousLoitering':
        return context.aiDetectedSuspiciousLoitering;
      case 'coldChainTempReturnedNormal':
        return context.coldChainTempReturnedNormal;
      case 'networkQualityFluctuation':
        return context.networkQualityFluctuation;
      case 'firmwareUpdateAvailable':
        return context.firmwareUpdateAvailable;
      default:
        return key;
    }
  }
  
  String _getLocalizedLocation(BuildContext context, String key) {
    switch (key) {
      case 'shanghaiHeadquarters':
        return context.shanghaiHeadquarters;
      case 'shenzhenStorage':
        return context.shenzhenStorage;
      case 'hangzhouFieldStation':
        return context.hangzhouFieldStation;
      case 'nanjingLab':
        return context.nanjingLab;
      default:
        return key;
    }
  }
}

class _AlertsError extends StatelessWidget {
  const _AlertsError({required this.onRetry});

  final VoidCallback onRetry;

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(
            context.deviceDetailsLoadError,
            style: Theme.of(context)
                .textTheme
                .bodyLarge
                ?.copyWith(
                    color: Theme.of(context).colorScheme.onSurface.withValues(alpha: 0.7),),
          ),
          const SizedBox(height: 16),
          FilledButton.icon(
            onPressed: onRetry,
            icon: const Icon(Icons.refresh),
            label: Text(context.retry),
          ),
        ],
      ),
    );
  }
}
