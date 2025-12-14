import 'dart:convert';
import 'dart:async';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:http/http.dart' as http;
import 'package:web_socket_channel/io.dart';
import 'package:snow_owl/features/devices/domain/device_models.dart';
import 'package:snow_owl/features/devices/services/device_service.dart';
import 'package:snow_owl/features/settings/application/settings_providers.dart';

// backend connection checker
// this provider checks if the backend API is reachable
final backendConnectionCheckerProvider = FutureProvider<bool>((ref) async {
  final settings = ref.watch(settingsStateProvider);
  final baseUrl = settings.serverConfig.apiUrl.isNotEmpty 
      ? settings.serverConfig.apiUrl 
      : 'http://localhost:8081';
  
  try {
    final client = http.Client();
    final response = await client.get(Uri.parse('$baseUrl/api/v1/devices'))
        .timeout(const Duration(seconds: 5));
    client.close();
    return response.statusCode == 200;
  } catch (e) {
    //     // print('Backend connection check failed: $e');
    return false;
  }
});

final apiConfigProvider = FutureProvider<Map<String, dynamic>>((ref) async {
  final settings = ref.watch(settingsStateProvider);
  final baseUrl = settings.serverConfig.apiUrl.isNotEmpty 
      ? settings.serverConfig.apiUrl 
      : 'http://localhost:8081';
      
  try {
    final configResponse = await http.get(Uri.parse('$baseUrl/config/frontend'))
        .timeout(const Duration(seconds: 10));
    if (configResponse.statusCode == 200) {
      return json.decode(configResponse.body);
    }
  } catch (e) {
    // print('Failed to load API config: $e');
  }
  
  return {
    'api': {
      'base_url': baseUrl,
      'timeout_seconds': 30,
      'retry_count': 3,
      'retry_delay_ms': 1000,
    },
    'websocket': {
      'enabled': true,
      'url': '${baseUrl.replaceFirst('http', 'ws')}/ws',
    },
    'polling': {
      'enabled': true,
      'interval_seconds': 30,
    },
  };
});

final deviceServiceProvider = Provider.family<DeviceService, DeviceServiceType>((ref, type) {
  final settings = ref.watch(settingsStateProvider);
  final isConnected = ref.watch(backendConnectionStatusProvider).valueOrNull ?? false;
  
  final baseUrl = settings.serverConfig.apiUrl.isNotEmpty 
      ? settings.serverConfig.apiUrl 
      : 'http://localhost:8081';
  
  // if connected to backend, use API service, else use static service
  if (isConnected) {
    return ApiDeviceService(baseUrl: baseUrl);
  } else {
    return const StaticDeviceService();
  }
});

final deviceTreeProvider = FutureProvider<List<DeviceNode>>((ref) async {
  final service = ref.watch(deviceServiceProvider(DeviceServiceType.api));
  return service.fetchTree();
});

final selectedDeviceControllerProvider =
    StateNotifierProvider<SelectedDeviceController, String?>(
  (ref) => SelectedDeviceController(),
);

final selectedDeviceIdProvider = Provider<String?>((ref) {
  return ref.watch(selectedDeviceControllerProvider);
});

final selectedDeviceDetailsProvider =
    FutureProvider<DeviceDetails?>((ref) async {
  final selectedId = ref.watch(selectedDeviceIdProvider);
  if (selectedId == null) {
    return null;
  }
  final service = ref.watch(deviceServiceProvider(DeviceServiceType.api));
  return service.fetchDetails(selectedId);
});

final createDeviceProvider = FutureProvider.family<bool, DeviceNode>((ref, device) async {
  final service = ref.watch(deviceServiceProvider(DeviceServiceType.api));
  if (service is ApiDeviceService) {
    return await service.createDevice(device);
  }
  return false;
});

final updateDeviceProvider = FutureProvider.family<bool, DeviceNode>((ref, device) async {
  final service = ref.watch(deviceServiceProvider(DeviceServiceType.api));
  if (service is ApiDeviceService) {
    return await service.updateDevice(device);
  }
  return false;
});

final deleteDeviceProvider = FutureProvider.family<bool, String>((ref, deviceId) async {
  final service = ref.watch(deviceServiceProvider(DeviceServiceType.api));
  if (service is ApiDeviceService) {
    return await service.deleteDevice(deviceId);
  }
  return false;
});

final backendConnectionStatusProvider = FutureProvider<bool>((ref) async {
  final settings = ref.watch(settingsStateProvider);
  final baseUrl = settings.serverConfig.apiUrl.isNotEmpty 
      ? settings.serverConfig.apiUrl 
      : 'http://localhost:8081';
  
  try {
    final service = ApiDeviceService(baseUrl: baseUrl);
    return await service.checkConnection();
  } catch (e) {
    //     // print('Backend connection check failed: $e');
    return false;
  }
});

final autoUpdateBackendConnectionStatusProvider = StreamProvider<bool>((ref) async* {
  final timer = Timer.periodic(const Duration(seconds: 10), (_) {
    ref.invalidate(backendConnectionStatusProvider);
  });
  
  ref.onDispose(() => timer.cancel());
  
  yield* ref.watch(backendConnectionStatusProvider.future).asStream();
});

final autoRefreshDeviceTreeProvider = FutureProvider.autoDispose<List<DeviceNode>>((ref) async {
  final config = await ref.watch(apiConfigProvider.future);
  final intervalSeconds = config['polling']?['interval_seconds'] ?? 30;
  
  ref.keepAlive();
  final timer = Timer.periodic(Duration(seconds: intervalSeconds), (_) {
    ref.invalidateSelf();
  });
  
  ref.onDispose(() => timer.cancel());
  
  final service = ref.watch(deviceServiceProvider(DeviceServiceType.api));
  return service.fetchTree();
});

final websocketDeviceUpdatesProvider = StreamProvider<List<DeviceNode>>((ref) async* {
  final config = await ref.watch(apiConfigProvider.future);
  final websocketConfig = config['websocket'];
  
  if (websocketConfig == null || !(websocketConfig['enabled'] ?? false)) {
    yield await ref.watch(deviceTreeProvider.future);
    return;
  }
  
  final websocketUrl = websocketConfig['url'] ?? 'ws://localhost:8081/ws';
  
  try {
    final channel = IOWebSocketChannel.connect(Uri.parse(websocketUrl));
    
    yield* channel.stream.map((message) {
      try {
        final data = json.decode(message);
        return _parseWebSocketMessage(data);
      } catch (e) {
        // print('Error parsing WebSocket message: $e');
        return <DeviceNode>[];
      }
    });
    } catch (e) {
      // print('WebSocket connection error: $e');
      throw Exception('WebSocket connection failed: $e');
  }
});

List<DeviceNode> _parseWebSocketMessage(dynamic data) {
  if (data is Map<String, dynamic> && data.containsKey('devices')) {
    return _parseDevices(List.from(data['devices']));
  }
  return <DeviceNode>[];
}

List<DeviceNode> _parseDevices(List<dynamic> data) {
  return <DeviceNode>[];
}

class SelectedDeviceController extends StateNotifier<String?> {
  SelectedDeviceController() : super(null);

  void select(String deviceId) {
    state = deviceId;
  }

  void ensureInitialized(List<DeviceNode> nodes) {
    if (state != null) {
      return;
    }
    final firstCamera = _findFirstDevice(nodes);
    if (firstCamera != null) {
      state = firstCamera.id;
    }
  }

  DeviceNode? _findFirstDevice(List<DeviceNode> nodes) {
    for (final node in nodes) {
      if (node.kind == DeviceKind.camera) {
        return node;
      }
      final child = _findFirstDevice(node.children);
      if (child != null) {
        return child;
      }
    }
    return null;
  }
}

enum DeviceServiceType {
  api,
  database,
}