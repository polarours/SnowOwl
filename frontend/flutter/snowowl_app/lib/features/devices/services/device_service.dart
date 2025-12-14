import 'dart:async';
import 'dart:convert';
import 'package:http/http.dart' as http;

import 'package:snow_owl/features/devices/domain/device_models.dart';

abstract class DeviceService {
  Future<List<DeviceNode>> fetchTree();
  Future<DeviceDetails?> fetchDetails(String deviceId);
  Future<String?> getStreamUrl(String deviceId) async => null;
  
  Future<bool> createDevice(DeviceNode device) async => false;
  Future<bool> updateDevice(DeviceNode device) async => false;
  Future<bool> deleteDevice(String deviceId) async => false;

  Future<bool> checkConnection() async => false;
  Future<bool> testConnection(String baseUrl) async => false;
}

class StaticDeviceService implements DeviceService {
  const StaticDeviceService();

  @override
  Future<List<DeviceNode>> fetchTree() async {
    await Future<void>.delayed(const Duration(milliseconds: 220));

    return const [
      DeviceNode(
        id: 'campus-01',
        name: 'headquartersCampus',
        kind: DeviceKind.gateway,
        status: DeviceStatus.online,
        children: [
          DeviceNode(
            id: 'building-a',
            name: 'buildingAMainControl',
            kind: DeviceKind.encoder,
            status: DeviceStatus.maintenance,
            children: [
              DeviceNode(
                id: 'cam-01',
                name: 'buildingAEntrance1',
                kind: DeviceKind.camera,
                status: DeviceStatus.online,
              ),
              DeviceNode(
                id: 'cam-02',
                name: 'buildingAUndergroundGarage',
                kind: DeviceKind.camera,
                status: DeviceStatus.offline,
              ),
            ],
          ),
          DeviceNode(
            id: 'building-b',
            name: 'buildingBLaboratory',
            kind: DeviceKind.encoder,
            status: DeviceStatus.online,
            children: [
              DeviceNode(
                id: 'sensor-01',
                name: 'labGasSensor',
                kind: DeviceKind.sensor,
                status: DeviceStatus.online,
              ),
            ],
          ),
        ],
      ),
      DeviceNode(
        id: 'campus-02',
        name: 'southChinaBranch',
        kind: DeviceKind.gateway,
        status: DeviceStatus.online,
        children: [
          DeviceNode(
            id: 'cam-03',
            name: 'storageNightVisionCamera',
            kind: DeviceKind.camera,
            status: DeviceStatus.online,
          ),
          DeviceNode(
            id: 'sensor-02',
            name: 'storageTemperatureHumidity',
            kind: DeviceKind.sensor,
            status: DeviceStatus.online,
          ),
        ],
      ),
    ];
  }

  @override
  Future<DeviceDetails?> fetchDetails(String deviceId) async {
    await Future<void>.delayed(const Duration(milliseconds: 160));

    final details = <String, DeviceDetails>{
      'cam-01': DeviceDetails(
        device: const DeviceNode(
          id: 'cam-01',
          name: 'buildingAEntrance1',
          kind: DeviceKind.camera,
          status: DeviceStatus.online,
        ),
        ipAddress: '10.8.12.21',
        location: 'shanghaiHeadquarters',
        firmwareVersion: 'v2.3.7',
        lastHeartbeat: DateTime.now().subtract(const Duration(minutes: 2)),
        streamEndpoint: 'rtmp://127.0.0.1:1935/live/gateway-a/cam-01-primary',
        tags: const ['accessControl', 'aiAnalytics', 'highPriority'],
      ),
      'cam-02': DeviceDetails(
        device: const DeviceNode(
          id: 'cam-02',
          name: 'buildingAUndergroundGarage',
          kind: DeviceKind.camera,
          status: DeviceStatus.offline,
        ),
        ipAddress: '10.8.14.32',
        location: 'shanghaiUndergroundParking',
        firmwareVersion: 'v1.9.4',
        lastHeartbeat: DateTime.now().subtract(const Duration(minutes: 18)),
        streamEndpoint: 'rtmp://127.0.0.1:1935/live/garage/cam-02-fallback',
        tags: const ['parking', 'lowLight', 'backupChannel'],
      ),
      'cam-03': DeviceDetails(
        device: const DeviceNode(
          id: 'cam-03',
          name: 'storageNightVisionCamera',
          kind: DeviceKind.camera,
          status: DeviceStatus.online,
        ),
        ipAddress: '172.16.42.8',
        location: 'shenzhenStorage',
        firmwareVersion: 'v2.5.1-nightly',
        lastHeartbeat: DateTime.now().subtract(const Duration(minutes: 1)),
        streamEndpoint: 'rtmp://127.0.0.1:1935/live/night-vision/cam-03',
        tags: const ['nightVision', 'outdoor', 'patrol'],
      ),
    };

    return details[deviceId] ?? details['cam-01']!;
  }
  
  @override
  Future<String?> getStreamUrl(String deviceId) async {
    final streamUrls = <String, String>{
      'cam-01': 'rtmp://127.0.0.1:1935/live/gateway-a/cam-01-primary',
      'cam-02': 'rtmp://127.0.0.1:1935/live/garage/cam-02-fallback',
      'cam-03': 'rtmp://127.0.0.1:1935/live/night-vision/cam-03',
    };
    
    return streamUrls[deviceId];
  }
  
  @override
  Future<bool> createDevice(DeviceNode device) async => false;
  
  @override
  Future<bool> updateDevice(DeviceNode device) async => false;
  
  @override
  Future<bool> deleteDevice(String deviceId) async => false;
  
  @override
  Future<bool> checkConnection() async {
    return true;
  }
  
  @override
  Future<bool> testConnection(String baseUrl) async {
    return true;
  }
}

class ApiDeviceService implements DeviceService {
  final String baseUrl;

  ApiDeviceService({required this.baseUrl});

  @override
  Future<List<DeviceNode>> fetchTree() async {
    try {
      final response = await http.get(Uri.parse('$baseUrl/api/v1/devices'));
      
      if (response.statusCode == 200) {
        final List<dynamic> devicesData = json.decode(response.body);
        
        final List<DeviceNode> nodes = []; // ignore: unused_local_variable
        final Map<String, DeviceNode> nodeMap = {};
        
        for (final device in devicesData) {
          final deviceKind = _parseDeviceKind(device['kind']);
          
          bool hasMicrophone = false;
          if (device.containsKey('metadata') && device['metadata'] is Map) {
            final metadata = device['metadata'] as Map<String, dynamic>;
            if (metadata.containsKey('has_microphone')) {
              hasMicrophone = metadata['has_microphone'] as bool? ?? false;
            }
            else if (metadata.containsKey('edge_device') && metadata['edge_device'] is Map) {
              final edgeDevice = metadata['edge_device'] as Map<String, dynamic>;
              if (edgeDevice.containsKey('has_microphone')) {
                hasMicrophone = edgeDevice['has_microphone'] as bool? ?? false;
              }
            }
          }
          
          if (!_isStreamDevice(deviceKind)) {
            final node = DeviceNode(
              id: device['id'].toString(),
              name: device['name'],
              kind: deviceKind,
              status: device['enabled'] ? DeviceStatus.online : DeviceStatus.offline,
              hasMicrophone: hasMicrophone,
            );
            
            nodeMap[node.id] = node;
          }
        }
        
        return nodeMap.values.toList();
      } else {
        throw Exception('Failed to load devices: ${response.statusCode}');
      }
    } catch (e) {
      // print('Error fetching devices: $e');
      return await _getStaticData();
    }
  }

  @override
  Future<DeviceDetails?> fetchDetails(String deviceId) async {
    try {
      final response = await http.get(Uri.parse('$baseUrl/api/v1/devices'));
      
      if (response.statusCode == 200) {
        final List<dynamic> devicesData = json.decode(response.body);
        
        for (final device in devicesData) {
          if (device['id'].toString() == deviceId) {
            final deviceKind = _parseDeviceKind(device['kind']);
            
            bool hasMicrophone = false;
            Map<String, dynamic> metadata = {};
            if (device.containsKey('metadata') && device['metadata'] is Map) {
              metadata = device['metadata'] as Map<String, dynamic>;
              if (metadata.containsKey('has_microphone')) {
                hasMicrophone = metadata['has_microphone'] as bool? ?? false;
              }
              else if (metadata.containsKey('edge_device') && metadata['edge_device'] is Map) {
                final edgeDevice = metadata['edge_device'] as Map<String, dynamic>;
                if (edgeDevice.containsKey('has_microphone')) {
                  hasMicrophone = edgeDevice['has_microphone'] as bool? ?? false;
                }
              }
            }
            
            return DeviceDetails(
              device: DeviceNode(
                id: device['id'].toString(),
                name: device['name'],
                kind: deviceKind,
                status: device['enabled'] ? DeviceStatus.online : DeviceStatus.offline,
                hasMicrophone: hasMicrophone,
              ),
              ipAddress: metadata.containsKey('ip_address') 
                  ? metadata['ip_address'] as String 
                  : 'Unknown',
              location: metadata.containsKey('location') 
                  ? metadata['location'] as String 
                  : 'Unknown',
              firmwareVersion: metadata.containsKey('firmware_version') 
                  ? metadata['firmware_version'] as String 
                  : 'Unknown',
              lastHeartbeat: metadata.containsKey('last_heartbeat') 
                  ? DateTime.tryParse(metadata['last_heartbeat'] as String) ?? DateTime.now()
                  : DateTime.now(),
              streamEndpoint: device['uri'] as String? ?? '',
              tags: metadata.containsKey('tags') && metadata['tags'] is List
                  ? (metadata['tags'] as List).map((e) => e.toString()).toList()
                  : [],
              hasMicrophone: hasMicrophone,
            );
          }
        }
      }
      
      return null;
    } catch (e) {
      return null;
    }
  }
  
  @override
  Future<String?> getStreamUrl(String deviceId) async {
    try {
      final response = await http.get(Uri.parse('$baseUrl/api/v1/capture/session/$deviceId'));
      
      if (response.statusCode == 200) {
        final data = json.decode(response.body);
        return data['rtmp_url'] ?? data['hls_url'];
      }
      return null;
    } catch (e) {
      // print('Error fetching stream URL: $e');
      return null;
    }
  }
  
  @override
  Future<bool> createDevice(DeviceNode device) async {
    try {
      final response = await http.post(
        Uri.parse('$baseUrl/api/v1/devices'),
        headers: {'Content-Type': 'application/json'},
        body: json.encode({
          'name': device.name,
          'kind': _deviceKindToString(device.kind),
          'uri': 'device://${device.id}',
          'enabled': device.status == DeviceStatus.online,
        }),
      );
      
      return response.statusCode == 200;
    } catch (e) {
      // print('Error creating device: $e');
      return false;
    }
  }
  
  @override
  Future<bool> updateDevice(DeviceNode device) async {
    try {
      await Future.delayed(const Duration(milliseconds: 300));
      return true;
    } catch (e) {
      // print('Error updating device: $e');
      return false;
    }
  }
  
  @override
  Future<bool> deleteDevice(String deviceId) async {
    try {
      final id = int.tryParse(deviceId);
      if (id == null) return false;
      
      final response = await http.delete(
        Uri.parse('$baseUrl/api/v1/devices/$id'),
      );
      
      return response.statusCode == 200;
    } catch (e) {
      // print('Error deleting device: $e');
      return false;
    }
  }
  
  @override
  Future<bool> checkConnection() async {
    try {
      final response = await http.get(Uri.parse('$baseUrl/api/v1/devices'))
          .timeout(const Duration(seconds: 5));
      return response.statusCode == 200;
    } catch (e) {
      // print('Connection check failed: $e');
      return false;
    }
  }
  
  @override
  Future<bool> testConnection(String baseUrl) async {
    try {
      final response = await http.get(Uri.parse('$baseUrl/api/v1/devices'))
          .timeout(const Duration(seconds: 5));
      return response.statusCode == 200;
    } catch (e) {
      // print('Connection test failed: $e');
      return false;
    }
  }
  
  DeviceKind _parseDeviceKind(String? kind) {
    switch (kind?.toLowerCase()) {
      case 'camera':
        return DeviceKind.camera;
      case 'encoder':
        return DeviceKind.encoder;
      case 'sensor':
        return DeviceKind.sensor;
      case 'gateway':
        return DeviceKind.gateway;
      case 'rtsp':
        return DeviceKind.rtsp;
      case 'rtmp':
        return DeviceKind.rtmp;
      case 'file':
        return DeviceKind.file;
      default:
        return DeviceKind.camera;
    }
  }
  
  bool _isStreamDevice(DeviceKind kind) {
    return kind == DeviceKind.rtsp || 
           kind == DeviceKind.rtmp || 
           kind == DeviceKind.file;
  }
  
  String _deviceKindToString(DeviceKind kind) {
    switch (kind) {
      case DeviceKind.camera:
        return 'camera';
      case DeviceKind.encoder:
        return 'encoder';
      case DeviceKind.sensor:
        return 'sensor';
      case DeviceKind.gateway:
        return 'gateway';
      case DeviceKind.rtsp:
        return 'rtsp';
      case DeviceKind.rtmp:
        return 'rtmp';
      case DeviceKind.file:
        return 'file';
    }
  }
  
  Future<List<DeviceNode>> _getStaticData() async {
    await Future<void>.delayed(const Duration(milliseconds: 220));

    return const [
      DeviceNode(
        id: 'campus-01',
        name: 'headquartersCampus',
        kind: DeviceKind.gateway,
        status: DeviceStatus.online,
        children: [
          DeviceNode(
            id: 'building-a',
            name: 'buildingAMainControl',
            kind: DeviceKind.encoder,
            status: DeviceStatus.maintenance,
            children: [
              DeviceNode(
                id: 'cam-01',
                name: 'buildingAEntrance1',
                kind: DeviceKind.camera,
                status: DeviceStatus.online,
              ),
              DeviceNode(
                id: 'cam-02',
                name: 'buildingAUndergroundGarage',
                kind: DeviceKind.camera,
                status: DeviceStatus.offline,
              ),
            ],
          ),
          DeviceNode(
            id: 'building-b',
            name: 'buildingBLaboratory',
            kind: DeviceKind.encoder,
            status: DeviceStatus.online,
            children: [
              DeviceNode(
                id: 'sensor-01',
                name: 'labGasSensor',
                kind: DeviceKind.sensor,
                status: DeviceStatus.online,
              ),
            ],
          ),
        ],
      ),
      DeviceNode(
        id: 'campus-02',
        name: 'southChinaBranch',
        kind: DeviceKind.gateway,
        status: DeviceStatus.online,
        children: [
          DeviceNode(
            id: 'cam-03',
            name: 'storageNightVisionCamera',
            kind: DeviceKind.camera,
            status: DeviceStatus.online,
          ),
          DeviceNode(
            id: 'sensor-02',
            name: 'storageTemperatureHumidity',
            kind: DeviceKind.sensor,
            status: DeviceStatus.online,
          ),
        ],
      ),
    ];
  }
}
