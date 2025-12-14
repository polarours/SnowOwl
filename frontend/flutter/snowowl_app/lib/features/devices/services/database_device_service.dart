import 'package:postgres/postgres.dart';

import 'package:snow_owl/features/devices/domain/device_models.dart';
import 'dart:convert';
import 'dart:io';

class DatabaseDeviceService {
  final String _host;
  final int _port;
  final String _database;
  final String _username;
  final String _password;

  DatabaseDeviceService({
    String host = 'localhost',
    int port = 5432,
    String database = 'snowowl_dev',
    String username = 'snowowl_dev',
    String password = 'SnowOwl_Dev!',
  })  : _host = host,
        _port = port,
        _database = database,
        _username = username,
        _password = password;

  Future<List<DeviceNode>> fetchTree() async {
    try {
      final endpoint = Endpoint(
        host: _host,
        port: _port,
        database: _database,
        username: _username,
        password: _password,
      );

      final connection = await Connection.open(endpoint);

      final result = await connection.execute(
        'SELECT id, name, kind, uri, is_primary, enabled, COALESCE(metadata, \'{}\'::json) as metadata FROM devices ORDER BY id ASC;',
      );

      await connection.close();

      final devices = <DeviceNode>[];
      for (final row in result) {
        final id = row.toColumnMap()['id'].toString();
        final name = row.toColumnMap()['name'] as String;
        final kindStr = row.toColumnMap()['kind'] as String;
        final enabled = row.toColumnMap()['enabled'] as int;
        final metadataStr = row.toColumnMap()['metadata'] as String;

        final kind = _parseDeviceKind(kindStr);
        final status = enabled == 1 ? DeviceStatus.online : DeviceStatus.offline;

        DateTime? registrationTime;
        DateTime? lastSeen;

        try {
          if (metadataStr.isNotEmpty && metadataStr != '{}') {
            final metadata = json.decode(metadataStr) as Map<String, dynamic>;
            if (metadata['registration_time'] != null) {
              registrationTime = DateTime.parse(metadata['registration_time'] as String);
            }
            if (metadata['last_seen'] != null) {
              lastSeen = DateTime.parse(metadata['last_seen'] as String);
            }
          }
        } catch (e) {
          // print('Error parsing metadata: $e');
        }

        devices.add(DeviceNode(
          id: id,
          name: name,
          kind: kind,
          status: status,
          registrationTime: registrationTime,
          lastSeen: lastSeen,
        ),);
      }

      return _organizeDevices(devices);
    } on SocketException {
      return _getStaticData();
    } catch (e) {
      // print('Unexpected error: $e');
      return _getStaticData();
    }
  }

  Future<DeviceDetails?> fetchDetails(String deviceId) async {
    try {
      final endpoint = Endpoint(
        host: _host,
        port: _port,
        database: _database,
        username: _username,
        password: _password,
      );

      final connection = await Connection.open(endpoint);

      final result = await connection.execute(
        'SELECT id, name, kind, uri, is_primary, enabled, COALESCE(metadata, \'{}\'::json) as metadata FROM devices WHERE id = @deviceId LIMIT 1;',
        parameters: {'deviceId': int.tryParse(deviceId) ?? 0},
      );

      await connection.close();

      if (result.isNotEmpty) {
        final row = result.first;
        final id = row.toColumnMap()['id'].toString();
        final name = row.toColumnMap()['name'] as String;
        final kindStr = row.toColumnMap()['kind'] as String;
        final enabled = row.toColumnMap()['enabled'] as int;
        final metadataStr = row.toColumnMap()['metadata'] as String;

        final kind = _parseDeviceKind(kindStr);
        final status = enabled == 1 ? DeviceStatus.online : DeviceStatus.offline;

        DateTime? registrationTime;
        DateTime? lastSeen;
        String ipAddress = 'Unknown';
        String location = 'Unknown';
        String firmwareVersion = 'Unknown';
        String streamEndpoint = '';
        List<String> tags = [];
        String description = '';
        String model = '';
        String manufacturer = '';

        try {
          if (metadataStr.isNotEmpty && metadataStr != '{}') {
            final metadata = json.decode(metadataStr) as Map<String, dynamic>;
            if (metadata['registration_time'] != null) {
              registrationTime = DateTime.parse(metadata['registration_time'] as String);
            }
            if (metadata['last_seen'] != null) {
              lastSeen = DateTime.parse(metadata['last_seen'] as String);
            }
            if (metadata['ip_address'] != null) {
              ipAddress = metadata['ip_address'] as String;
            }
            if (metadata['location'] != null) {
              location = metadata['location'] as String;
            }
            if (metadata['firmware_version'] != null) {
              firmwareVersion = metadata['firmware_version'] as String;
            }
            if (metadata['stream_endpoint'] != null) {
              streamEndpoint = metadata['stream_endpoint'] as String;
            }
            if (metadata['tags'] != null) {
              final tagsData = metadata['tags'];
              if (tagsData is List) {
                tags = tagsData.map((tag) => tag.toString()).toList();
              }
            }
            if (metadata['description'] != null) {
              description = metadata['description'] as String;
            }
            if (metadata['model'] != null) {
              model = metadata['model'] as String;
            }
            if (metadata['manufacturer'] != null) {
              manufacturer = metadata['manufacturer'] as String;
            }
          }
        } catch (e) {
          // print('Error parsing metadata: $e');
        }

        return DeviceDetails(
          device: DeviceNode(
            id: id,
            name: name,
            kind: kind,
            status: status,
            registrationTime: registrationTime,
            lastSeen: lastSeen,
          ),
          ipAddress: ipAddress,
          location: location,
          firmwareVersion: firmwareVersion,
          lastHeartbeat: lastSeen ?? DateTime.now(),
          streamEndpoint: streamEndpoint,
          tags: tags,
          registrationTime: registrationTime,
          description: description,
          model: model,
          manufacturer: manufacturer,
        );
      }

      return null;
    } on SocketException {
      // print('Network error while fetching device details');
      return null;
    } catch (e) {
      // print('Unexpected error while fetching device details: $e');
      return null;
    }
  }

  DeviceKind _parseDeviceKind(String kind) {
    switch (kind.toLowerCase()) {
      case 'camera':
        return DeviceKind.camera;
      case 'rtsp':
        return DeviceKind.camera;
      case 'rtmp':
        return DeviceKind.camera;
      case 'file':
        return DeviceKind.camera;
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

  List<DeviceNode> _organizeDevices(List<DeviceNode> devices) {
    final cameras = <DeviceNode>[];
    final sensors = <DeviceNode>[];
    final encoders = <DeviceNode>[];
    final gateways = <DeviceNode>[];

    for (final device in devices) {
      switch (device.kind) {
        case DeviceKind.camera:
        case DeviceKind.rtsp:
        case DeviceKind.rtmp:
        case DeviceKind.file:
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
