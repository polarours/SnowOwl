import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/features/settings/application/settings_providers.dart';
import 'package:snow_owl/features/monitoring/services/monitoring_service.dart';
import 'package:snow_owl/features/devices/application/device_providers.dart';
import 'package:snow_owl/features/monitoring/domain/monitoring_models.dart';

final monitoringServiceProvider = Provider<MonitoringService>((ref) {
  final settings = ref.watch(settingsStateProvider);
  final isConnected = ref.watch(backendConnectionStatusProvider).valueOrNull ?? false;
  
  final baseUrl = settings.serverConfig.apiUrl.isNotEmpty 
      ? settings.serverConfig.apiUrl 
      : 'http://localhost:8081';
  
  // if connected to backend, use API service, else use static service
  return isConnected 
      ? ApiMonitoringService(baseUrl: baseUrl)
      : const StaticMonitoringService();
});

final monitoringStreamsProvider = FutureProvider<List<MonitoringStream>>((ref) async {
  final service = ref.watch(monitoringServiceProvider);
  return service.fetchStreams();
});

final monitoringSummaryProvider = FutureProvider<MonitoringSessionSummary>((ref) async {
  final service = ref.watch(monitoringServiceProvider);
  return service.fetchSummary();
});