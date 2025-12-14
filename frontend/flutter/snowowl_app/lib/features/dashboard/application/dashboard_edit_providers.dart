import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:snow_owl/features/dashboard/domain/editable_elements.dart';

/// State for dashboard editing mode
class DashboardEditState {
  final bool isEditing;
  final List<DashboardZone> zones;
  final DashboardZone? selectedZone;

  DashboardEditState({
    required this.isEditing,
    required this.zones,
    this.selectedZone,
  });

  DashboardEditState copyWith({
    bool? isEditing,
    List<DashboardZone>? zones,
    DashboardZone? selectedZone,
  }) {
    return DashboardEditState(
      isEditing: isEditing ?? this.isEditing,
      zones: zones ?? this.zones,
      selectedZone: selectedZone ?? this.selectedZone,
    );
  }
}

/// Provider for dashboard editing state
final dashboardEditStateProvider = StateNotifierProvider<DashboardEditNotifier, DashboardEditState>(
  (ref) => DashboardEditNotifier(),
);

class DashboardEditNotifier extends StateNotifier<DashboardEditState> {
  DashboardEditNotifier()
      : super(DashboardEditState(
          isEditing: false,
          zones: [],
        ));

  /// Toggle editing mode
  void toggleEditMode() {
    state = state.copyWith(isEditing: !state.isEditing);
  }

  /// Enable editing mode
  void enableEditMode() {
    state = state.copyWith(isEditing: true);
  }

  /// Disable editing mode
  void disableEditMode() {
    state = state.copyWith(isEditing: false);
  }

  /// Add a new zone
  void addZone(DashboardZone zone) {
    final newZones = [...state.zones, zone];
    state = state.copyWith(zones: newZones);
  }

  /// Update a zone
  void updateZone(DashboardZone updatedZone) {
    final newZones = state.zones.map((zone) {
      return zone.id == updatedZone.id ? updatedZone : zone;
    }).toList();
    state = state.copyWith(zones: newZones);
  }

  /// Remove a zone
  void removeZone(String zoneId) {
    final newZones = state.zones.where((zone) => zone.id != zoneId).toList();
    state = state.copyWith(zones: newZones);
  }

  /// Select a zone
  void selectZone(DashboardZone zone) {
    state = state.copyWith(selectedZone: zone);
  }

  /// Deselect current zone
  void deselectZone() {
    state = state.copyWith(selectedZone: null);
  }

  /// Add an element to a zone
  void addElementToZone(String zoneId, DashboardElement element) {
    final updatedZones = state.zones.map((zone) {
      if (zone.id == zoneId) {
        final newElements = [...zone.elements, element];
        return zone.copyWith(elements: newElements);
      }
      return zone;
    }).toList();

    state = state.copyWith(zones: updatedZones);

    // Update selected zone if it was the one modified
    if (state.selectedZone?.id == zoneId) {
      final updatedZone = updatedZones.firstWhere((zone) => zone.id == zoneId);
      state = state.copyWith(selectedZone: updatedZone);
    }
  }

  /// Remove an element from a zone
  void removeElementFromZone(String zoneId, String elementId) {
    final updatedZones = state.zones.map((zone) {
      if (zone.id == zoneId) {
        final newElements = zone.elements.where((element) => element.id != elementId).toList();
        return zone.copyWith(elements: newElements);
      }
      return zone;
    }).toList();

    state = state.copyWith(zones: updatedZones);

    // Update selected zone if it was the one modified
    if (state.selectedZone?.id == zoneId) {
      final updatedZone = updatedZones.firstWhere((zone) => zone.id == zoneId);
      state = state.copyWith(selectedZone: updatedZone);
    }
  }
}