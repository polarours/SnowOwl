import 'package:flutter/material.dart';
import 'package:flutter_animate/flutter_animate.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import 'package:snow_owl/theme/app_colors.dart';
import 'package:snow_owl/widgets/glass_panel.dart';
import 'package:snow_owl/features/dashboard/application/dashboard_providers.dart';
import 'package:snow_owl/features/dashboard/application/dashboard_edit_providers.dart';
import 'package:snow_owl/features/dashboard/domain/dashboard_models.dart';
import 'package:snow_owl/features/dashboard/domain/editable_elements.dart';
import 'package:snow_owl/l10n/l10n.dart';
import 'package:snow_owl/features/devices/application/device_providers.dart';
import 'package:snow_owl/features/devices/domain/device_models.dart';
import 'package:snow_owl/features/settings/application/settings_providers.dart';
import 'package:snow_owl/features/monitoring/application/monitoring_providers.dart';
import 'package:snow_owl/features/dashboard/view/scene_editor_page.dart';

class DashboardPage extends ConsumerWidget {
  const DashboardPage({super.key});

  static const routePath = '/';
  static const routeName = 'dashboard';

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final snapshotAsync = ref.watch(dashboardSnapshotProvider);

    return Scaffold(
      extendBody: true,
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            colors: [
              Theme.of(context).colorScheme.surface,
              Theme.of(context).colorScheme.surface,
              Theme.of(context).colorScheme.secondary.withValues(alpha: 0.1),
            ],
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
          ),
        ),
        child: SafeArea(
          bottom: false,
          child: snapshotAsync.when(
            data: (snapshot) => _DashboardContent(snapshot: snapshot),
            loading: () => const _DashboardLoading(),
            error: (error, stackTrace) => _DashboardError(
              message: context.deviceDetailsLoadError,
              onRetry: () => ref.invalidate(dashboardSnapshotProvider),
            ),
          ),
        ),
      ),
      floatingActionButton: const _EditFloatingActionButton(),
    );
  }
}

class _EditFloatingActionButton extends ConsumerWidget {
  const _EditFloatingActionButton();

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isEditing = ref.watch(dashboardEditStateProvider.select((value) => value.isEditing));

    return FloatingActionButton(
      onPressed: () {
        ref.read(dashboardEditStateProvider.notifier).toggleEditMode();
      },
      backgroundColor: isEditing ? AppColors.alertOrange : AppColors.monitoringBlue,
      foregroundColor: Colors.white,
      child: Icon(isEditing ? Icons.check : Icons.edit),
    );
  }
}

class _DashboardContent extends StatelessWidget {
  const _DashboardContent({required this.snapshot});

  final DashboardSnapshot snapshot;

  @override
  Widget build(BuildContext context) {
    return CustomScrollView(
      physics: const BouncingScrollPhysics(),
      slivers: [
        SliverPadding(
          padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 24),
          sliver: SliverToBoxAdapter(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const _ConnectionStatusHeader(),
                const SizedBox(height: 16),
                // Message overview taking full width
                _HeroHeader(metrics: snapshot.heroMetrics),
                const SizedBox(height: 24),
                Text(
                  context.deviceManagementTitle,
                  style: Theme.of(context).textTheme.titleLarge,
                ),
                const SizedBox(height: 16),
                _QuickActionsGrid(actions: snapshot.quickActions),
                const SizedBox(height: 32),
                Text(
                  context.deviceManagementDescription,
                  style: Theme.of(context).textTheme.titleLarge,
                ),
                const SizedBox(height: 16),
                _SystemSummarySection(metrics: snapshot.systemMetrics),
                const SizedBox(height: 16),
                const _EditableZonesSection(),
                const SizedBox(height: 120),
              ],
            ).animate().fadeIn(duration: 400.ms).moveY(begin: 24, end: 0),
          ),
        ),
      ],
    );
  }
}
class _EditableZonesSection extends ConsumerWidget {
  const _EditableZonesSection();

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final editState = ref.watch(dashboardEditStateProvider);
    final isEditing = editState.isEditing;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Text(
              'Zones and Devices',
              style: Theme.of(context).textTheme.titleLarge,
            ),
            if (isEditing)
              TextButton(
                onPressed: () {
                  // Generate a simple ID for demo purposes
                  final newZone = DashboardZone(
                    id: 'zone_${DateTime.now().millisecondsSinceEpoch}',
                    name: 'New Zone',
                    description: 'Description of the new zone',
                  );
                  ref.read(dashboardEditStateProvider.notifier).addZone(newZone);
                },
                child: Text('Add Zone'),
              ),
          ],
        ),
        const SizedBox(height: 16),
        if (editState.zones.isEmpty && !isEditing)
          GlassPanel(
            padding: const EdgeInsets.all(24),
            child: Text(
              'No zones configured. Enter edit mode to add zones and devices.',
              textAlign: TextAlign.center,
              style: Theme.of(context).textTheme.bodyMedium,
            ),
          )
        else
          _ZonesGrid(zones: editState.zones),
      ],
    );
  }
}

class _ZonesGrid extends ConsumerWidget {
  final List<DashboardZone> zones;

  const _ZonesGrid({required this.zones});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isEditing = ref.watch(dashboardEditStateProvider.select((value) => value.isEditing));

    return GridView.builder(
      shrinkWrap: true,
      physics: const NeverScrollableScrollPhysics(),
      gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
        crossAxisCount: 2,
        crossAxisSpacing: 16,
        mainAxisSpacing: 16,
        childAspectRatio: 1.5,
      ),
      itemCount: zones.length + (isEditing ? 1 : 0), // Add one for the "add zone" button
      itemBuilder: (context, index) {
        if (isEditing && index == zones.length) {
          // "Add Zone" button in edit mode
          return _AddZoneCard(
            onTap: () {
              final newZone = DashboardZone(
                id: 'zone_${DateTime.now().millisecondsSinceEpoch}',
                name: 'New Zone',
                description: 'Description of the new zone',
              );
              ref.read(dashboardEditStateProvider.notifier).addZone(newZone);
            },
          );
        }

        final zone = zones[index];
        return _ZoneCard(zone: zone);
      },
    );
  }
}

class _AddZoneCard extends StatelessWidget {
  final VoidCallback onTap;

  const _AddZoneCard({required this.onTap});

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: GlassPanel(
        padding: const EdgeInsets.all(16),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(Icons.add, size: 32, color: Colors.white70),
            const SizedBox(height: 8),
            Text(
              'Add Zone',
              style: Theme.of(context).textTheme.bodyMedium,
            ),
          ],
        ),
      ),
    );
  }
}

class _ZoneCard extends ConsumerWidget {
  final DashboardZone zone;

  const _ZoneCard({required this.zone});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isEditing = ref.watch(dashboardEditStateProvider.select((value) => value.isEditing));

    return GlassPanel(
      padding: const EdgeInsets.all(16),
      child: Stack(
        children: [
          Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                zone.name,
                style: Theme.of(context).textTheme.titleMedium,
              ),
              const SizedBox(height: 4),
              Text(
                zone.description,
                style: Theme.of(context).textTheme.bodySmall?.copyWith(
                      color: Colors.white70,
                    ),
                maxLines: 2,
                overflow: TextOverflow.ellipsis,
              ),
              const SizedBox(height: 8),
              Expanded(
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.start,
                  children: [
                    Icon(Icons.devices_other, size: 16, color: Colors.white70),
                    const SizedBox(width: 4),
                    Text(
                      '${zone.elements.length} devices',
                      style: Theme.of(context).textTheme.bodySmall?.copyWith(
                            color: Colors.white70,
                          ),
                    ),
                  ],
                ),
              ),
            ],
          ),
          if (isEditing)
            Positioned(
              top: 4,
              right: 4,
              child: GestureDetector(
                onTap: () {
                  // Show zone editor dialog
                  _showZoneEditorDialog(context, ref, zone);
                },
                child: Container(
                  padding: const EdgeInsets.all(4),
                  decoration: const BoxDecoration(
                    color: AppColors.alertOrange,
                    shape: BoxShape.circle,
                  ),
                  child: const Icon(Icons.edit, size: 16, color: Colors.white),
                ),
              ),
            ),
        ],
      ),
    );
  }

  void _showZoneEditorDialog(BuildContext context, WidgetRef ref, DashboardZone zone) {
    final nameController = TextEditingController(text: zone.name);
    final descriptionController = TextEditingController(text: zone.description);

    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text('Edit Zone'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              TextField(
                controller: nameController,
                decoration: InputDecoration(labelText: 'Zone Name'),
              ),
              const SizedBox(height: 16),
              TextField(
                controller: descriptionController,
                decoration: InputDecoration(labelText: 'Description'),
              ),
              const SizedBox(height: 16),
              Row(
                mainAxisAlignment: MainAxisAlignment.end,
                children: [
                  TextButton(
                    onPressed: () {
                      Navigator.of(context).pop();
                    },
                    child: Text('Cancel'),
                  ),
                  const SizedBox(width: 8),
                  ElevatedButton(
                    onPressed: () {
                      final updatedZone = zone.copyWith(
                        name: nameController.text,
                        description: descriptionController.text,
                      );
                      ref.read(dashboardEditStateProvider.notifier).updateZone(updatedZone);
                      Navigator.of(context).pop();
                    },
                    child: Text('Save'),
                  ),
                ],
              ),
            ],
          ),
        );
      },
    );
  }
}

class _ConnectionStatusHeader extends ConsumerWidget {
  const _ConnectionStatusHeader();

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final connectionStatus = ref.watch(backendConnectionStatusProvider);
    final settings = ref.watch(settingsStateProvider);
    final isServerConnected = connectionStatus.valueOrNull == true;
    final isServerConfigured = settings.serverConfig.apiUrl.isNotEmpty;

    return GlassPanel(
      padding: const EdgeInsets.all(16),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          // Server connection status
          Row(
            children: [
              Container(
                width: 12,
                height: 12,
                decoration: BoxDecoration(
                  color: isServerConnected ? AppColors.successGreen : (isServerConfigured ? AppColors.alertOrange : Colors.grey),
                  borderRadius: BorderRadius.circular(6),
                ),
              ),
              const SizedBox(width: 8),
              Text(
                isServerConnected 
                  ? context.serverConnected 
                  : (isServerConfigured ? context.serverDisconnected : context.serverNotConfigured),
                style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                  fontWeight: FontWeight.w600,
                ),
              ),
            ],
          ),
          // Server URL
          if (isServerConfigured)
            Text(
              settings.serverConfig.apiUrl,
              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                color: Theme.of(context).colorScheme.onSurface.withValues(alpha: 0.7),
              ),
            ),
        ],
      ),
    );
  }
}

class _DashboardLoading extends StatelessWidget {
  const _DashboardLoading();

  @override
  Widget build(BuildContext context) {
    return const Center(child: CircularProgressIndicator());
  }
}

class _DashboardError extends StatelessWidget {
  const _DashboardError({required this.message, required this.onRetry});

  final String message;
  final VoidCallback onRetry;

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(
            message,
            style: Theme.of(context)
                .textTheme
                .bodyLarge
                ?.copyWith(color: Theme.of(context).colorScheme.onSurface.withValues(alpha: 0.7)),
          ),
          const SizedBox(height: 16),
          ElevatedButton.icon(
            onPressed: onRetry,
            icon: const Icon(Icons.refresh),
            label: Text(context.reload),
          ),
        ],
      ),
    );
  }
}

class _HeroHeader extends ConsumerWidget {
  const _HeroHeader({required this.metrics});

  final List<DashboardHeroMetric> metrics;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final theme = Theme.of(context);
    final deviceTree = ref.watch(deviceTreeProvider);
    final monitoringSummary = ref.watch(monitoringSummaryProvider);
    
    return GlassPanel(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            context.systemOverview,
            style: theme.textTheme.headlineSmall?.copyWith(
              fontWeight: FontWeight.w700,
            ),
          ),
          const SizedBox(height: 12),
          Text(
            context.systemOverviewDescription,
            style: theme.textTheme.bodyMedium?.copyWith(
                color: theme.colorScheme.onSurface.withValues(alpha: 0.7),),
          ),
          const SizedBox(height: 20),
          // Restore the original metrics display in a single row
          Wrap(
            spacing: 12,
            runSpacing: 12,
            children: [
              for (final metric in metrics)
                _HeroMetricChip(
                  label: _getLocalizedText(context, metric.label),
                  value: metric.value,
                  color: metric.color,
                ),
            ],
          ),
          const SizedBox(height: 16),
          // Add additional information in a simple row
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              // Date and time
              Row(
                children: [
                  Icon(Icons.access_time, size: 16, color: Colors.white70),
                  const SizedBox(width: 8),
                  Text(
                    _getCurrentTimeString(),
                    style: theme.textTheme.bodyMedium?.copyWith(
                      color: theme.colorScheme.onSurface, // Changed from Colors.white
                      fontWeight: FontWeight.w600,
                    ),
                  ),
                ],
              ),
              // Weather (mock data)
              Row(
                children: [
                  Icon(Icons.cloud_outlined, size: 16, color: Colors.white70),
                  const SizedBox(width: 8),
                  Text(
                    '22°C, Sunny',
                    style: theme.textTheme.bodyMedium?.copyWith(
                      color: theme.colorScheme.onSurface, // Changed from Colors.white
                      fontWeight: FontWeight.w600,
                    ),
                  ),
                ],
              ),
              // System info from device tree and monitoring summary
              deviceTree.when(
                data: (devices) => monitoringSummary.when(
                  data: (summary) {
                    final onlineCount = devices.where((d) => d.status == DeviceStatus.online).length;
                    return Text(
                      '${context.onlineDevices}: $onlineCount, ${context.todayAlerts}: ${summary.alertsToday}',
                      style: theme.textTheme.bodySmall?.copyWith(
                        color: theme.colorScheme.onSurface.withValues(alpha: 0.7),
                      ),
                    );
                  },
                  loading: () => const Text('Loading...'),
                  error: (_, __) => const Text('Error'),
                ),
                loading: () => const Text('Loading...'),
                error: (_, __) => const Text('Error'),
              ),
            ],
          ),
        ],
      ),
    );
  }
  
  String _getCurrentTimeString() {
    final now = DateTime.now();
    return '${now.year}-${now.month.toString().padLeft(2, '0')}-${now.day.toString().padLeft(2, '0')} '
           '${now.hour.toString().padLeft(2, '0')}:${now.minute.toString().padLeft(2, '0')}';
  }
}

// New widget for displaying information cards
class _InfoCard extends StatelessWidget {
  final IconData icon;
  final String label;
  final String value;

  const _InfoCard({
    required this.icon,
    required this.label,
    required this.value,
  });

  @override
  Widget build(BuildContext context) {
    return GlassPanel(
      padding: const EdgeInsets.all(16),
      child: Column(
        children: [
          Icon(icon, color: Colors.white70),
          const SizedBox(height: 8),
          Text(
            value,
            style: Theme.of(context).textTheme.headlineSmall,
          ),
          Text(
            label,
            style: Theme.of(context).textTheme.bodySmall?.copyWith(
                  color: Colors.white70,
                ),
          ),
        ],
      ),
    );
  }
}

class _HeroMetricChip extends StatelessWidget {
  const _HeroMetricChip({
    required this.label,
    required this.value,
    required this.color,
  });

  final String label;
  final String value;
  final Color color;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return GlassPanel(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      borderRadius: BorderRadius.circular(18),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 10,
            height: 10,
            decoration: BoxDecoration(
              color: color,
              borderRadius: BorderRadius.circular(14),
            ),
          ),
          const SizedBox(width: 10),
          Text(
            value,
            style: theme.textTheme.titleLarge,
          ),
          const SizedBox(width: 6),
          Text(
            label,
            style: theme.textTheme.bodySmall?.copyWith(
                  color: theme.colorScheme.onSurface.withValues(alpha: 0.7),
                  fontWeight: FontWeight.w600,
                ),
          ),
        ],
      ),
    );
  }
}

class _QuickActionsGrid extends StatelessWidget {
  const _QuickActionsGrid({required this.actions});

  final List<DashboardQuickAction> actions;

  @override
  Widget build(BuildContext context) {
    // Add the scene editor action to the quick actions
    final actionsWithSceneEditor = [
      ...actions,
      DashboardQuickAction(
        title: 'sceneEditor',
        subtitle: 'Design facility layouts',
        icon: SceneEditorPage.navigationIcon,
        color: AppColors.warningAmber,
        routePath: SceneEditorPage.routePath,
      ),
    ];

    return GridView.count(
      shrinkWrap: true,
      physics: const NeverScrollableScrollPhysics(),
      crossAxisCount: 2,
      crossAxisSpacing: 16,
      mainAxisSpacing: 16,
      childAspectRatio: 1.2,
      children: [
        for (final action in actionsWithSceneEditor)
          _QuickActionCard(
            title: _getLocalizedText(context, action.title),
            subtitle: _getLocalizedText(context, action.subtitle),
            icon: action.icon,
            color: action.color,
            routePath: action.routePath,
          ),
      ],
    ).animate().fadeIn(duration: 320.ms, delay: 120.ms);
  }
}

class _QuickActionCard extends StatelessWidget {
  const _QuickActionCard({
    required this.title,
    required this.subtitle,
    required this.icon,
    required this.color,
    this.routePath,
  });

  final String title;
  final String subtitle;
  final IconData icon;
  final Color color;
  final String? routePath;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return GestureDetector(
      onTap: routePath == null ? null : () => context.go(routePath!),
      child: GlassPanel(
        padding: const EdgeInsets.all(20),
        borderRadius: BorderRadius.circular(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Container(
              width: 44,
              height: 44,
              decoration: BoxDecoration(
                color: color.withOpacityPercent(0.18),
                borderRadius: BorderRadius.circular(18),
              ),
              child: Icon(icon, color: color, size: 24),
            ),
            const Spacer(),
            Text(
              title,
              style: theme.textTheme.titleLarge,
            ),
            const SizedBox(height: 8),
            Text(
              subtitle,
              style: theme.textTheme.bodySmall?.copyWith(color: Colors.white70),
            ),
          ],
        ),
      ).animate().moveY(begin: 10, end: 0).fadeIn(duration: 260.ms),
    );
  }
}

class _SystemSummarySection extends StatelessWidget {
  const _SystemSummarySection({required this.metrics});

  final List<DashboardSystemMetric> metrics;

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        for (int index = 0; index < metrics.length; index++) ...[
          if (index != 0) const SizedBox(height: 12),
          _SystemSummaryTile(metric: metrics[index]),
        ],
      ],
    );
  }
}

class _SystemSummaryTile extends StatelessWidget {
  const _SystemSummaryTile({required this.metric});

  final DashboardSystemMetric metric;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final trendColor = metric.trendDirection == MetricTrendDirection.up
        ? AppColors.successGreen
        : AppColors.captureRed;
    final trendIcon = metric.trendDirection == MetricTrendDirection.up
        ? Icons.north_east
        : Icons.south_east;

    return GlassPanel(
      padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 18),
      borderRadius: BorderRadius.circular(22),
      child: Row(
        children: [
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  _getLocalizedText(context, metric.title),
                  style: theme.textTheme.bodyMedium
                      ?.copyWith(color: Colors.white70),
                ),
                const SizedBox(height: 6),
                Text(
                  metric.value,
                  style: theme.textTheme.headlineSmall,
                ),
              ],
            ),
          ),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
            decoration: BoxDecoration(
              color: trendColor.withOpacityPercent(0.16),
              borderRadius: BorderRadius.circular(16),
            ),
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(trendIcon, color: trendColor, size: 16),
                const SizedBox(width: 6),
                Text(
                  metric.trendLabel,
                  style: theme.textTheme.bodySmall?.copyWith(
                    color: trendColor,
                    fontWeight: FontWeight.w600,
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

class _SceneListItem extends ConsumerWidget {
  final DashboardZone zone;

  const _SceneListItem({required this.zone});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return GlassPanel(
      padding: const EdgeInsets.all(12),
      margin: const EdgeInsets.only(bottom: 8),
      child: Row(
        children: [
          Icon(Icons.account_tree_outlined, color: Colors.white70),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  zone.name,
                  style: Theme.of(context).textTheme.bodyMedium,
                ),
                Text(
                  '${zone.elements.length} devices',
                  style: Theme.of(context).textTheme.bodySmall?.copyWith(
                        color: Colors.white70,
                      ),
                ),
              ],
            ),
          ),
          IconButton(
            icon: const Icon(Icons.visibility, size: 18),
            onPressed: () {
              // Preview scene
              ScaffoldMessenger.of(context).showSnackBar(
                SnackBar(content: Text('Preview functionality coming soon')),
              );
            },
          ),
          IconButton(
            icon: const Icon(Icons.edit, size: 18),
            onPressed: () {
              // Edit scene
              ref
                  .read(dashboardEditStateProvider.notifier)
                  .selectZone(zone);
              context.push(SceneEditorPage.routePath);
            },
          ),
        ],
      ),
    );
  }
}

String _getLocalizedText(BuildContext context, String key) {
  switch (key) {
    case 'onlineDevices':
      return context.onlineDevices;
    case 'todayAlerts':
      return context.todayAlerts;
    case 'scheduledMaintenance':
      return context.scheduledMaintenance;
    case 'monitoringCenterQuickAction':
      return context.monitoringCenterQuickAction;
    case 'monitoringCenterSubtitle':
      return context.monitoringCenterSubtitle;
    case 'fieldCaptureStation':
      return context.fieldCaptureStation;
    case 'fieldCaptureSubtitle':
      return context.fieldCaptureSubtitle;
    case 'deviceDeployment':
      return context.deviceDeployment;
    case 'deviceDeploymentSubtitle':
      return context.deviceDeploymentSubtitle;
    case 'systemHealth':
      return context.systemHealth;
    case 'systemHealthSubtitle':
      return context.systemHealthSubtitle;
    case 'uptimeRate':
      return context.uptimeRate;
    case 'streamingSuccessRate':
      return context.streamingSuccessRate;
    case 'averageLatency':
      return context.averageLatency;
    case 'sceneEditor':
      return context.sceneEditor;
    case 'designFacilityLayouts':
      return context.designFacilityLayouts;
    default:
      return key;
  }
}