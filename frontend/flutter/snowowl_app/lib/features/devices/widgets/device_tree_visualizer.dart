import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/features/devices/domain/device_models.dart';
import 'package:snow_owl/theme/app_colors.dart';
import 'package:snow_owl/widgets/glass_panel.dart';
import 'package:snow_owl/l10n/l10n.dart';

class DeviceTreeVisualizer extends ConsumerStatefulWidget {
  const DeviceTreeVisualizer({super.key, required this.devices});

  final List<DeviceNode> devices;

  @override
  ConsumerState<DeviceTreeVisualizer> createState() => _DeviceTreeVisualizerState();
}

class _DeviceTreeVisualizerState extends ConsumerState<DeviceTreeVisualizer> {
  DeviceNode? _selectedDevice;
  final Set<DeviceNode> _expandedNodes = {};
  final Map<DeviceKind, bool> _kindFilters = {
    DeviceKind.camera: true,
    DeviceKind.encoder: true,
    DeviceKind.sensor: true,
    DeviceKind.gateway: true,
    DeviceKind.rtsp: true,
    DeviceKind.rtmp: true,
    DeviceKind.file: true,
  };

  @override
  void initState() {
    super.initState();
    _expandAllNodes(widget.devices);
  }

  void _expandAllNodes(List<DeviceNode> nodes) {
    for (final node in nodes) {
      _expandedNodes.add(node);
      if (node.children.isNotEmpty) {
        _expandAllNodes(node.children);
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context); // ignore: unused_local_variable
    final isDark = theme.brightness == Brightness.dark; // ignore: unused_local_variable // ignore: unused_local_variable

    return GlassPanel(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                context.devices,
                style: theme.textTheme.titleLarge,
              ),
              Row(
                children: [
                  IconButton(
                    icon: const Icon(Icons.filter_list),
                    onPressed: () => _showFilterDialog(context),
                    tooltip: context.filterDevices,
                  ),
                  IconButton(
                    icon: const Icon(Icons.expand_more),
                    onPressed: () {
                      setState(() {
                        _expandAllNodes(widget.devices);
                      });
                    },
                    tooltip: context.expandAll,
                  ),
                  IconButton(
                    icon: const Icon(Icons.expand_less),
                    onPressed: () {
                      setState(() {
                        _expandedNodes.clear();
                      });
                    },
                    tooltip: context.collapseAll,
                  ),
                ],
              ),
            ],
          ),
          const SizedBox(height: 16),
          Expanded(
            child: SingleChildScrollView(
              child: _buildFilteredTree(widget.devices, 0),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildFilteredTree(List<DeviceNode> nodes, int depth) {
    final filteredNodes = nodes.where((node) => _kindFilters[node.kind] ?? true).toList();
    
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        for (final node in filteredNodes)
          _buildTreeNode(node, depth),
      ],
    );
  }

  Widget _buildTreeNode(DeviceNode node, int depth) {
    final isExpanded = _expandedNodes.contains(node);
    final isSelected = _selectedDevice == node;
    final hasChildren = node.children.isNotEmpty;
    final theme = Theme.of(context); // ignore: unused_local_variable
    final isDark = theme.brightness == Brightness.dark; // ignore: unused_local_variable // ignore: unused_local_variable

    final filteredChildren = node.children.where((child) => _kindFilters[child.kind] ?? true).toList();

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        GestureDetector(
          onTap: () {
            setState(() {
              if (isSelected) {
                _selectedDevice = null;
              } else {
                _selectedDevice = node;
              }
              
              if (hasChildren) {
                if (isExpanded) {
                  _expandedNodes.remove(node);
                } else {
                  _expandedNodes.add(node);
                }
              }
            });
          },
          child: Container(
            margin: const EdgeInsets.symmetric(vertical: 4),
            padding: EdgeInsets.only(left: depth * 20.0, top: 8, bottom: 8, right: 16),
            decoration: BoxDecoration(
              color: isSelected 
                  ? (isDark ? AppColors.monitoringBlue.withValues(alpha: 0.3) : AppColors.monitoringBlue.withValues(alpha: 0.2))
                  : Colors.transparent,
              borderRadius: BorderRadius.circular(12),
              border: isSelected 
                  ? Border.all(color: AppColors.monitoringBlue, width: 1)
                  : null,
            ),
            child: Row(
              children: [
                if (hasChildren) 
                  IconButton(
                    icon: Icon(
                      isExpanded ? Icons.expand_more : Icons.chevron_right,
                      size: 20,
                      color: isDark ? Colors.white70 : Colors.black54,
                    ),
                    onPressed: () {
                      setState(() {
                        if (isExpanded) {
                          _expandedNodes.remove(node);
                        } else {
                          _expandedNodes.add(node);
                        }
                      });
                    },
                    padding: EdgeInsets.zero,
                    constraints: const BoxConstraints(),
                  )
                else
                  const SizedBox(width: 20),
                const SizedBox(width: 8),
                Container(
                  padding: const EdgeInsets.all(6),
                  decoration: BoxDecoration(
                    color: _getKindColor(node.kind).withValues(alpha: 0.1),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Icon(
                    node.kind.icon,
                    size: 20,
                    color: _getKindColor(node.kind),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        _getLocalizedName(context, node.name),
                        style: theme.textTheme.titleMedium?.copyWith(
                          fontWeight: isSelected ? FontWeight.bold : FontWeight.normal,
                          color: isSelected ? AppColors.monitoringBlue : null,
                        ),
                      ),
                      Text(
                        '${_getLocalizedKind(context, node.kind)} • ${_getLocalizedStatus(context, node.status)}',
                        style: theme.textTheme.bodySmall?.copyWith(
                          color: isDark ? Colors.white70 : Colors.black54,
                        ),
                      ),
                    ],
                  ),
                ),
                if (node.hasMicrophone)
                  const Icon(Icons.mic, size: 16, color: Colors.green),
                const SizedBox(width: 8),
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                  decoration: BoxDecoration(
                    color: _getStatusColor(node.status).withValues(alpha: 0.2),
                    borderRadius: BorderRadius.circular(12),
                  ),
                  child: Text(
                    _getLocalizedStatus(context, node.status),
                    style: theme.textTheme.bodySmall?.copyWith(
                      color: _getStatusColor(node.status),
                      fontWeight: FontWeight.w500,
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
        if (isExpanded && filteredChildren.isNotEmpty)
          Padding(
            padding: const EdgeInsets.only(left: 20),
            child: _buildFilteredTree(filteredChildren, depth + 1),
          ),
      ],
    );
  }

  Color _getStatusColor(DeviceStatus status) {
    switch (status) {
      case DeviceStatus.online:
        return AppColors.successGreen;
      case DeviceStatus.offline:
        return AppColors.captureRed;
      case DeviceStatus.maintenance:
        return AppColors.warningAmber;
    }
  }
  
  Color _getKindColor(DeviceKind kind) {
    switch (kind) {
      case DeviceKind.camera:
        return Colors.blue;
      case DeviceKind.encoder:
        return Colors.purple;
      case DeviceKind.sensor:
        return Colors.green;
      case DeviceKind.gateway:
        return Colors.orange;
      case DeviceKind.rtsp:
      case DeviceKind.rtmp:
      case DeviceKind.file:
        return Colors.teal;
    }
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
      default:
        return key;
    }
  }
  
  String _getLocalizedKind(BuildContext context, DeviceKind kind) {
    switch (kind) {
      case DeviceKind.camera:
        return context.cameras;
      case DeviceKind.encoder:
        return context.encoders;
      case DeviceKind.sensor:
        return context.sensors;
      case DeviceKind.gateway:
        return context.gateways;
      case DeviceKind.rtsp:
        return 'RTSP';
      case DeviceKind.rtmp:
        return 'RTMP';
      case DeviceKind.file:
        return context.file;
    }
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
  
  void _showFilterDialog(BuildContext context) {
    final theme = Theme.of(context); // ignore: unused_local_variable
    
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return StatefulBuilder(
          builder: (context, setState) {
            return AlertDialog(
              title: Text(context.filterDevices),
              content: SizedBox(
                width: 300,
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    for (final kind in DeviceKind.values)
                      CheckboxListTile(
                        title: Text(_getLocalizedKind(context, kind)),
                        value: _kindFilters[kind] ?? true,
                        onChanged: (bool? value) {
                          setState(() {
                            _kindFilters[kind] = value ?? true;
                          });
                        },
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
                TextButton(
                  onPressed: () {
                    setState(() {
                      for (final kind in DeviceKind.values) {
                        _kindFilters[kind] = true;
                      }
                    });
                  },
                  child: Text(context.reset),
                ),
                ElevatedButton(
                  onPressed: () {
                    Navigator.of(context).pop();
                  },
                  child: Text(context.apply),
                ),
              ],
            );
          },
        );
      },
    );
  }
}