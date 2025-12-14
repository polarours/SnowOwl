import 'package:flutter/material.dart';

enum AlertSeverity {
  critical,
  warning,
  info,
}

class AlertEvent {
  const AlertEvent({
    required this.id,
    required this.title,
    required this.description,
    required this.timestamp,
    required this.severity,
    required this.location,
    required this.relatedDevice,
    this.acknowledged = false,
  });

  final String id;
  final String title;
  final String description;
  final DateTime timestamp;
  final AlertSeverity severity;
  final String location;
  final String relatedDevice;
  final bool acknowledged;
}

extension AlertSeverityStyle on AlertSeverity {
  Color get tint {
    switch (this) {
      case AlertSeverity.critical:
        return const Color(0xFFFF3B30);
      case AlertSeverity.warning:
        return const Color(0xFFFF9500);
      case AlertSeverity.info:
        return const Color(0xFF64D2FF);
    }
  }

  String get label {
    switch (this) {
      case AlertSeverity.critical:
        return '紧急';
      case AlertSeverity.warning:
        return '警告';
      case AlertSeverity.info:
        return '提示';
    }
  }
}
