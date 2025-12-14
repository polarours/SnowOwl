import 'package:flutter/material.dart';
import 'package:flutter_animate/flutter_animate.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/theme/app_colors.dart';
import 'package:snow_owl/widgets/glass_panel.dart';
import 'package:snow_owl/features/monitoring/application/monitoring_providers.dart';
import 'package:snow_owl/features/monitoring/application/detection_provider.dart';
import 'package:snow_owl/features/monitoring/domain/monitoring_models.dart';
import 'package:snow_owl/l10n/l10n.dart';
import 'package:snow_owl/features/monitoring/view/stream_viewer_page.dart';
import 'package:snow_owl/features/devices/application/device_providers.dart';
import 'package:snow_owl/features/devices/domain/device_models.dart';
import 'package:snow_owl/features/shared/video_player_widget.dart';

class MonitoringHubPage extends ConsumerWidget {
  const MonitoringHubPage({super.key});

  static const routePath = '/monitoring';
  static const routeName = 'monitoring-hub';
  static const navigationIcon = Icons.remove_red_eye_outlined;

  void _showSceneEditor(BuildContext context) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return const _SceneEditorDialog();
      },
    );
  }

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final summary = ref.watch(monitoringSummaryProvider);
    final deviceTree = ref.watch(deviceTreeProvider);
    final isDetectionEnabled = ref.watch(detectionEnabledProvider);

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
          child: CustomScrollView(
            physics: const BouncingScrollPhysics(),
            slivers: [
              SliverPadding(
                padding: const EdgeInsets.fromLTRB(24, 24, 24, 0),
                sliver: SliverToBoxAdapter(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Row(
                        children: [
                          Expanded(
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Text(
                                  context.monitoringCenter,
                                  style: Theme.of(context).textTheme.headlineMedium?.copyWith(
                                    fontWeight: FontWeight.bold,
                                  ),
                                ),
                                const SizedBox(height: 10),
                                Text(
                                  'Monitor all connected devices in real-time',
                                  style: Theme.of(context)
                                    .textTheme
                                    .bodyLarge
                                    ?.copyWith(
                                      color: Theme.of(context)
                                        .colorScheme
                                        .onSurface
                                        .withValues(alpha: 0.7),),
                                ),
                                const SizedBox(height: 10),
                                GlassPanel(
                                  padding: const EdgeInsets.all(12),
                                  child: Row(
                                    mainAxisSize: MainAxisSize.min,
                                    children: [
                                      Icon(
                                        isDetectionEnabled ? Icons.check_circle : Icons.cancel,
                                        color: isDetectionEnabled ? AppColors.successGreen : AppColors.captureRed,
                                        size: 20,
                                      ),
                                      const SizedBox(width: 8),
                                      Text(
                                        isDetectionEnabled ? context.detectionEnabled : context.detectionDisabled,
                                        style: Theme.of(context).textTheme.bodyMedium,
                                      ),
                                    ],
                                  ),
                                ),
                              ],
                            ),
                          ),
                          Switch(
                            value: isDetectionEnabled,
                            onChanged: (value) {
                              ref.read(detectionEnabledProvider.notifier).setDetectionEnabled(value);
                            },
                            activeColor: AppColors.successGreen,
                          ),
                          IconButton(
                            icon: const Icon(Icons.edit_outlined),
                            onPressed: () {
                              // Show scene editor
                              _showSceneEditor(context);
                            },
                            style: IconButton.styleFrom(
                              padding: const EdgeInsets.all(12),
                              backgroundColor: Theme.of(context).colorScheme.primary.withValues(alpha: 0.1),
                              shape: RoundedRectangleBorder(
                                borderRadius: BorderRadius.circular(12),
                              ),
                            ),
                          ),
                        ],
                      ),
                      const SizedBox(height: 24),
                      summary.when(
                        data: (data) => _SessionSummary(summary: data),
                        loading: () => const _SummaryLoading(),
                        error: (error, stackTrace) => _SummaryError(
                          onRetry: () {
                            ref.invalidate(monitoringSummaryProvider);
                          },
                        ),
                      ),
                      const SizedBox(height: 28),
                    ],
                  ).animate().fadeIn(duration: 360.ms).moveY(begin: 18, end: 0),
                ),
              ),
              deviceTree.when(
                data: (devices) => SliverPadding(
                  padding: const EdgeInsets.symmetric(horizontal: 24),
                  sliver: SliverGrid(
                    gridDelegate:
                        const SliverGridDelegateWithFixedCrossAxisCount(
                      crossAxisCount: 2,
                      mainAxisSpacing: 16,
                      crossAxisSpacing: 16,
                      childAspectRatio: 1.2,
                    ),
                    delegate: SliverChildBuilderDelegate(
                      (context, index) {
                        final device = devices[index];
                        // Only show camera devices in monitoring
                        if (device.kind == DeviceKind.camera) {
                          return _DeviceStreamCard(device: device);
                        }
                        return const SizedBox.shrink();
                      },
                      childCount: devices.length,
                    ),
                  ),
                ),
                loading: () => const SliverFillRemaining(
                  child: Center(child: CircularProgressIndicator()),
                ),
                error: (error, stackTrace) => SliverFillRemaining(
                  child: _StreamsError(
                    onRetry: () {
                      ref.invalidate(deviceTreeProvider);
                    },
                  ),
                ),
              ),
              const SliverToBoxAdapter(child: SizedBox(height: 120)),
            ],
          ),
        ),
      ),
    );
  }
}

class _SceneEditorDialog extends ConsumerStatefulWidget {
  const _SceneEditorDialog();

  @override
  _SceneEditorDialogState createState() => _SceneEditorDialogState();
}

class _SceneEditorDialogState extends ConsumerState<_SceneEditorDialog> {
  final List<DeviceNode> _selectedDevices = []; // ignore: unused_field
  final List<DeviceNode> _availableDevices = []; // ignore: unused_field
  final TextEditingController _sceneNameController = TextEditingController();
  final List<DeviceNode> _sceneDevices = [];

  String _getLocalizedName(BuildContext context, String key) {
    switch (key) {
      case 'headquartersCampus':
        return context.headquartersCampus;
      case 'buildingAMainControl':
        return context.buildingAMainControl;
      case 'buildingAEntrance1':
        return context.buildingAEntrance1;
      case 'buildingAUndergroundGarage':
        return context.buildingAUndergroundGarage;
      case 'buildingBLaboratory':
        return context.buildingBLaboratory;
      case 'labGasSensor':
        return context.labGasSensor;
      case 'southChinaBranch':
        return context.southChinaBranch;
      case 'storageNightVisionCamera':
        return context.storageNightVisionCamera;
      case 'storageTemperatureHumidity':
        return context.storageTemperatureHumidity;
      default:
        return key;
    }
  }

  @override
  Widget build(BuildContext context) {
    final deviceTree = ref.watch(deviceTreeProvider);
    final theme = Theme.of(context);

    return AlertDialog(
      title: Text(context.editScene),
      content: SizedBox(
        width: 800,
        height: 600,
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            TextField(
              controller: _sceneNameController,
              decoration: InputDecoration(
                labelText: context.sceneName,
                hintText: context.enterANameForYourScene,
              ),
            ),
            const SizedBox(height: 20),
            Text(context.sceneConfiguration, style: theme.textTheme.titleMedium),
            const SizedBox(height: 10),
            Expanded(
              child: Row(
                children: [
                  // Available devices
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(context.availableDevices, style: theme.textTheme.bodyLarge),
                        const SizedBox(height: 10),
                        Expanded(
                          child: deviceTree.when(
                            data: (devices) {
                              return _buildDeviceList(
                                context, 
                                devices.where((d) => !_sceneDevices.contains(d)).toList(),
                                false,
                              );
                            },
                            loading: () => const Center(child: CircularProgressIndicator()),
                            error: (error, stackTrace) => Center(
                              child: Text('${context.failedToLoadDevices}: $error'),
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                  const SizedBox(width: 20),
                  // Scene devices with drag and drop
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(context.sceneDevices, style: theme.textTheme.bodyLarge),
                        const SizedBox(height: 10),
                        Expanded(
                          child: _buildSceneDeviceList(context),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
      actions: [
        TextButton(
          onPressed: () {
            Navigator.of(context).pop();
          },
          child: Text(context.cancel),
        ),
        ElevatedButton(
          onPressed: () {
            // Save scene logic would go here
            Navigator.of(context).pop();
            ScaffoldMessenger.of(context).showSnackBar(
              SnackBar(
                content: Text('${context.scene} "${_sceneNameController.text}" ${context.savedWith} ${_sceneDevices.length} ${context.devicesCount}'),
                backgroundColor: AppColors.successGreen,
              ),
            );
          },
          child: Text(context.saveScene),
        ),
      ],
    );
  }
  
  Widget _buildDeviceList(BuildContext context, List<DeviceNode> devices, bool isSceneList) {
    return ListView.builder(
      itemCount: devices.length,
      itemBuilder: (context, index) {
        final device = devices[index];
        return Card(
          margin: const EdgeInsets.symmetric(vertical: 4),
          child: ListTile(
            title: Text(_getLocalizedName(context, device.name)),
            subtitle: Text('${device.kind} - ${_getLocalizedStatus(context, device.status)}'),
            trailing: isSceneList 
              ? IconButton(
                  icon: const Icon(Icons.delete),
                  onPressed: () {
                    setState(() {
                      _sceneDevices.remove(device);
                    });
                  },
                )
              : IconButton(
                  icon: const Icon(Icons.add),
                  onPressed: () {
                    setState(() {
                      if (!_sceneDevices.contains(device)) {
                        _sceneDevices.add(device);
                      }
                    });
                  },
                ),
          ),
        );
      },
    );
  }
  
  Widget _buildSceneDeviceList(BuildContext context) {
    return ReorderableListView(
      onReorder: (oldIndex, newIndex) {
        setState(() {
          if (newIndex > oldIndex) {
            newIndex -= 1;
          }
          final item = _sceneDevices.removeAt(oldIndex);
          _sceneDevices.insert(newIndex, item);
        });
      },
      children: [
        for (int i = 0; i < _sceneDevices.length; i++)
          Card(
            key: ValueKey(_sceneDevices[i]),
            margin: const EdgeInsets.symmetric(vertical: 4),
            child: ListTile(
              title: Text(_getLocalizedName(context, _sceneDevices[i].name)),
              subtitle: Text('${_sceneDevices[i].kind} - ${_getLocalizedStatus(context, _sceneDevices[i].status)}'),
              trailing: IconButton(
                icon: const Icon(Icons.delete),
                onPressed: () {
                  setState(() {
                    _sceneDevices.removeAt(i);
                  });
                },
              ),
            ),
          ),
      ],
    );
  }
  
  String _getLocalizedStatus(BuildContext context, DeviceStatus status) {
    switch (status) {
      case DeviceStatus.online:
        return context.online;
      case DeviceStatus.offline:
        return context.offline;
      case DeviceStatus.maintenance:
        return context.maintenance;
    }
  }
}

class _SessionSummary extends StatelessWidget {
  const _SessionSummary({required this.summary});

  final MonitoringSessionSummary summary;

  @override
  Widget build(BuildContext context) {
    final items = <_SummaryItem>[
      _SummaryItem(
        label: context.onlineChannels,
        value: '${summary.activeStreams}',
        caption: context.gateways,
        color: AppColors.successGreen,
        icon: Icons.videocam_rounded,
      ),
      _SummaryItem(
        label: context.totalBandwidth,
        value: '${summary.totalBandwidth.toStringAsFixed(1)} Gbps',
        caption: context.backboneLinkStats,
        color: AppColors.monitoringBlue,
        icon: Icons.waves_rounded,
      ),
      _SummaryItem(
        label: context.averageLatency,
        value: '${summary.avgLatency.inMilliseconds} ms',
        caption: context.sampledOver60Seconds,
        color: Colors.deepPurpleAccent,
        icon: Icons.speed_rounded,
      ),
      _SummaryItem(
        label: context.history,
        value: '${summary.alertsToday}',
        caption: '${context.pendingAlerts} ${summary.alertsToday ~/ 2}',
        color: AppColors.alertOrange,
        icon: Icons.warning_amber_rounded,
      ),
    ];

    return SizedBox(
      height: 160,
      child: ListView.separated(
        scrollDirection: Axis.horizontal,
        physics: const BouncingScrollPhysics(),
        padding: EdgeInsets.zero,
        separatorBuilder: (_, __) => const SizedBox(width: 18),
        itemCount: items.length,
        itemBuilder: (context, index) => SizedBox(
          width: 240,
          child: _SummaryCard(item: items[index]),
        ),
      ),
    );
  }
}

class _SummaryLoading extends StatelessWidget {
  const _SummaryLoading();

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      height: 160,
      child: ListView.separated(
        scrollDirection: Axis.horizontal,
        itemCount: 4,
        separatorBuilder: (_, __) => const SizedBox(width: 18),
        itemBuilder: (context, index) => const GlassPanel(
          padding: EdgeInsets.all(24),
          child: Center(child: CircularProgressIndicator()),
        ),
      ),
    );
  }
}

class _SummaryError extends StatelessWidget {
  const _SummaryError({required this.onRetry});

  final VoidCallback onRetry;

  @override
  Widget build(BuildContext context) {
    return GlassPanel(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            context.networkSummaryFailure,
            style: Theme.of(context).textTheme.titleMedium,
          ),
          const SizedBox(height: 8),
      Text(
      context.networkSummaryFailureCaption,
      style: Theme.of(context)
        .textTheme
        .bodySmall
        ?.copyWith(
          color:
            Theme.of(context).colorScheme.onSurface.withValues(alpha: 0.7),),
      ),
          const SizedBox(height: 12),
          TextButton.icon(
            onPressed: onRetry,
            icon: const Icon(Icons.refresh),
            label: Text(context.reload),
          ),
        ],
      ),
    );
  }
}

class _SummaryItem {
  const _SummaryItem({
    required this.label,
    required this.value,
    required this.caption,
    required this.color,
    required this.icon,
  });

  final String label;
  final String value;
  final String caption;
  final Color color;
  final IconData icon;
}

class _SummaryCard extends StatelessWidget {
  const _SummaryCard({required this.item});

  final _SummaryItem item;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return GlassPanel(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                CircleAvatar(
                  backgroundColor: item.color.withOpacityPercent(0.18),
                  radius: 20,
                  child: Icon(item.icon, color: item.color, size: 20),
                ),
                const SizedBox(width: 12),
                Text(item.value, style: theme.textTheme.headlineSmall?.copyWith(fontSize: 20)),
              ],
            ),
            const SizedBox(height: 16),
            Text(
              item.label,
      style: theme.textTheme.bodyMedium?.copyWith(
        color: theme.colorScheme.onSurface.withValues(alpha: 0.7),),
      ),
            const Spacer(),
            Text(
              item.caption,
      style: theme.textTheme.bodySmall?.copyWith(
        color: theme.colorScheme.onSurface.withValues(alpha: 0.54),),
      ),
          ],
        ),
      )
          .animate()
          .fadeIn(duration: 340.ms, delay: 60.ms)
          .moveY(begin: 16, end: 0);
  }
}

class _DeviceStreamCard extends ConsumerWidget {
  const _DeviceStreamCard({required this.device});

  final DeviceNode device;

  String _getLocalizedName(BuildContext context, String key) {
    switch (key) {
      case 'headquartersCampus':
        return context.headquartersCampus;
      case 'buildingAMainControl':
        return context.buildingAMainControl;
      case 'buildingAEntrance1':
        return context.buildingAEntrance1;
      case 'buildingAUndergroundGarage':
        return context.buildingAUndergroundGarage;
      case 'buildingBLaboratory':
        return context.buildingBLaboratory;
      case 'labGasSensor':
        return context.labGasSensor;
      case 'southChinaBranch':
        return context.southChinaBranch;
      case 'storageNightVisionCamera':
        return context.storageNightVisionCamera;
      case 'storageTemperatureHumidity':
        return context.storageTemperatureHumidity;
      default:
        return key;
    }
  }

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final statusColor = switch (device.status) {
      DeviceStatus.online => AppColors.successGreen,
      DeviceStatus.offline => AppColors.captureRed,
      DeviceStatus.maintenance => AppColors.warningAmber,
    };
    final theme = Theme.of(context);

    // Generate a gradient based on device properties
    final gradientColors = [
      if (device.status == DeviceStatus.online) 
        const Color(0xFF1A2980) 
      else if (device.status == DeviceStatus.offline)
        const Color(0xFFf12711)
      else
        const Color(0xFFf5af19),
      if (device.status == DeviceStatus.online) 
        const Color(0xFF26D0CE) 
      else if (device.status == DeviceStatus.offline)
        const Color(0xFFf5af19)
      else
        const Color(0xFF203A43),
    ];

    // Generate the correct stream URL for the device
    final deviceId = device.id;
    final streamKey = switch (deviceId) {
      'cam-01' => 'gateway-a/$deviceId-primary',
      'cam-02' => 'garage/$deviceId-fallback',
      'cam-03' => 'night-vision/$deviceId',
      _ => 'stream',
    };
    
    final streamUrl = 'rtmp://127.0.0.1:1935/live/$streamKey';

    return GlassPanel(
      child: Stack(
        children: [
          Positioned.fill(
            child: DecoratedBox(
              decoration: BoxDecoration(
                borderRadius: BorderRadius.circular(24),
                gradient: LinearGradient(
                  colors: gradientColors,
                  begin: Alignment.topLeft,
                  end: Alignment.bottomRight,
                ),
              ),
            ),
          ),
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
                    switch (device.status) {
                      DeviceStatus.online => context.online,
                      DeviceStatus.offline => context.offline,
                      DeviceStatus.maintenance => context.maintenance,
                    },
                    style: theme.textTheme.labelMedium?.copyWith(color: Colors.white),
                  ),
                ],
              ),
            ),
          ),
          Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  _getLocalizedName(context, device.name),
                  style: theme.textTheme.titleLarge
                      ?.copyWith(fontWeight: FontWeight.w700, color: Colors.white),
                ),
                const SizedBox(height: 8),
                Text(
                  device.kind.toString(),
                  style: theme.textTheme.bodySmall
                      ?.copyWith(color: Colors.white70),
                ),
                const SizedBox(height: 8),
                // Display video stream directly
                Expanded(
                  child: ClipRRect(
                    borderRadius: BorderRadius.circular(12),
                    child: SimpleVideoPlayer(
                      streamUrl: streamUrl,
                      aspectRatio: 16/9,
                    ),
                  ),
                ),
                const SizedBox(height: 8),
                Wrap(
                  spacing: 8,
                  runSpacing: 8,
                  children: [
                    _StreamChip(
                      label: device.kind.toString().split('.').last,
                      icon: device.kind.icon,
                    ),
                    if (device.hasMicrophone)
                      _StreamChip(
                        label: context.audio,
                        icon: Icons.mic,
                      ),
                  ],
                ),
                const SizedBox(height: 8),
                SizedBox(
                  width: double.infinity,
                  child: ElevatedButton.icon(
                    onPressed: () {
                      // Create a MonitoringStream from the device data and navigate to stream viewer
                      // Use the correct RTMP URL for the specific device
                      final deviceId = device.id;
                      final streamKey = switch (deviceId) {
                        'cam-01' => 'gateway-a/$deviceId-primary',
                        'cam-02' => 'garage/$deviceId-fallback',
                        'cam-03' => 'night-vision/$deviceId',
                        _ => 'stream',
                      };
                      
                      final stream = MonitoringStream(
                        id: device.id,
                        name: device.name,
                        protocol: 'RTMP', // Using RTMP since server is in primary mode with RTMP forwarding
                        status: device.status == DeviceStatus.online 
                            ? MonitoringStreamStatus.online 
                            : MonitoringStreamStatus.offline,
                        location: '', // Empty for now
                        health: '', // Empty for now
                        gradient: const [
                          Color(0xFF1A2980),
                          Color(0xFF26D0CE),
                        ], // Default gradient
                        url: 'rtmp://127.0.0.1:1935/live/$streamKey', // Point to local MediaMTX server with correct stream key
                        tags: const [],
                      );
                      
                      // Navigate to the stream viewer page
                      Navigator.push(
                        context,
                        MaterialPageRoute(
                          builder: (context) => StreamViewerPage(stream: stream),
                        ),
                      );
                    },
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.white.withOpacityPercent(0.14),
                      foregroundColor: Colors.white,
                      elevation: 0,
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(18),
                      ),
                    ),
                    icon: const Icon(Icons.open_in_full_rounded),
                    label: Text(context.enterFullscreen),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    )
        .animate()
        .fadeIn(duration: 420.ms, delay: 80.ms)
        .scaleXY(begin: 0.98, end: 1);
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
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(
            'Failed to load device list',
            style: Theme.of(context).textTheme.titleMedium,
          ),
          const SizedBox(height: 12),
          FilledButton.icon(
            onPressed: onRetry,
            icon: const Icon(Icons.refresh),
            label: Text(context.retryLoad),
          ),
        ],
      ),
    );
  }
}