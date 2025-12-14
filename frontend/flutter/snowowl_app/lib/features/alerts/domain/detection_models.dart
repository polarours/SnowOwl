import 'package:flutter/material.dart';

enum DetectionType {
  motion,
  intrusion,
  fire,
  gasLeak,
  equipment,
  faceRecognition,
}

class DetectionEvent {
  const DetectionEvent({
    required this.id,
    required this.type,
    required this.description,
    required this.timestamp,
    required this.confidence,
    required this.location,
    required this.deviceId,
    this.boundingBox,
  });

  final String id;
  final DetectionType type;
  final String description;
  final DateTime timestamp;
  final double confidence;
  final String location;
  final String deviceId;
  final Rect? boundingBox;

  DetectionEvent copyWith({
    String? id,
    DetectionType? type,
    String? description,
    DateTime? timestamp,
    double? confidence,
    String? location,
    String? deviceId,
    Rect? boundingBox,
  }) {
    return DetectionEvent(
      id: id ?? this.id,
      type: type ?? this.type,
      description: description ?? this.description,
      timestamp: timestamp ?? this.timestamp,
      confidence: confidence ?? this.confidence,
      location: location ?? this.location,
      deviceId: deviceId ?? this.deviceId,
      boundingBox: boundingBox ?? this.boundingBox,
    );
  }
}

extension DetectionTypeExtension on DetectionType {
  String get label {
    switch (this) {
      case DetectionType.motion:
        return 'Motion';
      case DetectionType.intrusion:
        return 'Intrusion';
      case DetectionType.fire:
        return 'Fire';
      case DetectionType.gasLeak:
        return 'Gas Leak';
      case DetectionType.equipment:
        return 'Equipment';
      case DetectionType.faceRecognition:
        return 'Face Recognition';
    }
  }

  IconData get icon {
    switch (this) {
      case DetectionType.motion:
        return Icons.directions_run;
      case DetectionType.intrusion:
        return Icons.security;
      case DetectionType.fire:
        return Icons.local_fire_department;
      case DetectionType.gasLeak:
        return Icons.opacity;
      case DetectionType.equipment:
        return Icons.build;
      case DetectionType.faceRecognition:
        return Icons.face;
    }
  }

  Color get color {
    switch (this) {
      case DetectionType.motion:
        return Colors.blue;
      case DetectionType.intrusion:
        return Colors.red;
      case DetectionType.fire:
        return Colors.orange;
      case DetectionType.gasLeak:
        return Colors.purple;
      case DetectionType.equipment:
        return Colors.brown;
      case DetectionType.faceRecognition:
        return Colors.green;
    }
  }
}