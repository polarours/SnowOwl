import 'package:flutter/material.dart';

enum DeviceKind {
  camera,
  encoder,
  sensor,
  gateway,
  rtsp,
  rtmp,
  file,
}

class DeviceNode {
  const DeviceNode({
    required this.id,
    required this.name,
    required this.kind,
    required this.status,
    this.children = const [],
    this.registrationTime,
    this.lastSeen,
    this.hasMicrophone = false, 
  });

  final String id;
  final String name;
  final DeviceKind kind;
  final DeviceStatus status;
  final List<DeviceNode> children;
  final DateTime? registrationTime; 
  final DateTime? lastSeen; 
  final bool hasMicrophone; 

  DeviceNode copyWith({
    String? id,
    String? name,
    DeviceKind? kind,
    DeviceStatus? status,
    List<DeviceNode>? children,
    DateTime? registrationTime,
    DateTime? lastSeen,
    bool? hasMicrophone,
  }) {
    return DeviceNode(
      id: id ?? this.id,
      name: name ?? this.name,
      kind: kind ?? this.kind,
      status: status ?? this.status,
      children: children ?? this.children,
      registrationTime: registrationTime ?? this.registrationTime,
      lastSeen: lastSeen ?? this.lastSeen,
      hasMicrophone: hasMicrophone ?? this.hasMicrophone,
    );
  }
}

enum DeviceStatus {
  online,
  offline,
  maintenance,
}

class DeviceDetails {
  const DeviceDetails({
    required this.device,
    required this.ipAddress,
    required this.location,
    required this.firmwareVersion,
    required this.lastHeartbeat,
    required this.streamEndpoint,
    required this.tags,
    this.registrationTime, 
    this.description = '', 
    this.model = '', 
    this.manufacturer = '',
    this.hasMicrophone = false, 
  });

  final DeviceNode device;
  final String ipAddress;
  final String location;
  final String firmwareVersion;
  final DateTime lastHeartbeat;
  final String streamEndpoint;
  final List<String> tags;
  final DateTime? registrationTime; 
  final String description; 
  final String model; 
  final String manufacturer; 
  final bool hasMicrophone; 

  DeviceDetails copyWith({
    DeviceNode? device,
    String? ipAddress,
    String? location,
    String? firmwareVersion,
    DateTime? lastHeartbeat,
    String? streamEndpoint,
    List<String>? tags,
    DateTime? registrationTime,
    String? description,
    String? model,
    String? manufacturer,
    bool? hasMicrophone,
  }) {
    return DeviceDetails(
      device: device ?? this.device,
      ipAddress: ipAddress ?? this.ipAddress,
      location: location ?? this.location,
      firmwareVersion: firmwareVersion ?? this.firmwareVersion,
      lastHeartbeat: lastHeartbeat ?? this.lastHeartbeat,
      streamEndpoint: streamEndpoint ?? this.streamEndpoint,
      tags: tags ?? this.tags,
      registrationTime: registrationTime ?? this.registrationTime,
      description: description ?? this.description,
      model: model ?? this.model,
      manufacturer: manufacturer ?? this.manufacturer,
      hasMicrophone: hasMicrophone ?? this.hasMicrophone,
    );
  }
}

extension DeviceKindIcon on DeviceKind {
  IconData get icon {
    switch (this) {
      case DeviceKind.camera:
        return Icons.videocam_outlined;
      case DeviceKind.encoder:
        return Icons.settings_input_component_outlined;
      case DeviceKind.sensor:
        return Icons.sensors;
      case DeviceKind.gateway:
        return Icons.hub_outlined;
      case DeviceKind.rtsp:
      case DeviceKind.rtmp:
      case DeviceKind.file:
        return Icons.video_library_outlined;
    }
  }
}

extension DeviceStatusChip on DeviceStatus {
  Color get color {
    switch (this) {
      case DeviceStatus.online:
        return const Color(0xFF34C759);
      case DeviceStatus.offline:
        return const Color(0xFFFF3B30);
      case DeviceStatus.maintenance:
        return const Color(0xFFFFCC00);
    }
  }

  String get label {
    switch (this) {
      case DeviceStatus.online:
        return 'online';
      case DeviceStatus.offline:
        return 'offline';
      case DeviceStatus.maintenance:
        return 'maintenance';
    }
  }
}