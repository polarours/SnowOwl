import 'package:flutter/material.dart';
import 'package:snow_owl/features/devices/domain/device_models.dart';

/// Represents a zone/area in the dashboard that can contain devices
class DashboardZone {
  final String id;
  final String name;
  final String description;
  final String? backgroundImage; // Path to background image
  final List<DashboardElement> elements;

  DashboardZone({
    required this.id,
    required this.name,
    required this.description,
    this.backgroundImage,
    this.elements = const [],
  });

  DashboardZone copyWith({
    String? id,
    String? name,
    String? description,
    String? backgroundImage,
    List<DashboardElement>? elements,
  }) {
    return DashboardZone(
      id: id ?? this.id,
      name: name ?? this.name,
      description: description ?? this.description,
      backgroundImage: backgroundImage ?? this.backgroundImage,
      elements: elements ?? this.elements,
    );
  }
}

/// Base class for all dashboard elements
abstract class DashboardElement {
  final String id;
  final String name;
  final ElementType type;
  final Offset position;
  final Size size;
  final String? deviceId; // Link to real device ID

  DashboardElement({
    required this.id,
    required this.name,
    required this.type,
    required this.position,
    required this.size,
    this.deviceId,
  });

  DashboardElement copyWith({
    String? id,
    String? name,
    ElementType? type,
    Offset? position,
    Size? size,
    String? deviceId,
  });
}

/// Different types of dashboard elements
enum ElementType {
  camera,
  sensor,
  zone,
}

/// Camera element for dashboard
class CameraElement extends DashboardElement {
  final bool showLiveFeed;
  final String? streamUrl;

  CameraElement({
    required super.id,
    required super.name,
    required super.position,
    required super.size,
    super.deviceId,
    this.showLiveFeed = true,
    this.streamUrl,
  }) : super(type: ElementType.camera);

  @override
  CameraElement copyWith({
    String? id,
    String? name,
    ElementType? type,
    Offset? position,
    Size? size,
    String? deviceId,
    bool? showLiveFeed,
    String? streamUrl,
  }) {
    return CameraElement(
      id: id ?? this.id,
      name: name ?? this.name,
      position: position ?? this.position,
      size: size ?? this.size,
      deviceId: deviceId ?? this.deviceId,
      showLiveFeed: showLiveFeed ?? this.showLiveFeed,
      streamUrl: streamUrl ?? this.streamUrl,
    );
  }
}

/// Sensor element for dashboard
class SensorElement extends DashboardElement {
  final List<SensorReading> readings;

  SensorElement({
    required super.id,
    required super.name,
    required super.position,
    required super.size,
    super.deviceId,
    this.readings = const [],
  }) : super(type: ElementType.sensor);

  @override
  SensorElement copyWith({
    String? id,
    String? name,
    ElementType? type,
    Offset? position,
    Size? size,
    String? deviceId,
    List<SensorReading>? readings,
  }) {
    return SensorElement(
      id: id ?? this.id,
      name: name ?? this.name,
      position: position ?? this.position,
      size: size ?? this.size,
      deviceId: deviceId ?? this.deviceId,
      readings: readings ?? this.readings,
    );
  }
}

/// Represents a sensor reading
class SensorReading {
  final String name;
  final String value;
  final String unit;
  final DateTime timestamp;

  SensorReading({
    required this.name,
    required this.value,
    required this.unit,
    required this.timestamp,
  });
}