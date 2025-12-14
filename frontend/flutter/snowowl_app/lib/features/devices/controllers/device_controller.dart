import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:snow_owl/features/devices/application/device_providers.dart';
import 'package:snow_owl/features/devices/domain/device_models.dart';
import 'package:snow_owl/features/settings/application/settings_providers.dart';

class DeviceController extends StateNotifier<AsyncValue<List<DeviceNode>>> {
  final Ref ref;
  
  DeviceController(this.ref) : super(const AsyncValue.loading()) {
    refreshDevices();
  }

  Future<void> refreshDevices() async {
    state = const AsyncValue.loading();
    try {
      final devices = await ref.read(deviceServiceProvider(DeviceServiceType.api)).fetchTree();
      state = AsyncValue.data(devices);
    } catch (e, st) {
      state = AsyncValue.error(e, st);
    }
  }

  Future<bool> testConnection() async {
    try {
      final settings = ref.read(settingsStateProvider);
      final baseUrl = settings.serverConfig.apiUrl.isNotEmpty 
          ? settings.serverConfig.apiUrl 
          : 'http://localhost:8081';
          
      final service = ref.read(deviceServiceProvider(DeviceServiceType.api));
      return await service.testConnection(baseUrl);
    } catch (e) {
      return false;
    }
  }
}