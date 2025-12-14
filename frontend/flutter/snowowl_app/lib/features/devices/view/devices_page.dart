import 'package:flutter/material.dart';
import 'package:flutter_animate/flutter_animate.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/l10n/l10n.dart';
import 'package:snow_owl/theme/app_colors.dart';
import 'package:snow_owl/widgets/glass_panel.dart';
import 'package:snow_owl/features/devices/application/device_providers.dart';
import 'package:snow_owl/features/devices/domain/device_models.dart';
import 'package:snow_owl/features/devices/controllers/device_tree_controller.dart';
import 'package:snow_owl/features/devices/widgets/device_tree_visualizer.dart';

class DevicesPage extends ConsumerWidget {
  const DevicesPage({super.key});

  static const routePath = '/devices';
  static const routeName = 'devices';
  static const navigationIcon = Icons.devices_other_outlined;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final treeAsync = ref.watch(deviceTreeProvider);
    final connectionStatus = ref.watch(backendConnectionStatusProvider);
    final theme = Theme.of(context);
    final l10n = context;
    final isDark = theme.brightness == Brightness.dark;
    final mutedColor = isDark ? Colors.white70 : Colors.black54;
    final gradientColors = isDark
        ? const [AppColors.backgroundDark, AppColors.backgroundSecondary]
        : const [AppColors.backgroundLight, AppColors.backgroundLightSecondary];
    return Scaffold(
      extendBody: true,
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            colors: gradientColors,
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
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
                    Text(l10n.deviceManagementTitle, style: theme.textTheme.headlineMedium),
                    const Spacer(),
                    connectionStatus.when(
                      data: (connected) => Row(
                        children: [
                          Icon(
                            connected ? Icons.cloud_done : Icons.cloud_off,
                            color: connected ? Colors.green : Colors.red,
                            size: 20,
                          ),
                          const SizedBox(width: 8),
                          Text(
                            connected ? 'Connected' : 'Disconnected',
                            style: theme.textTheme.bodySmall?.copyWith(
                              color: connected ? Colors.green : Colors.red,
                            ),
                          ),
                        ],
                      ),
                      loading: () => const Row(
                        children: [
                          SizedBox(
                            width: 20,
                            height: 20,
                            child: CircularProgressIndicator(strokeWidth: 2),
                          ),
                          SizedBox(width: 8),
                          Text('Checking connection...'),
                        ],
                      ),
                      error: (_, __) => const Row(
                        children: [
                          Icon(Icons.error, color: Colors.red, size: 20),
                          SizedBox(width: 8),
                          Text('Connection error', style: TextStyle(color: Colors.red)),
                        ],
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 10),
                Text(
                  'Visualize and manage all connected devices in a tree structure',
                  style: theme.textTheme.bodyLarge?.copyWith(color: mutedColor),
                ),
                const SizedBox(height: 24),
                Expanded(
                  child: treeAsync.when(
                    data: (nodes) => DeviceTreeVisualizer(devices: nodes),
                    loading: () =>
                        const Center(child: CircularProgressIndicator()),
                    error: (error, stackTrace) => _TreeError(
                      onRetry: () => ref.invalidate(deviceTreeProvider),
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

class _DevicesContent extends ConsumerStatefulWidget {
  const _DevicesContent({required this.nodes});

  final List<DeviceNode> nodes;

  @override
  ConsumerState<_DevicesContent> createState() => _DevicesContentState();
}

class _DevicesContentState extends ConsumerState<_DevicesContent> {
  late DeviceTreeController controller;

  @override
  void initState() {
    super.initState();
    controller = DeviceTreeController();
    Future.microtask(() => controller.ensureInitialized(widget.nodes));
  }

  @override
  void didUpdateWidget(covariant _DevicesContent oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.nodes != widget.nodes) {
      Future.microtask(() => controller.ensureInitialized(widget.nodes));
    }
  }

  @override
  Widget build(BuildContext context) {
    final selectedId = ref.watch(selectedDeviceIdProvider);
    final connectionStatus = ref.watch(backendConnectionStatusProvider);
    final l10n = context;

    return Row(
      children: [
        Expanded(
          flex: 1,
          child: Column(
            children: [
              Padding(
                padding: const EdgeInsets.all(16),
                child: Row(
                  children: [
                    Text(
                      'Device Tree',
                      style: Theme.of(context).textTheme.titleLarge,
                    ),
                    const Spacer(),
                    IconButton(
                      icon: const Icon(Icons.add),
                      onPressed: _showAddDeviceDialog,
                      tooltip: l10n.addDevice,
                    ),
                    IconButton(
                      icon: const Icon(Icons.refresh),
                      onPressed: () => ref.invalidate(deviceTreeProvider),
                      tooltip: l10n.reload,
                    ),
                  ],
                ),
              ),
              connectionStatus.when(
                data: (connected) => connected
                    ? const SizedBox.shrink()
                    : Container(
                        padding: const EdgeInsets.all(16),
                        margin: const EdgeInsets.all(16),
                        decoration: BoxDecoration(
                          color: Colors.red.withValues(alpha: 0.2),
                          borderRadius: BorderRadius.circular(8),
                        ),
                        child: const Text(
                          'Not connected to backend. Showing static data.',
                          style: TextStyle(color: Colors.red),
                        ),
                      ),
                loading: () => const SizedBox.shrink(),
                error: (_, __) => Container(
                  padding: const EdgeInsets.all(16),
                  margin: const EdgeInsets.all(16),
                  decoration: BoxDecoration(
                    color: Colors.red.withValues(alpha: 0.2),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: const Text(
                    'Error connecting to backend. Showing static data.',
                    style: TextStyle(color: Colors.red),
                  ),
                ),
              ),
              Expanded(
                child: DeviceTreeView(
                  nodes: widget.nodes,
                  controller: controller,
                  onSelected: (node) {
                    final container = ProviderScope.containerOf(context);
                    container
                        .read(selectedDeviceControllerProvider.notifier)
                        .select(node.id);
                  },
                  onDelete: _deleteDevice, 
                ),
              ),
            ],
          ),
        ),
        Expanded(
          flex: 2,
          child: selectedId == null
              ? _NoDeviceSelected(l10n: l10n)
              : DeviceDetailsCard(deviceId: selectedId),
        ),
      ],
    );
  }

  void _showAddDeviceDialog() {
    final l10n = context;
    final nameController = TextEditingController();
    final idController = TextEditingController();
    DeviceKind selectedKind = DeviceKind.camera;
    DeviceStatus selectedStatus = DeviceStatus.online;

    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text(l10n.addDevice),
          content: SingleChildScrollView(
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                TextField(
                  controller: idController,
                  decoration: const InputDecoration(
                    labelText: 'Device ID',
                    hintText: 'Enter unique device ID',
                  ),
                ),
                TextField(
                  controller: nameController,
                  decoration: const InputDecoration(
                    labelText: 'Device Name',
                    hintText: 'Enter device name',
                  ),
                ),
                DropdownButtonFormField<DeviceKind>(
                  initialValue: selectedKind,
                  decoration: const InputDecoration(labelText: 'Device Kind'),
                  items: DeviceKind.values.map((kind) {
                    return DropdownMenuItem(
                      value: kind,
                      child: Text(kind.toString().split('.').last),
                    );
                  }).toList(),
                  onChanged: (value) {
                    if (value != null) {
                      selectedKind = value;
                    }
                  },
                ),
                DropdownButtonFormField<DeviceStatus>(
                  initialValue: selectedStatus,
                  decoration: const InputDecoration(labelText: 'Status'),
                  items: DeviceStatus.values.map((status) {
                    return DropdownMenuItem(
                      value: status,
                      child: Text(status.label),
                    );
                  }).toList(),
                  onChanged: (value) {
                    if (value != null) {
                      selectedStatus = value;
                    }
                  },
                ),
              ],
            ),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(),
              child: Text(l10n.cancel),
            ),
            ElevatedButton(
              onPressed: () {
                if (idController.text.isNotEmpty && nameController.text.isNotEmpty) {
                  final newDevice = DeviceNode(
                    id: idController.text,
                    name: nameController.text,
                    kind: selectedKind,
                    status: selectedStatus,
                    registrationTime: DateTime.now(),
                    lastSeen: DateTime.now(),
                  );
                  
                  ref.read(createDeviceProvider(newDevice)).whenData((success) {
                    if (success) {
                      ref.invalidate(deviceTreeProvider);
                      Navigator.of(context).pop();
                      ScaffoldMessenger.of(context).showSnackBar(
                        SnackBar(content: Text(l10n.deviceCreatedSuccessfully)),
                      );
                    } else {
                      ScaffoldMessenger.of(context).showSnackBar(
                        SnackBar(content: Text(l10n.failedToCreateDevice)),
                      );
                    }
                  });
                }
              },
              child: Text(l10n.addDevice),
            ),
          ],
        );
      },
    );
  }

  void _deleteDevice(String deviceId) {
    final l10n = context;
    
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text(l10n.deleteDevice),
          content: Text(l10n.confirmDeleteDevice),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(),
              child: Text(l10n.cancel),
            ),
            ElevatedButton(
              onPressed: () {
                ref.read(deleteDeviceProvider(deviceId)).whenData((success) {
                  if (success) {
                    ref.invalidate(deviceTreeProvider);
                    Navigator.of(context).pop();
                    ScaffoldMessenger.of(context).showSnackBar(
                      SnackBar(content: Text(l10n.deviceDeletedSuccessfully)),
                    );
                  } else {
                    ScaffoldMessenger.of(context).showSnackBar(
                      SnackBar(content: Text(l10n.failedToDeleteDevice)),
                    );
                  }
                });
              },
              child: Text(l10n.deleteDevice),
            ),
          ],
        );
      },
    );
  }
}

class _DeviceEntryWithGroupInfo {
  const _DeviceEntryWithGroupInfo({required this.entry, required this.isGroupNode});

  final _DeviceEntry entry;
  final bool isGroupNode;
}

class DeviceTreeView extends ConsumerWidget {
  const DeviceTreeView({
    super.key,
    required this.nodes,
    required this.controller,
    required this.onSelected,
    required this.onDelete,
  });

  final List<DeviceNode> nodes;
  final DeviceTreeController controller;
  final ValueChanged<DeviceNode> onSelected;
  final ValueChanged<String> onDelete;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final entries = _flattenWithGroupInfo(nodes);
    final theme = Theme.of(context);
    final l10n = context;
    final isDark = theme.brightness == Brightness.dark;
    final iconColor = isDark ? Colors.white70 : Colors.black54;
    final secondaryTextColor = isDark ? Colors.white54 : Colors.black45;
    final selectionFill = isDark
        ? Colors.white.withValues(alpha: 0.12)
        : Colors.black.withValues(alpha: 0.08);

    return GlassPanel(
      padding: const EdgeInsets.all(18),
      child: ListView.separated(
        physics: const BouncingScrollPhysics(),
        itemCount: entries.length,
        separatorBuilder: (_, __) => const SizedBox(height: 8),
        itemBuilder: (context, index) {
          final entryWithGroupInfo = entries[index];
          final entry = entryWithGroupInfo.entry;
          final isGroupNode = entryWithGroupInfo.isGroupNode;
          final isSelected = entry.node.id == controller.selectedId;
          
          String displayName = entry.node.name;
          if (entry.node.id == 'gateways_group') {
            displayName = '⧈ GATEWAYS';
          } else if (entry.node.id == 'encoders_group') {
            displayName = '⧈ ENCODERS';
          } else if (entry.node.id == 'cameras_group') {
            displayName = '⧈ CAMERAS';
          } else if (entry.node.id == 'sensors_group') {
            displayName = '⧈ SENSORS';
          } else if (entry.node.id == 'streams_group') {
            displayName = '⧈ STREAMS';
          } else if (entry.node.id == 'others_group') {
            displayName = '⧈ OTHER DEVICES';
          }
          
          return GestureDetector(
            onTap: isGroupNode ? null : () => onSelected(entry.node),
            child: AnimatedContainer(
              duration: const Duration(milliseconds: 220),
              padding: EdgeInsets.only(
                left: 16 + entry.depth * 18,
                right: 16,
                top: isGroupNode ? 18 : 14,
                bottom: isGroupNode ? 18 : 14,
              ),
              decoration: BoxDecoration(
                borderRadius: BorderRadius.circular(18),
                color: isSelected && !isGroupNode 
                    ? selectionFill 
                    : (isGroupNode 
                        ? (isDark ? Colors.white.withValues(alpha: 0.05) : Colors.black.withValues(alpha: 0.03))
                        : Colors.transparent),
                border: isGroupNode 
                    ? Border.all(
                        color: isDark ? Colors.cyanAccent.shade700.withValues(alpha: 0.3) : Colors.blue.withValues(alpha: 0.3), 
                        width: 1,)
                    : null,
                boxShadow: isGroupNode
                    ? [
                        BoxShadow(
                          color: isDark 
                              ? Colors.cyanAccent.shade700.withValues(alpha: 0.1)
                              : Colors.blue.withValues(alpha: 0.1),
                          blurRadius: 8,
                          offset: const Offset(0, 2),
                        ),
                      ]
                    : null,
              ),
              child: Row(
                children: [
                  Stack(
                    children: [
                      if (isGroupNode)
                        Icon(
                          Icons.hexagon,
                          color: isDark ? Colors.cyanAccent.shade700.withValues(alpha: 0.2) : Colors.blue.withValues(alpha: 0.2),
                          size: isGroupNode ? 24 : 20,
                        ),
                      Icon(
                        entry.node.kind.icon, 
                        color: isGroupNode 
                          ? (isDark ? Colors.cyanAccent.shade700 : Colors.blue) 
                          : iconColor,
                        size: isGroupNode ? 20 : 18,
                      ),
                    ],
                  ),
                  const SizedBox(width: 12),
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Row(
                          children: [
                            if (isGroupNode) ...[
                              Icon(
                                Icons.hexagon_outlined,
                                size: 12,
                                color: isDark ? Colors.cyanAccent.shade700 : Colors.blue,
                              ),
                              const SizedBox(width: 6),
                            ],
                            Text(
                              displayName.startsWith('⧈') 
                                  ? displayName.substring(2)
                                  : displayName,
                              style: theme.textTheme.titleSmall?.copyWith(
                                fontWeight: isGroupNode ? FontWeight.w800 : FontWeight.normal,
                                color: isGroupNode 
                                  ? (isDark ? Colors.cyanAccent.shade100 : Colors.blue) 
                                  : (isDark ? Colors.white70 : Colors.black87),
                                fontSize: isGroupNode ? 16.0 : null,
                                letterSpacing: isGroupNode ? 1.2 : null,
                              ),
                            ),
                          ],
                        ),
                        if (!isGroupNode && (entry.node.registrationTime != null || entry.node.lastSeen != null))
                          Text(
                            _formatDeviceTimeInfo(entry.node, l10n),
                            style: theme.textTheme.bodySmall
                                ?.copyWith(color: secondaryTextColor),
                          )
                        else if (!isGroupNode)
                          Text(
                            entry.node.id,
                            style: theme.textTheme.bodySmall
                                ?.copyWith(color: secondaryTextColor),
                          )
                        else
                          Text(
                            'No devices in this category',
                            style: theme.textTheme.bodySmall?.copyWith(
                              color: isDark ? Colors.white38 : Colors.black38,
                              fontStyle: FontStyle.italic,
                            ),
                          ),
                      ],
                    ),
                  ),
                  if (!isGroupNode)
                    GlassPanel(
                      padding:
                          const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
                      borderRadius: BorderRadius.circular(14),
                      blurSigma: 10,
                      child: Text(
                        _getStatusLabel(entry.node.status, l10n),
                        style: theme.textTheme.labelSmall
                            ?.copyWith(color: entry.node.status.color),
                      ),
                    ),
                  if (!isGroupNode)
                    IconButton(
                      icon: const Icon(Icons.delete),
                      onPressed: () => onDelete(entry.node.id),
                      tooltip: 'Delete Device',
                    ),
                ],
              ),
            ),
          ).animate().fadeIn(
                duration: const Duration(milliseconds: 240),
                delay: Duration(milliseconds: index * 30),
              );
        },
      ),
    );
  }

  String _getStatusLabel(DeviceStatus status, BuildContext l10n) {
    switch (status) {
      case DeviceStatus.online:
        return l10n.online;
      case DeviceStatus.offline:
        return l10n.offline;
      case DeviceStatus.maintenance:
        return l10n.maintenance;
    }
  }

  String _formatDeviceTimeInfo(DeviceNode node, BuildContext l10n) {
    if (node.lastSeen != null) {
      final now = DateTime.now();
      final difference = now.difference(node.lastSeen!);
      
      if (difference.inMinutes < 1) {
        return l10n.justNow;
      } else if (difference.inHours < 1) {
        return l10n.minutesAgo(difference.inMinutes);
      } else if (difference.inDays < 1) {
        return l10n.hoursAgo(difference.inHours);
      } else {
        return l10n.daysAgo(difference.inDays);
      }
    } else if (node.registrationTime != null) {
      final now = DateTime.now();
      final difference = now.difference(node.registrationTime!);
      if (difference.inDays > 0) {
        return l10n.registeredDaysAgo(difference.inDays);
      } else {
        return l10n.recentlyRegistered;
      }
    }
    return node.id;
  }

  List<_DeviceEntryWithGroupInfo> _flattenWithGroupInfo(List<DeviceNode> nodes, [int depth = 0]) {
    final result = <_DeviceEntryWithGroupInfo>[];
    
    // Group devices by type
    final cameras = <DeviceNode>[];
    final encoders = <DeviceNode>[];
    final sensors = <DeviceNode>[];
    final gateways = <DeviceNode>[];
    final streams = <DeviceNode>[]; // RTSP/RTMP streams
    final others = <DeviceNode>[];
    
    // Sort devices into categories
    for (final node in nodes) {
      switch (node.kind) {
        case DeviceKind.camera:
          cameras.add(node);
          break;
        case DeviceKind.encoder:
          encoders.add(node);
          break;
        case DeviceKind.sensor:
          sensors.add(node);
          break;
        case DeviceKind.gateway:
          gateways.add(node);
          break;
        case DeviceKind.rtsp:
        case DeviceKind.rtmp:
        case DeviceKind.file:
          streams.add(node);
          break;
        // default:
        //   others.add(node);
      }
    }
    
    // Add categorized groups (always show categories even if empty)
    result.add(_DeviceEntryWithGroupInfo(
      entry: _DeviceEntry(node: const DeviceNode(
        id: 'cameras_group',
        name: 'Cameras',
        kind: DeviceKind.camera,
        status: DeviceStatus.online,
      ), depth: depth,),
      isGroupNode: true,
    ),);
    
    for (final camera in cameras) {
      result.add(_DeviceEntryWithGroupInfo(
        entry: _DeviceEntry(node: camera, depth: depth + 1),
        isGroupNode: false,
      ),);
    }
    
    result.add(_DeviceEntryWithGroupInfo(
      entry: _DeviceEntry(node: const DeviceNode(
        id: 'encoders_group',
        name: 'Encoders',
        kind: DeviceKind.encoder,
        status: DeviceStatus.online,
      ), depth: depth,),
      isGroupNode: true,
    ),);
    
    for (final encoder in encoders) {
      result.add(_DeviceEntryWithGroupInfo(
        entry: _DeviceEntry(node: encoder, depth: depth + 1),
        isGroupNode: false,
      ),);
    }
    
    result.add(_DeviceEntryWithGroupInfo(
      entry: _DeviceEntry(node: const DeviceNode(
        id: 'sensors_group',
        name: 'Sensors',
        kind: DeviceKind.sensor,
        status: DeviceStatus.online,
      ), depth: depth,),
      isGroupNode: true,
    ),);
    
    for (final sensor in sensors) {
      result.add(_DeviceEntryWithGroupInfo(
        entry: _DeviceEntry(node: sensor, depth: depth + 1),
        isGroupNode: false,
      ),);
    }
    
    result.add(_DeviceEntryWithGroupInfo(
      entry: _DeviceEntry(node: const DeviceNode(
        id: 'gateways_group',
        name: 'Gateways',
        kind: DeviceKind.gateway,
        status: DeviceStatus.online,
      ), depth: depth,),
      isGroupNode: true,
    ),);
    
    for (final gateway in gateways) {
      result.add(_DeviceEntryWithGroupInfo(
        entry: _DeviceEntry(node: gateway, depth: depth + 1),
        isGroupNode: false,
      ),);
    }
    
    result.add(_DeviceEntryWithGroupInfo(
      entry: _DeviceEntry(node: const DeviceNode(
        id: 'streams_group',
        name: 'Streams',
        kind: DeviceKind.rtsp,
        status: DeviceStatus.online,
      ), depth: depth,),
      isGroupNode: true,
    ),);
    
    for (final stream in streams) {
      result.add(_DeviceEntryWithGroupInfo(
        entry: _DeviceEntry(node: stream, depth: depth + 1),
        isGroupNode: false,
      ),);
    }
    
    result.add(_DeviceEntryWithGroupInfo(
      entry: _DeviceEntry(node: const DeviceNode(
        id: 'others_group',
        name: 'Other Devices',
        kind: DeviceKind.camera, // Just pick one as placeholder
        status: DeviceStatus.online,
      ), depth: depth,),
      isGroupNode: true,
    ),);
    
    for (final other in others) {
      result.add(_DeviceEntryWithGroupInfo(
        entry: _DeviceEntry(node: other, depth: depth + 1),
        isGroupNode: false,
      ),);
    }
    
    return result;
  }
}

class _DeviceEntry {
  const _DeviceEntry({required this.node, required this.depth});

  final DeviceNode node;
  final int depth;
}

class DeviceDetailsCard extends ConsumerWidget {
  const DeviceDetailsCard({super.key, required this.deviceId});

  final String deviceId;
  
  void _showAudioControlsDialog(BuildContext context, DeviceNode device) {
    final l10n = context;
    double volume = 0.5;
    bool noiseReduction = false;
    bool echoCancellation = false;

    showDialog(
      context: context,
      builder: (BuildContext context) {
        return StatefulBuilder(
          builder: (context, setState) {
            return AlertDialog(
              title: Text('${_getLocalizedName(context, device.name)} - ${l10n.audio}'),
              content: SingleChildScrollView(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    // Volume control
                    ListTile(
                      title: Text(l10n.volume),
                      trailing: SizedBox(
                        width: 100,
                        child: Slider(
                          value: volume,
                          onChanged: (value) {
                            setState(() {
                              volume = value;
                            });
                          },
                          min: 0.0,
                          max: 1.0,
                          divisions: 100,
                        ),
                      ),
                    ),
                    // Noise reduction toggle
                    SwitchListTile(
                      title: Text(l10n.noiseReduction),
                      value: noiseReduction,
                      onChanged: (value) {
                        setState(() {
                          noiseReduction = value;
                        });
                      },
                    ),
                    // Echo cancellation toggle
                    SwitchListTile(
                      title: Text(l10n.echoCancellation),
                      value: echoCancellation,
                      onChanged: (value) {
                        setState(() {
                          echoCancellation = value;
                        });
                      },
                    ),
                    // Intercom button
                    const SizedBox(height: 16),
                    ElevatedButton.icon(
                      onPressed: () {
                        // Start intercom session
                        ScaffoldMessenger.of(context).showSnackBar(
                          SnackBar(
                            content: Text('${l10n.startingIntercomWith} ${_getLocalizedName(context, device.name)}'),
                            backgroundColor: Colors.green,
                          ),
                        );
                        Navigator.of(context).pop();
                      },
                      icon: const Icon(Icons.voice_chat),
                      label: Text(l10n.startIntercom),
                    ),
                  ],
                ),
              ),
              actions: [
                TextButton(
                  onPressed: () => Navigator.of(context).pop(),
                  child: Text(l10n.cancel),
                ),
                ElevatedButton(
                  onPressed: () {
                    // Apply audio settings
                    ScaffoldMessenger.of(context).showSnackBar(
                      SnackBar(
                        content: Text('${l10n.audioSettingsAppliedTo} ${_getLocalizedName(context, device.name)}'),
                        backgroundColor: Colors.blue,
                      ),
                    );
                    Navigator.of(context).pop();
                  },
                  child: Text(l10n.apply),
                ),
              ],
            );
          },
        );
      },
    );
  }
  
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
      case 'shanghaiHeadquarters':
        return context.shanghaiHeadquarters;
      case 'shanghaiUndergroundParking':
        return context.shanghaiUndergroundParking;
      case 'shenzhenStorage':
        return context.shenzhenStorage;
      default:
        return key;
    }
  }

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final detailsAsync = ref.watch(selectedDeviceDetailsProvider);
    final theme = Theme.of(context);
    final l10n = context;
    final isDark = theme.brightness == Brightness.dark;
    final mutedColor = isDark ? Colors.white70 : Colors.black54;
    final chipColor = isDark ? Colors.white12 : Colors.black12;

    return GlassPanel(
      padding: const EdgeInsets.all(28),
      child: SingleChildScrollView(
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                CircleAvatar(
                  radius: 36,
                  backgroundColor:
                      detailsAsync.value?.device.status.color.withValues(alpha: 0.18),
                  child: Icon(
                    detailsAsync.value?.device.kind.icon,
                    color: detailsAsync.value?.device.status.color,
                    size: 28,
                  ),
                ),
                const SizedBox(width: 18),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        _getLocalizedName(context, detailsAsync.value?.device.name ?? ''),
                        style: theme.textTheme.headlineSmall,
                      ),
                      const SizedBox(height: 6),
                      Text(
                        _getLocalizedName(context, detailsAsync.value?.location ?? ''),
                        style: theme.textTheme.bodySmall
                            ?.copyWith(color: mutedColor),
                      ),
                      if (detailsAsync.value?.device.hasMicrophone == true)
                        Padding(
                          padding: const EdgeInsets.only(top: 4),
                          child: Row(
                            children: [
                              const Icon(Icons.mic, size: 16, color: Colors.green),
                              const SizedBox(width: 4),
                              Text(
                                context.deviceHasMicrophone,
                                style: theme.textTheme.bodySmall?.copyWith(
                                  color: Colors.green,
                                ),
                              ),
                            ],
                          ),
                        ),
                    ],
                  ),
                ),
                FilledButton.icon(
                  onPressed: () {
                    // Navigate to monitoring hub instead of directly showing stream
                    // This aligns with the new architecture where monitoring hub is the central place for streams
                    Navigator.of(context).pushNamed('/monitoring');
                  },
                  icon: const Icon(Icons.video_camera_front),
                  label: Text(l10n.viewStream),
                ),
                // Add audio controls button if device has microphone
                if (detailsAsync.value?.device.hasMicrophone == true)
                  IconButton(
                    icon: const Icon(Icons.audiotrack),
                    onPressed: () {
                      // Show audio controls dialog
                      _showAudioControlsDialog(context, detailsAsync.value!.device);
                    },
                    tooltip: context.audioControls,
                  ),
              ],
            ),
            const SizedBox(height: 24),
            Wrap(
              spacing: 20,
              runSpacing: 16,
              children: [
                _InfoTile(label: l10n.deviceID, value: detailsAsync.value?.device.id ?? ''),
                _InfoTile(label: l10n.ipAddress, value: detailsAsync.value?.ipAddress ?? ''),
                _InfoTile(label: l10n.firmware, value: detailsAsync.value?.firmwareVersion ?? ''),
                _InfoTile(
                  label: l10n.lastHeartbeat,
                  value: detailsAsync.value?.lastHeartbeat != null
                      ? '${detailsAsync.value!.lastHeartbeat.hour.toString().padLeft(2, '0')}:${detailsAsync.value!.lastHeartbeat.minute.toString().padLeft(2, '0')}'
                      : '',
                ),
                _InfoTile(label: l10n.stream, value: detailsAsync.value?.streamEndpoint ?? ''),
                if (detailsAsync.value?.manufacturer.isNotEmpty ?? false)
                  _InfoTile(label: l10n.manufacturer, value: detailsAsync.value?.manufacturer ?? ''),
                if (detailsAsync.value?.model.isNotEmpty ?? false)
                  _InfoTile(label: l10n.model, value: detailsAsync.value?.model ?? ''),
                if (detailsAsync.value?.registrationTime != null)
                  _InfoTile(
                    label: l10n.registrationTime,
                    value: '${detailsAsync.value?.registrationTime!.year}-${detailsAsync.value?.registrationTime!.month.toString().padLeft(2, '0')}-${detailsAsync.value?.registrationTime!.day.toString().padLeft(2, '0')} ${detailsAsync.value?.registrationTime!.hour.toString().padLeft(2, '0')}:${detailsAsync.value?.registrationTime!.minute.toString().padLeft(2, '0')}',
                  ),
                if (detailsAsync.value?.description.isNotEmpty ?? false)
                  _InfoTile(label: l10n.description, value: detailsAsync.value?.description ?? ''),
              ],
            ),
            const SizedBox(height: 24),
            Text(l10n.tags, style: theme.textTheme.titleMedium),
            const SizedBox(height: 8),
            Wrap(
              spacing: 12,
              runSpacing: 12,
              children: detailsAsync.value?.tags
                  .map(
                    (tag) => Chip(
                      label: Text(tag),
                      backgroundColor: chipColor,
                    ),
                  )
                  .toList() ?? [],
            ),
            const SizedBox(height: 24),
            Row(
              children: [
                TextButton.icon(
                  onPressed: () {},
                  style: TextButton.styleFrom(
                    foregroundColor: theme.colorScheme.primary,
                  ),
                  icon: const Icon(Icons.history_toggle_off),
                  label: Text(l10n.history),
                ),
                const SizedBox(width: 12),
                TextButton.icon(
                  onPressed: () {},
                  style: TextButton.styleFrom(
                    foregroundColor: theme.colorScheme.primary,
                  ),
                  icon: const Icon(Icons.upgrade_rounded),
                  label: Text(l10n.pushUpdate),
                ),
                const Spacer(),
                ElevatedButton.icon(
                  onPressed: () {},
                  icon: const Icon(Icons.restart_alt_rounded),
                  label: Text(l10n.reboot),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

class _InfoTile extends StatelessWidget {
  const _InfoTile({required this.label, required this.value});

  final String label;
  final String value;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final isDark = theme.brightness == Brightness.dark;
    final labelColor = isDark ? Colors.white70 : Colors.black54;

    return GlassPanel(
      padding: const EdgeInsets.symmetric(horizontal: 18, vertical: 14),
      borderRadius: BorderRadius.circular(16),
      blurSigma: 12,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(
            label,
            style: theme.textTheme.bodySmall?.copyWith(color: labelColor),
          ),
          const SizedBox(height: 4),
          Text(value, style: theme.textTheme.titleSmall),
        ],
      ),
    );
  }
}

class _NoDeviceSelected extends StatelessWidget {
  const _NoDeviceSelected({required this.l10n});

  final BuildContext l10n;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final isDark = theme.brightness == Brightness.dark;
    final iconColor = isDark ? Colors.white24 : Colors.black26;

    return GlassPanel(
      padding: const EdgeInsets.all(24),
      child: Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.select_all, size: 64, color: iconColor),
            const SizedBox(height: 12),
            Text(l10n.selectDeviceToViewDetails, style: theme.textTheme.titleMedium),
          ],
        ),
      ),
    );
  }
}

class _TreeError extends StatelessWidget {
  const _TreeError({required this.onRetry});

  final VoidCallback onRetry;

  @override
  Widget build(BuildContext context) {
    final l10n = context;
    
    return Center(
      child: GlassPanel(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(l10n.deviceTreeLoadError, style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 12),
            FilledButton.icon(
              onPressed: onRetry,
              icon: const Icon(Icons.refresh),
              label: Text(l10n.retry),
            ),
          ],
        ),
      ),
    );
  }
}
