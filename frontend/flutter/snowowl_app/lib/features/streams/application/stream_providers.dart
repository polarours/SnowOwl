import 'package:flutter/foundation.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:riverpod/riverpod.dart';

import 'package:snow_owl/features/streams/domain/stream_models.dart';
import 'package:snow_owl/features/streams/services/stream_api_service.dart';
import 'package:snow_owl/features/settings/application/settings_providers.dart';

// API Service Provider
final streamApiServiceProvider = Provider<StreamApiService>((ref) {
  final settings = ref.watch(settingsStateProvider);
  final isDatabaseConnected = settings.serverConfig.isConnected;
  
  if (isDatabaseConnected) {
    const baseUrl = kDebugMode 
        ? 'http://localhost:8081' 
        : 'http://localhost:8081';
    return StreamApiService(baseUrl: baseUrl);
  } else {
    // Return a service that will provide mock data
    return StreamApiService(baseUrl: 'http://localhost:8081'); // baseUrl not used for mock data
  }
});

// Streams State Provider
final streamsProvider = FutureProvider<List<VideoStream>>((ref) async {
  final settings = ref.watch(settingsStateProvider);
  final isDatabaseConnected = settings.serverConfig.isConnected;
  final apiService = ref.watch(streamApiServiceProvider);
  
  if (isDatabaseConnected) {
    // Fetch real data from API
    return apiService.fetchStreams();
  } else {
    // Return mock data
    return apiService.fetchStreams();
  }
});

// Selected Stream Provider
final selectedStreamProvider = StateProvider<VideoStream?>((ref) => null);

// Stream Detection Config Provider
final streamDetectionConfigProvider = StateNotifierProvider<StreamDetectionConfigNotifier, StreamDetectionConfig>(
  (ref) => StreamDetectionConfigNotifier(),
);

class StreamDetectionConfigNotifier extends StateNotifier<StreamDetectionConfig> {
  StreamDetectionConfigNotifier() : super(StreamDetectionConfig());

  void updateConfig(StreamDetectionConfig newConfig) {
    state = newConfig;
  }

  void toggleMotionDetection() {
    state = state.copyWith(motionDetection: !state.motionDetection);
  }

  void toggleIntrusionDetection() {
    state = state.copyWith(intrusionDetection: !state.intrusionDetection);
  }

  void toggleFireDetection() {
    state = state.copyWith(fireDetection: !state.fireDetection);
  }

  void toggleGasLeakDetection() {
    state = state.copyWith(gasLeakDetection: !state.gasLeakDetection);
  }

  void toggleEquipmentDetection() {
    state = state.copyWith(equipmentDetection: !state.equipmentDetection);
  }
}

// Add Stream Provider
final addStreamProvider = FutureProvider.family<VideoStream, VideoStream>((ref, stream) async {
  final apiService = ref.watch(streamApiServiceProvider);
  return apiService.addStream(stream);
});

// Update Stream Provider
final updateStreamProvider = FutureProvider.family<void, VideoStream>((ref, stream) async {
  final apiService = ref.watch(streamApiServiceProvider);
  return apiService.updateStream(stream);
});

// Delete Stream Provider
final deleteStreamProvider = FutureProvider.family<void, String>((ref, streamId) async {
  final apiService = ref.watch(streamApiServiceProvider);
  return apiService.deleteStream(streamId);
});