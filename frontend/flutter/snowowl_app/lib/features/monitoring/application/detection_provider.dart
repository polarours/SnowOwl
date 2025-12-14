import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:snow_owl/features/monitoring/services/detection_service.dart';
import 'package:snow_owl/features/settings/application/settings_providers.dart';

final detectionServiceProvider = Provider<DetectionService>((ref) {
  final settings = ref.watch(settingsStateProvider);
  final baseUrl = settings.serverConfig.apiUrl.isNotEmpty
      ? settings.serverConfig.apiUrl
      : 'http://localhost:8081';
  return DetectionService(baseUrl: baseUrl);
});

final detectionEnabledProvider = StateNotifierProvider<DetectionController, bool>(
  (ref) => DetectionController(ref),
);

class DetectionController extends StateNotifier<bool> {
  final Ref _ref;
  
  DetectionController(this._ref) : super(false) {
    _loadCurrentStatus();
  }
  
  Future<void> _loadCurrentStatus() async {
    try {
      final service = _ref.read(detectionServiceProvider);
      final status = await service.getDetectionStatus('detection');
      state = status;
    } catch (e) {
      state = false;
    }
  }
  
  Future<void> toggleDetection() async {
    final newState = !state;
    state = newState;
    
    try {
      final service = _ref.read(detectionServiceProvider);
      final success = await service.setDetectionEnabled('detection', newState);
      if (!success) {
        state = !newState;
      }
    } catch (e) {
      state = !newState;
      rethrow;
    }
  }
  
  Future<void> setDetectionEnabled(bool enabled) async {
    state = enabled;
    
    try {
      final service = _ref.read(detectionServiceProvider);
      final success = await service.setDetectionEnabled('detection', enabled);
      if (!success) {
        state = !enabled;
      }
    } catch (e) {
      state = !enabled;
      rethrow;
    }
  }
}