import 'dart:async';
import 'dart:convert';
import 'package:http/http.dart' as http;
import 'package:flutter/material.dart';

import 'package:snow_owl/features/monitoring/domain/monitoring_models.dart';

abstract class MonitoringService {
  Future<List<MonitoringStream>> fetchStreams();
  Future<MonitoringSessionSummary> fetchSummary();
}

class StaticMonitoringService implements MonitoringService {
  const StaticMonitoringService();

  @override
  Future<List<MonitoringStream>> fetchStreams() async {
    await Future<void>.delayed(const Duration(milliseconds: 200));

    return const [
      MonitoringStream(
        id: 'cam-01',
        name: 'northWarehouseGate1',
        protocol: 'WebRTC',
        status: MonitoringStreamStatus.online,
        location: 'shanghaiPudong',
        health: 'stable4.2Mbps',
        gradient: <Color>[
          Color(0xFF0F2027),
          Color(0xFF203A43),
          Color(0xFF2C5364),
        ],
        url: 'rtmp://127.0.0.1:1935/live/gateway-a/cam-01-primary',
        tags: ['warehouse', 'entranceExit'],
      ),
      MonitoringStream(
        id: 'cam-02',
        name: 'buildingAControlRoom',
        protocol: 'RTSP',
        status: MonitoringStreamStatus.degraded,
        location: 'hangzhouBinjiang',
        health: 'jitter2.4Mbps',
        gradient: <Color>[
          Color(0xFF1A2A6C),
          Color(0xFFb21f1f),
          Color(0xFFfdbb2d),
        ],
        url: 'rtmp://127.0.0.1:1935/live/garage/cam-02-fallback',
        tags: ['hall', 'aiAnalytics'],
      ),
      MonitoringStream(
        id: 'cam-03',
        name: 'southParkingLot',
        protocol: 'RTMP',
        status: MonitoringStreamStatus.online,
        location: 'shenzhenBaoan',
        health: 'normal3.1Mbps',
        gradient: <Color>[Color(0xFF1f4037), Color(0xFF99f2c8)],
        url: 'rtmp://127.0.0.1:1935/live/night-vision/cam-03',
        tags: ['outdoor', 'nightVision'],
      ),
      MonitoringStream(
        id: 'cam-04',
        name: 'labVentilationDuct',
        protocol: 'WebRTC',
        status: MonitoringStreamStatus.offline,
        location: 'nanjingJiangning',
        health: 'offline0Kbps',
        gradient: <Color>[Color(0xFF232526), Color(0xFF414345)],
        url: 'rtmp://127.0.0.1:1935/live/stream',
        tags: ['laboratory'],
      ),
    ];
  }

  @override
  Future<MonitoringSessionSummary> fetchSummary() async {
    await Future<void>.delayed(const Duration(milliseconds: 120));
    return const MonitoringSessionSummary(
      activeStreams: 12,
      totalBandwidth: 36.8,
      avgLatency: Duration(milliseconds: 148),
      alertsToday: 4,
    );
  }
}

class ApiMonitoringService implements MonitoringService {
  final String baseUrl;

  ApiMonitoringService({required this.baseUrl});

  @override
  Future<List<MonitoringStream>> fetchStreams() async {
    try {
      final response = await http.get(Uri.parse('$baseUrl/api/v1/streams'));
      
      if (response.statusCode == 200) {
        final List<dynamic> streamsData = json.decode(response.body);
        final List<MonitoringStream> streams = [];
        
        for (final stream in streamsData) {
          final status = _parseStreamStatus(stream['status'] as String?);
          final gradient = _parseGradient(stream['gradient'] as List<dynamic>?);
          
          streams.add(MonitoringStream(
            id: stream['id'].toString(),
            name: stream['name'] as String,
            protocol: stream['protocol'] as String,
            status: status,
            location: stream['location'] as String,
            health: stream['health'] as String,
            gradient: gradient,
            url: stream['url'] as String,
            tags: List<String>.from(stream['tags'] as List<dynamic>),
          ),);
        }
        
        return streams;
      } else {
        throw Exception('Failed to load streams: ${response.statusCode}');
      }
    } catch (e) {
      // print('Error fetching streams: $e');
      return const []; // Return empty list instead of static data
    }
  }

  @override
  Future<MonitoringSessionSummary> fetchSummary() async {
    try {
      final response = await http.get(Uri.parse('$baseUrl/api/v1/monitoring/summary'));
      
      if (response.statusCode == 200) {
        final data = json.decode(response.body);
        
        return MonitoringSessionSummary(
          activeStreams: data['active_streams'] as int? ?? 0,
          totalBandwidth: (data['total_bandwidth'] as num?)?.toDouble() ?? 0.0,
          avgLatency: Duration(milliseconds: data['avg_latency_ms'] as int? ?? 0),
          alertsToday: data['alerts_today'] as int? ?? 0,
        );
      } else {
        throw Exception('Failed to load summary: ${response.statusCode}');
      }
    } catch (e) {
      // print('Error fetching summary: $e');
      // Return zero-values instead of static data
      return const MonitoringSessionSummary(
        activeStreams: 0,
        totalBandwidth: 0.0,
        avgLatency: Duration(milliseconds: 0),
        alertsToday: 0,
      );
    }
  }
  
  MonitoringStreamStatus _parseStreamStatus(String? status) {
    switch (status?.toLowerCase()) {
      case 'online':
        return MonitoringStreamStatus.online;
      case 'degraded':
        return MonitoringStreamStatus.degraded;
      case 'offline':
        return MonitoringStreamStatus.offline;
      default:
        return MonitoringStreamStatus.offline;
    }
  }
  
  List<Color> _parseGradient(List<dynamic>? gradientData) {
    if (gradientData == null || gradientData.isEmpty) {
      return const [Color(0xFF232526), Color(0xFF414345)];
    }
    
    final colors = <Color>[];
    for (final colorData in gradientData) {
      if (colorData is String && colorData.startsWith('#')) {
        colors.add(Color(int.parse(colorData.substring(1), radix: 16) + 0xFF000000));
      } else if (colorData is int) {
        colors.add(Color(colorData));
      }
    }
    
    return colors.isEmpty ? const [Color(0xFF232526), Color(0xFF414345)] : colors;
  }
}