import 'package:flutter/foundation.dart';

import 'package:snow_owl/features/devices/domain/device_models.dart';

class DeviceTreeController extends ChangeNotifier {
  final Set<String> _expandedNodes = {};
  String? _selectedId;

  bool isExpanded(String nodeId) => _expandedNodes.contains(nodeId);

  bool isSelected(String nodeId) => _selectedId == nodeId;

  void toggle(String nodeId) {
    if (_expandedNodes.contains(nodeId)) {
      _expandedNodes.remove(nodeId);
    } else {
      _expandedNodes.add(nodeId);
    }
    notifyListeners();
  }

  void select(String nodeId) {
    _selectedId = nodeId;
    notifyListeners();
  }

  String? get selectedId => _selectedId;

  void ensureInitialized(List<DeviceNode> nodes) {
    notifyListeners();
  }
}
