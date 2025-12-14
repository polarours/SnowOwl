import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import 'package:snow_owl/theme/app_colors.dart';
import 'package:snow_owl/widgets/glass_panel.dart';
import 'package:snow_owl/features/dashboard/application/dashboard_edit_providers.dart';
import 'package:snow_owl/features/dashboard/domain/editable_elements.dart';
import 'package:snow_owl/features/devices/domain/device_models.dart';
import 'package:snow_owl/l10n/l10n.dart';

class SceneEditorPage extends ConsumerStatefulWidget {
  const SceneEditorPage({super.key});

  static const routePath = '/scene-editor';
  static const routeName = 'sceneEditor';
  static const navigationIcon = Icons.account_tree_outlined;

  @override
  ConsumerState<SceneEditorPage> createState() => _SceneEditorPageState();
}

class _SceneEditorPageState extends ConsumerState<SceneEditorPage> {
  late DashboardZone _currentZone;
  Offset? _dragPosition;

  @override
  void initState() {
    super.initState();
    // Initialize with a default zone for the scene
    _currentZone = DashboardZone(
      id: 'scene_${DateTime.now().millisecondsSinceEpoch}',
      name: 'Warehouse Scene',
      description: 'Virtual warehouse layout',
    );
  }

  @override
  Widget build(BuildContext context) {
    final editState = ref.watch(dashboardEditStateProvider);
    
    return Scaffold(
      appBar: AppBar(
        title: Text(context.sceneEditor),
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: () => context.pop(),
        ),
        actions: [
          IconButton(
            icon: const Icon(Icons.image),
            onPressed: _selectBackgroundImage,
          ),
          IconButton(
            icon: const Icon(Icons.save),
            onPressed: _saveScene,
          ),
        ],
      ),
      body: Column(
        children: [
          _buildToolbar(),
          Expanded(
            child: Row(
              children: [
                // Sidebar with device palette
                _buildDevicePalette(),
                // Main scene editor area
                Expanded(
                  child: _buildSceneEditor(editState),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildToolbar() {
    return GlassPanel(
      child: Padding(
        padding: const EdgeInsets.all(8.0),
        child: Row(
          children: [
            IconButton(
              icon: const Icon(Icons.wallpaper),
              onPressed: _addWall,
            ),
            IconButton(
              icon: const Icon(Icons.square_foot),
              onPressed: _addArea,
            ),
            IconButton(
              icon: const Icon(Icons.delete),
              onPressed: _clearSelection,
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildDevicePalette() {
    return GlassPanel(
      child: Padding(
        padding: const EdgeInsets.all(8.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              context.devices,
              style: Theme.of(context).textTheme.titleMedium,
            ),
            Expanded(
              child: ListView(
                children: [
                  _DraggableDeviceItem(
                    deviceType: ElementType.camera,
                    icon: Icons.videocam_outlined,
                    label: context.cameras,
                  ),
                  _DraggableDeviceItem(
                    deviceType: ElementType.sensor,
                    icon: Icons.sensors,
                    label: context.sensors,
                  ),
                  // Add more device types as needed
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildSceneEditor(DashboardEditState editState) {
    return GlassPanel(
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: DragTarget<ElementTypes>(
          onAcceptWithDetails: (details) {
            setState(() {
              _dragPosition = null;
            });
            if (_dragPosition != null) {
              _addElement(details.data.type, _dragPosition!);
            }
          },
          onLeave: (data) {
            setState(() {
              _dragPosition = null;
            });
          },
          onWillAcceptWithDetails: (details) {
            return true;
          },
          builder: (context, candidateItems, rejectedItems) {
            return Stack(
              children: [
                // Background image or grid
                if (_currentZone.backgroundImage != null)
                  Image.asset(
                    _currentZone.backgroundImage!,
                    fit: BoxFit.cover,
                    width: double.infinity,
                    height: double.infinity,
                  )
                else
                  CustomPaint(
                    size: const Size(double.infinity, double.infinity),
                    painter: _GridPainter(),
                  ),
                
                // Existing elements in the zone
                for (final element in _currentZone.elements)
                  _SceneElement(
                    element: element,
                    onSelected: () => _selectElement(element),
                    onDelete: () => _deleteElement(element),
                    onEdit: () => _editElement(element),
                  ),
                
                // Drop target indicator
                if (_dragPosition != null)
                  Positioned(
                    left: _dragPosition!.dx - 25,
                    top: _dragPosition!.dy - 25,
                    child: const Icon(
                      Icons.add_circle,
                      color: AppColors.successGreen,
                      size: 50,
                    ),
                  ),
              ],
            );
          },
        ),
      ),
    );
  }

  void _saveScene() {
    // Save the current zone to the dashboard edit state
    ref.read(dashboardEditStateProvider.notifier).addZone(_currentZone);
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text(context.sceneSavedSuccessfully)),
    );
    context.pop();
  }

  void _selectBackgroundImage() {
    // In a real app, this would open a file picker
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Background image selector not implemented')),
    );
  }

  void _addWall() {
    // Placeholder for adding walls/obstacles to the scene
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Wall tool activated')),
    );
  }

  void _addArea() {
    // Placeholder for adding areas/regions to the scene
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Area tool activated')),
    );
  }

  void _clearSelection() {
    // Placeholder for clearing selections
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Selection cleared')),
    );
  }

  void _selectElement(DashboardElement element) {
    // Placeholder for element selection
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Selected: ${element.name}')),
    );
  }

  void _editElement(DashboardElement element) {
    // Show dialog to edit element properties
    _showElementEditorDialog(element);
  }

  void _deleteElement(DashboardElement element) {
    setState(() {
      _currentZone = _currentZone.copyWith(
        elements: _currentZone.elements.where((e) => e.id != element.id).toList(),
      );
    });
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Deleted: ${element.name}')),
    );
  }

  void _addElement(ElementType type, Offset position) {
    final elementId = '${type.name}_${DateTime.now().millisecondsSinceEpoch}';
    
    DashboardElement element;
    switch (type) {
      case ElementType.camera:
        element = CameraElement(
          id: elementId,
          name: 'Camera ${_currentZone.elements.length + 1}',
          position: position,
          size: const Size(60, 60),
          deviceId: '', // Will be linked to real device later
        );
        break;
      case ElementType.sensor:
        element = SensorElement(
          id: elementId,
          name: 'Sensor ${_currentZone.elements.length + 1}',
          position: position,
          size: const Size(40, 40),
          deviceId: '', // Will be linked to real device later
        );
        break;
      default:
        return;
    }

    setState(() {
      _currentZone = _currentZone.copyWith(
        elements: [..._currentZone.elements, element],
      );
    });
    
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Added: ${element.name}')),
    );
  }

  void _showElementEditorDialog(DashboardElement element) {
    final nameController = TextEditingController(text: element.name);
    
    String? deviceId = element.deviceId;
    String? streamUrl;
    
    if (element is CameraElement) {
      streamUrl = element.streamUrl;
    }
    
    // Mock list of available devices for demonstration
    // In a real implementation, this would come from a provider/service
    final List<Map<String, dynamic>> availableDevices = [
      {'id': 'cam_001', 'name': 'Front Door Camera', 'type': ElementType.camera},
      {'id': 'cam_002', 'name': 'Backyard Camera', 'type': ElementType.camera},
      {'id': 'sensor_001', 'name': 'Temperature Sensor', 'type': ElementType.sensor},
      {'id': 'sensor_002', 'name': 'Motion Sensor', 'type': ElementType.sensor},
    ];
    
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return StatefulBuilder(
          builder: (context, setState) {
            return AlertDialog(
              title: Text('Edit ${element.name}'),
              content: SingleChildScrollView(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    TextField(
                      controller: nameController,
                      decoration: InputDecoration(labelText: 'Name'),
                    ),
                    const SizedBox(height: 16),
                    // Device selection dropdown
                    DropdownButtonFormField<String>(
                      value: deviceId,
                      hint: Text('Select Real Device'),
                      items: [
                        const DropdownMenuItem(
                          value: null,
                          child: Text('None'),
                        ),
                        ...availableDevices.map((device) {
                          return DropdownMenuItem(
                            value: device['id'] as String,
                            child: Text(device['name'] as String),
                          );
                        }).toList(),
                      ],
                      onChanged: (value) {
                        setState(() {
                          deviceId = value;
                        });
                      },
                      decoration: InputDecoration(
                        labelText: 'Real Device',
                        border: const OutlineInputBorder(),
                      ),
                    ),
                    if (element is CameraElement) ...[
                      const SizedBox(height: 16),
                      TextField(
                        controller: TextEditingController(text: streamUrl),
                        onChanged: (value) => streamUrl = value,
                        decoration: InputDecoration(labelText: context.streamUrl),
                      ),
                    ],
                    const SizedBox(height: 16),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.end,
                      children: [
                        TextButton(
                          onPressed: () => Navigator.of(context).pop(),
                          child: Text(context.cancel),
                        ),
                        const SizedBox(width: 8),
                        ElevatedButton(
                          onPressed: () {
                            DashboardElement updatedElement;
                            
                            if (element is CameraElement) {
                              updatedElement = element.copyWith(
                                name: nameController.text,
                                deviceId: deviceId,
                                streamUrl: streamUrl,
                              );
                            } else if (element is SensorElement) {
                              updatedElement = element.copyWith(
                                name: nameController.text,
                                deviceId: deviceId,
                              );
                            } else {
                              updatedElement = element.copyWith(
                                name: nameController.text,
                                deviceId: deviceId,
                              );
                            }
                            
                            // Update the element in the zone
                            setState(() {
                              _currentZone = _currentZone.copyWith(
                                elements: _currentZone.elements.map((e) {
                                  return e.id == element.id ? updatedElement : e;
                                }).toList(),
                              );
                            });
                            
                            Navigator.of(context).pop();
                          },
                          child: Text(context.save),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            );
          },
        );
      },
    );
  }
}

class _DraggableDeviceItem extends StatelessWidget {
  final ElementType deviceType;
  final IconData icon;
  final String label;

  const _DraggableDeviceItem({
    required this.deviceType,
    required this.icon,
    required this.label,
  });

  @override
  Widget build(BuildContext context) {
    return GlassPanel(
      child: Padding(
        padding: const EdgeInsets.all(4.0),
        child: Draggable<ElementTypes>(
          data: ElementTypes(type: deviceType),
          feedback: Material(
            color: Colors.transparent,
            child: GlassPanel(
              child: Padding(
                padding: const EdgeInsets.all(8.0),
                child: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(icon, color: Colors.white),
                    const SizedBox(width: 8),
                    Text(label, style: const TextStyle(color: Colors.white)),
                  ],
                ),
              ),
            ),
          ),
          childWhenDragging: Opacity(
            opacity: 0.5,
            child: ListTile(
              leading: Icon(icon, color: Colors.white),
              title: Text(label, style: const TextStyle(color: Colors.white)),
            ),
          ),
          child: ListTile(
            leading: Icon(icon, color: Colors.white),
            title: Text(label, style: const TextStyle(color: Colors.white)),
          ),
        ),
      ),
    );
  }
}

class ElementTypes {
  final ElementType type;
  
  ElementTypes({required this.type});
}

class _SceneElement extends StatelessWidget {
  final DashboardElement element;
  final VoidCallback onSelected;
  final VoidCallback onDelete;
  final VoidCallback onEdit;

  const _SceneElement({
    required this.element,
    required this.onSelected,
    required this.onDelete,
    required this.onEdit,
  });

  @override
  Widget build(BuildContext context) {
    return Positioned(
      left: element.position.dx,
      top: element.position.dy,
      child: GestureDetector(
        onTap: onSelected,
        onDoubleTap: onDelete,
        onLongPress: onEdit,
        child: Container(
          width: element.size.width,
          height: element.size.height,
          decoration: BoxDecoration(
            color: _getElementColor(element.type),
            borderRadius: BorderRadius.circular(8),
            border: Border.all(color: Colors.white70, width: 2),
          ),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(_getElementIcon(element.type), color: Colors.white, size: 20),
              const SizedBox(height: 4),
              Text(
                element.name,
                style: const TextStyle(color: Colors.white, fontSize: 8),
                textAlign: TextAlign.center,
              ),
              if (element.deviceId != null && element.deviceId!.isNotEmpty)
                const Icon(Icons.link, size: 12, color: AppColors.successGreen)
            ],
          ),
        ),
      ),
    );
  }

  Color _getElementColor(ElementType type) {
    switch (type) {
      case ElementType.camera:
        return AppColors.captureRed;
      case ElementType.sensor:
        return AppColors.warningAmber;
      case ElementType.zone:
        return AppColors.monitoringBlue;
    }
  }

  IconData _getElementIcon(ElementType type) {
    switch (type) {
      case ElementType.camera:
        return Icons.videocam;
      case ElementType.sensor:
        return Icons.sensors;
      case ElementType.zone:
        return Icons.square_foot;
    }
  }
}

class _GridPainter extends CustomPainter {
  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = Colors.white10
      ..strokeWidth = 1;

    // Draw grid lines
    const gridSize = 20.0;
    
    // Vertical lines
    for (double x = 0; x <= size.width; x += gridSize) {
      canvas.drawLine(Offset(x, 0), Offset(x, size.height), paint);
    }
    
    // Horizontal lines
    for (double y = 0; y <= size.height; y += gridSize) {
      canvas.drawLine(Offset(0, y), Offset(size.width, y), paint);
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}
