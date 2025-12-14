import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:http/http.dart' as http;

import 'package:snow_owl/features/devices/application/device_providers.dart';
import 'package:snow_owl/features/capture/domain/capture_models.dart';
import 'package:snow_owl/features/capture/domain/capture_session_extended.dart';
import 'package:snow_owl/features/capture/services/capture_service.dart';
import 'package:snow_owl/features/settings/application/settings_providers.dart';

// Provides the base API URL from settings
final apiBaseUrlProvider = Provider<Uri>((ref) {
  final settings = ref.watch(settingsStateProvider);
  final baseUrl = settings.serverConfig.apiUrl.isNotEmpty 
      ? settings.serverConfig.apiUrl 
      : 'http://localhost:8081';
  return Uri.parse(baseUrl);
});

// Provides a shared HTTP client
final httpClientProvider = Provider<http.Client>((ref) {
  final client = http.Client();
  ref.onDispose(client.close);
  return client;
});

// Provides the CaptureService implementation
final captureServiceProvider = Provider<CaptureService>((ref) {
  final client = ref.watch(httpClientProvider);
  final baseUrl = ref.watch(apiBaseUrlProvider);
  return HttpCaptureService(client: client, baseUrl: baseUrl);
});

// Provides the currently selected device ID for capture operations
final captureSessionProvider =
    FutureProvider<CaptureSessionState?>((ref) async {
  final deviceId = ref.watch(selectedDeviceIdProvider);
  if (deviceId == null) {
    return null;
  }
  final service = ref.watch(captureServiceProvider);
  return service.loadSession(deviceId);
});

// Provides extended capture session details
final extendedCaptureSessionProvider =
    FutureProvider<ExtendedCaptureSession?>((ref) async {
  final deviceId = ref.watch(selectedDeviceIdProvider);
  if (deviceId == null) {
    return null;
  }
  final service = ref.watch(captureServiceProvider);
  return service.loadExtendedSession(deviceId);
});

// Provides the selected device ID for capture operations
final streamUrlProvider = FutureProvider.family<String?, String>((ref, deviceId) async {
  final deviceService = ref.watch(deviceServiceProvider(DeviceServiceType.api));
  return deviceService.getStreamUrl(deviceId);
});