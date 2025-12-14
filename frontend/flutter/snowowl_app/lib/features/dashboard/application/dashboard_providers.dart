import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/features/settings/application/settings_providers.dart';
import 'package:snow_owl/features/devices/application/device_providers.dart';
import 'package:snow_owl/features/dashboard/services/dashboard_service.dart';
import 'package:snow_owl/features/dashboard/domain/dashboard_models.dart';

final dashboardServiceProvider = Provider<DashboardService>((ref) {
  final settings = ref.watch(settingsStateProvider);
  final isConnected = ref.watch(backendConnectionStatusProvider).valueOrNull ?? false;
  
  final baseUrl = settings.serverConfig.apiUrl.isNotEmpty 
      ? settings.serverConfig.apiUrl 
      : 'http://localhost:8081';
  
  // if connected to backend, use API service, else use static service
  return isConnected 
      ? ApiDashboardService(baseUrl: baseUrl)
      : const StaticDashboardService();
});

final dashboardSnapshotProvider = FutureProvider<DashboardSnapshot>((ref) async {
  final service = ref.watch(dashboardServiceProvider);
  return service.fetchSnapshot();
});