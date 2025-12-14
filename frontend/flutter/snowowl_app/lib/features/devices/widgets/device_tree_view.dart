import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/features/devices/domain/device_models.dart';
import 'package:snow_owl/features/devices/controllers/device_tree_controller.dart';
class DeviceTreeView extends ConsumerWidget {
  const DeviceTreeView({
    super.key,
    required this.nodes,
    required this.controller,
    this.onSelected,
    this.onDelete, 
  });

  final List<DeviceNode> nodes;
  final DeviceTreeController controller;
  final ValueChanged<DeviceNode>? onSelected;
  final ValueChanged<String>? onDelete; 

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return _DeviceTree(
      nodes: nodes,
      controller: controller,
      onSelected: onSelected,
      onDelete: onDelete, 
    );
  }
}

class _DeviceTree extends ConsumerWidget {
  const _DeviceTree({
    required this.nodes,
    required this.controller,
    this.onSelected,
    this.onDelete, 
  });

  final List<DeviceNode> nodes;
  final DeviceTreeController controller;
  final ValueChanged<DeviceNode>? onSelected;
  final ValueChanged<String>? onDelete; 

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return ListView.builder(
      itemCount: nodes.length,
      itemBuilder: (context, index) {
        return _DeviceTreeNode(
          node: nodes[index],
          controller: controller,
          onSelected: onSelected,
          onDelete: onDelete, 
        );
      },
    );
  }
}

class _DeviceTreeNode extends ConsumerWidget {
  const _DeviceTreeNode({
    required this.node,
    required this.controller,
    this.onSelected,
    this.onDelete, 
  });

  final DeviceNode node;
  final DeviceTreeController controller;
  final ValueChanged<DeviceNode>? onSelected;
  final ValueChanged<String>? onDelete; 

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isExpanded = controller.isExpanded(node.id);
    final isSelected = controller.isSelected(node.id);
    final hasChildren = node.children.isNotEmpty;

    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        MouseRegion(
          cursor: SystemMouseCursors.click,
          child: GestureDetector(
            onTap: () => onSelected?.call(node),
            child: Container(
              color: isSelected
                  ? colorScheme.primary.withValues(alpha: 0.12)
                  : Colors.transparent,
              padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
              child: Row(
                children: [
                  if (hasChildren) ...[
                    IconButton(
                      icon: Icon(
                        isExpanded
                            ? Icons.expand_more_rounded
                            : Icons.chevron_right_rounded,
                        size: 20,
                      ),
                      onPressed: () => controller.toggle(node.id),
                      padding: EdgeInsets.zero,
                      constraints: const BoxConstraints(),
                    ),
                    const SizedBox(width: 8),
                  ] else ...[
                    const SizedBox(width: 28),
                  ],
                  Icon(node.kind.icon, size: 20),
                  const SizedBox(width: 12),
                  Expanded(
                    child: Text(
                      node.name,
                      style: theme.textTheme.bodyMedium?.copyWith(
                        fontWeight:
                            isSelected ? FontWeight.w600 : FontWeight.normal,
                      ),
                      overflow: TextOverflow.ellipsis,
                    ),
                  ),
                  Container(
                    padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                    decoration: BoxDecoration(
                      color: node.status.color.withValues(alpha: 0.18),
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: Text(
                      node.status.label,
                      style: theme.textTheme.labelSmall?.copyWith(
                        color: node.status.color,
                        fontWeight: FontWeight.w600,
                      ),
                    ),
                  ),
                  if (!hasChildren && onDelete != null) ...[
                    const SizedBox(width: 8),
                    IconButton(
                      icon: const Icon(Icons.delete, size: 18),
                      onPressed: () => onDelete!(node.id),
                      padding: EdgeInsets.zero,
                      constraints: const BoxConstraints(),
                    ),
                  ],
                ],
              ),
            ),
          ),
        ),
        if (hasChildren && isExpanded)
          Padding(
            padding: const EdgeInsets.only(left: 24),
            child: _DeviceTree(
              nodes: node.children,
              controller: controller,
              onSelected: onSelected,
              onDelete: onDelete, 
            ),
          ),
      ],
    );
  }
}
