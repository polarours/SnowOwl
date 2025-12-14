import 'package:flutter/material.dart';

enum MonitoringStreamStatus {
  online,
  offline,
  degraded,
}

class MonitoringStream {
  const MonitoringStream({
    required this.id,
    required this.name,
    required this.protocol,
    required this.url,          
    required this.status,
    required this.location,
    required this.health,
    required this.gradient,
    this.tags = const [],
  });

  final String id;
  final String name;
  final String protocol;
  final String url;              
  final MonitoringStreamStatus status;
  final String location;
  final String health;
  final List<Color> gradient;
  final List<String> tags;
}


class MonitoringSessionSummary {
  const MonitoringSessionSummary({
    required this.activeStreams,
    required this.totalBandwidth,
    required this.avgLatency,
    required this.alertsToday,
  });

  final int activeStreams;
  final double totalBandwidth;
  final Duration avgLatency;
  final int alertsToday;
}
