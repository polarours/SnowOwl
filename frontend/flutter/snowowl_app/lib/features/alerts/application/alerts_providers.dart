import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/features/settings/application/settings_providers.dart';
import 'package:snow_owl/features/devices/application/device_providers.dart';
import 'package:snow_owl/features/alerts/services/alerts_service.dart';
import 'package:snow_owl/features/alerts/domain/alert_models.dart';

final alertsServiceProvider = Provider<AlertsService>((ref) {
  final settings = ref.watch(settingsStateProvider);
  final isConnected = ref.watch(backendConnectionStatusProvider).valueOrNull ?? false;
  
  final baseUrl = settings.serverConfig.apiUrl.isNotEmpty 
      ? settings.serverConfig.apiUrl 
      : 'http://localhost:8081';
  
  // if connected to backend, use API service, else use static service
  return isConnected 
      ? ApiAlertsService(baseUrl: baseUrl)
      : const StaticAlertsService();
});

final recentAlertsProvider = FutureProvider<List<AlertEvent>>((ref) async {
  final service = ref.watch(alertsServiceProvider);
  return service.fetchRecentAlerts();
});