import 'dart:convert';
import 'dart:io';

import 'package:http/http.dart' as http;

import 'package:snow_owl/features/devices/domain/device_models.dart';
import 'device_service.dart';

class ApiDeviceService implements DeviceService {
  final String baseUrl;
  
  ApiDeviceService({required this.baseUrl});

  @override
  Future<bool> testConnection(String baseUrl) async {
    try {
      final uri = Uri.parse('$baseUrl/api/v1/devices');
      final response = await http.get(uri).timeout(const Duration(seconds: 5));
      return response.statusCode == 200;
    } catch (e) {
      // print('Connection test failed: $e');
      return false;
    }
  }

  @override
  Future<List<DeviceNode>> fetchTree() async {
    try {
      final uri = Uri.parse('$baseUrl/api/v1/devices');
      final response = await http.get(uri).timeout(const Duration(seconds: 30));
      
      if (response.statusCode == 200) {
        final List<dynamic> data = json.decode(response.body);
        return _parseDevices(data);
      } else {
        throw HttpException('Failed to load devices: ${response.statusCode}');
      }
    } catch (e) {
      // print('Error fetching devices: $e');
      return _getStaticData();
    }
  }

  @override
  Future<DeviceDetails?> fetchDetails(String deviceId) async {
    try {
      final uri = Uri.parse('$baseUrl/api/v1/devices');
      final response = await http.get(uri).timeout(const Duration(seconds: 30));
      
      if (response.statusCode == 200) {
        final List<dynamic> data = json.decode(response.body);
        final devices = _parseDevices(data);
        
        DeviceNode? foundNode;
        void searchNode(DeviceNode node) {
          if (node.id == deviceId) {
            foundNode = node;
            return;
          }
          for (final child in node.children) {
            searchNode(child);
          }
        }
        
        for (final device in devices) {
          searchNode(device);
          if (foundNode != null) break;
        }
        
        if (foundNode != null) {
          return DeviceDetails(
            device: foundNode!,
            ipAddress: '192.168.1.${deviceId.hashCode % 255}',
            location: 'Unknown Location',
            firmwareVersion: 'v1.0.0',
            lastHeartbeat: DateTime.now().subtract(Duration(minutes: deviceId.hashCode % 60)),
            streamEndpoint: 'rtmp://snowowl.live/stream/$deviceId',
            tags: const ['Auto-generated', 'Test'],
            registrationTime: DateTime.now().subtract(Duration(days: deviceId.hashCode % 30)),
            description: 'Device details auto-generated from API data',
            model: 'Generic Model',
            manufacturer: 'SnowOwl',
          );
        }
      }
      
      return null;
    } catch (e) {
      // print('Error fetching device details: $e');
      return null;
    }
  }
  
  @override
  Future<bool> createDevice(DeviceNode device) async {
    try {
      final uri = Uri.parse('$baseUrl/api/v1/devices');
      final response = await http.post(
        uri,
        headers: {'Content-Type': 'application/json'},
        body: json.encode({
          'name': device.name,
          'kind': _deviceKindToString(device.kind),
          'uri': 'device://${device.id}',
          'enabled': device.status == DeviceStatus.online ? 1 : 0,
          'metadata': {
            'registration_time': device.registrationTime?.toIso8601String(),
            'last_seen': device.lastSeen?.toIso8601String(),
          },
        }),
      ).timeout(const Duration(seconds: 30));
      
      return response.statusCode == 200;
    } catch (e) {
      // print('Error creating device: $e');
      return false;
    }
  }
  
  @override
  Future<bool> updateDevice(DeviceNode device) async {
    try {
      final uri = Uri.parse('$baseUrl/api/v1/devices');
      final response = await http.post(
        uri,
        headers: {'Content-Type': 'application/json'},
        body: json.encode({
          'id': int.tryParse(device.id) ?? 0,
          'name': device.name,
          'kind': _deviceKindToString(device.kind),
          'uri': 'device://${device.id}',
          'enabled': device.status == DeviceStatus.online ? 1 : 0,
          'metadata': {
            'registration_time': device.registrationTime?.toIso8601String(),
            'last_seen': device.lastSeen?.toIso8601String(),
          },
        }),
      ).timeout(const Duration(seconds: 30));
      
      return response.statusCode == 200;
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
      
      final uri = Uri.parse('$baseUrl/api/v1/devices/$id');
      final response = await http.delete(uri).timeout(const Duration(seconds: 30));
      
      return response.statusCode == 200;
    } catch (e) {
      // print('Error deleting device: $e');
      return false;
    }
  }

  @override
  Future<bool> checkConnection() async {
    try {
      final uri = Uri.parse('$baseUrl/api/v1/devices');
      final response = await http.get(uri).timeout(const Duration(seconds: 5));
      return response.statusCode == 200;
    } catch (e) {
      // print('Connection check failed: $e');
      return false;
    }
  }

  @override
  Future<String?> getStreamUrl(String deviceId) async {
    try {
      final uri = Uri.parse('$baseUrl/api/v1/capture/session/$deviceId');
      final response = await http.get(uri).timeout(const Duration(seconds: 30));
      
      if (response.statusCode == 200) {
        final data = json.decode(response.body);
        if (data['rtmp_url'] != null && data['rtmp_url'].isNotEmpty) {
          return data['rtmp_url'];
        }
        else if (data['hls_url'] != null && data['hls_url'].isNotEmpty) {
          return data['hls_url'];
        }
        else {
          return 'rtmp://127.0.0.1:1935/live/stream';
        }
      } else {
        // print('Failed to get stream URL: ${response.statusCode}');
        return 'rtmp://127.0.0.1:1935/live/stream';
      }
    } catch (e) {
      // print('Error getting stream URL: $e');
      return 'rtmp://127.0.0.1:1935/live/stream';
    }
  }

  List<DeviceNode> _parseDevices(List<dynamic> data) {
    final nodes = <DeviceNode>[];
    
    for (final device in data) {
      final id = device['id'].toString();
      final name = device['name'] as String? ?? 'unnamed device';
      final kind = _parseDeviceKind(device['kind'] as String? ?? 'unknown');
      final status = (device['enabled'] as bool?) == true ? DeviceStatus.online : DeviceStatus.offline;
      
      DateTime? registrationTime;
      DateTime? lastSeen;
      
      if (device['metadata'] != null && device['metadata'] is Map<String, dynamic>) {
        final metadata = device['metadata'] as Map<String, dynamic>;
        if (metadata['registration_time'] != null) {
          try {
            registrationTime = DateTime.parse(metadata['registration_time'] as String);
          } catch (e) {
            // Ignore parse errors
          }
        }
        if (metadata['last_seen'] != null) {
          try {
            lastSeen = DateTime.parse(metadata['last_seen'] as String);
          } catch (e) {
            // Ignore parse errors
          }
        }
      }
      
      nodes.add(DeviceNode(
        id: id,
        name: name,
        kind: kind,
        status: status,
        registrationTime: registrationTime,
        lastSeen: lastSeen,
      ),);
    }
    
    return _organizeDevices(nodes);
  }
  
  DeviceKind _parseDeviceKind(String kind) {
    switch (kind.toLowerCase()) {
      case 'camera':
        return DeviceKind.camera;
      case 'rtsp':
        return DeviceKind.rtsp;
      case 'rtmp':
        return DeviceKind.rtmp;
      case 'file':
        return DeviceKind.file;
      case 'sensor':
        return DeviceKind.sensor;
      case 'encoder':
        return DeviceKind.encoder;
      case 'gateway':
        return DeviceKind.gateway;
      default:
        return DeviceKind.sensor;
    }
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
  
  List<DeviceNode> _organizeDevices(List<DeviceNode> devices) {
    final cameras = <DeviceNode>[];
    final sensors = <DeviceNode>[];
    final encoders = <DeviceNode>[];
    final gateways = <DeviceNode>[];
    final streams = <DeviceNode>[];
    
    for (final device in devices) {
      switch (device.kind) {
        case DeviceKind.camera:
          cameras.add(device);
          break;
        case DeviceKind.sensor:
          sensors.add(device);
          break;
        case DeviceKind.encoder:
          encoders.add(device);
          break;
        case DeviceKind.gateway:
          gateways.add(device);
          break;
        case DeviceKind.rtsp:
        case DeviceKind.rtmp:
        case DeviceKind.file:
          streams.add(device);
          break;
      }
    }
    
    final organized = <DeviceNode>[];
    
    if (gateways.isNotEmpty) {
      organized.add(DeviceNode(
        id: 'gateways',
        name: 'Gateways',
        kind: DeviceKind.gateway,
        status: DeviceStatus.online,
        children: gateways,
      ),);
    }
    
    if (encoders.isNotEmpty) {
      organized.add(DeviceNode(
        id: 'encoders',
        name: 'Encoders',
        kind: DeviceKind.encoder,
        status: DeviceStatus.online,
        children: encoders,
      ),);
    }
    
    if (cameras.isNotEmpty) {
      organized.add(DeviceNode(
        id: 'cameras',
        name: 'Cameras',
        kind: DeviceKind.camera,
        status: DeviceStatus.online,
        children: cameras,
      ),);
    }
    
    if (sensors.isNotEmpty) {
      organized.add(DeviceNode(
        id: 'sensors',
        name: 'Sensors',
        kind: DeviceKind.sensor,
        status: DeviceStatus.online,
        children: sensors,
      ),);
    }
    
    return organized.isEmpty ? devices : organized;
  }
  
  List<DeviceNode> _getStaticData() {
    return const [
      DeviceNode(
        id: 'campus-01',
        name: 'Headquarters Campus',
        kind: DeviceKind.gateway,
        status: DeviceStatus.online,
        children: [
          DeviceNode(
            id: 'building-a',
            name: 'Building A Control Center',
            kind: DeviceKind.encoder,
            status: DeviceStatus.maintenance,
            children: [
              DeviceNode(
                id: 'cam-01',
                name: 'Building A - Main Entrance',
                kind: DeviceKind.camera,
                status: DeviceStatus.online,
              ),
              DeviceNode(
                id: 'cam-02',
                name: 'Building A - Underground Garage',
                kind: DeviceKind.camera,
                status: DeviceStatus.offline,
              ),
            ],
          ),
          DeviceNode(
            id: 'building-b',
            name: 'Building B Laboratory',
            kind: DeviceKind.encoder,
            status: DeviceStatus.online,
            children: [
              DeviceNode(
                id: 'sensor-01',
                name: 'Laboratory - Gas Sensor',
                kind: DeviceKind.sensor,
                status: DeviceStatus.online,
              ),
            ],
          ),
        ],
      ),
      DeviceNode(
        id: 'campus-02',
        name: 'South China Branch',
        kind: DeviceKind.gateway,
        status: DeviceStatus.online,
        children: [
          DeviceNode(
            id: 'cam-03',
            name: 'Warehouse - Night Vision Camera',
            kind: DeviceKind.camera,
            status: DeviceStatus.online,
          ),
          DeviceNode(
            id: 'sensor-02',
            name: 'Warehouse - Temperature & Humidity',
            kind: DeviceKind.sensor,
            status: DeviceStatus.online,
          ),
        ],
      ),
    ];
  }
}