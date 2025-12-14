import 'dart:async';
import 'dart:convert';
import 'package:http/http.dart' as http;

import 'package:snow_owl/features/alerts/domain/alert_models.dart';

abstract class AlertsService {
  Future<List<AlertEvent>> fetchRecentAlerts();
}

class StaticAlertsService implements AlertsService {
  const StaticAlertsService();

  @override
  Future<List<AlertEvent>> fetchRecentAlerts() async {
    await Future<void>.delayed(const Duration(milliseconds: 150));

    final now = DateTime.now();

    return [
      AlertEvent(
        id: 'alert-001',
        title: 'intrusionDetectionGate1',
        description: 'aiDetectedSuspiciousLoitering',
        timestamp: now.subtract(const Duration(minutes: 4)),
        severity: AlertSeverity.critical,
        location: 'shanghaiHeadquarters',
        relatedDevice: 'cam-01',
      ),
      AlertEvent(
        id: 'alert-002',
        title: 'temperatureAnomalyRecovered',
        description: 'coldChainTempReturnedNormal',
        timestamp: now.subtract(const Duration(minutes: 27)),
        severity: AlertSeverity.info,
        location: 'shenzhenStorage',
        relatedDevice: 'sensor-02',
        acknowledged: true,
      ),
      AlertEvent(
        id: 'alert-003',
        title: 'streamBitrateFluctuation',
        description: 'networkQualityFluctuation',
        timestamp: now.subtract(const Duration(hours: 1, minutes: 12)),
        severity: AlertSeverity.warning,
        location: 'hangzhouFieldStation',
        relatedDevice: 'encoder-07',
      ),
      AlertEvent(
        id: 'alert-004',
        title: 'cameraFirmwareUpdateAvailable',
        description: 'firmwareUpdateAvailable',
        timestamp: now.subtract(const Duration(hours: 3, minutes: 22)),
        severity: AlertSeverity.info,
        location: 'nanjingLab',
        relatedDevice: 'cam-04',
      ),
    ];
  }
}

class ApiAlertsService implements AlertsService {
  final String baseUrl;

  ApiAlertsService({required this.baseUrl});

  @override
  Future<List<AlertEvent>> fetchRecentAlerts() async {
    try {
      final response = await http.get(Uri.parse('$baseUrl/api/v1/alerts/recent'));
      
      if (response.statusCode == 200) {
        final List<dynamic> alertsData = json.decode(response.body);
        final List<AlertEvent> alerts = [];
        
        for (final alert in alertsData) {
          final severity = _parseSeverity(alert['severity'] as String?);
          
          alerts.add(AlertEvent(
            id: alert['id'].toString(),
            title: alert['title'] as String,
            description: alert['description'] as String,
            timestamp: DateTime.parse(alert['timestamp'] as String),
            severity: severity,
            location: alert['location'] as String,
            relatedDevice: alert['related_device'] as String,
            acknowledged: alert['acknowledged'] as bool? ?? false,
          ),);
        }
        
        return alerts;
      } else {
        throw Exception('Failed to load alerts: ${response.statusCode}');
      }
    } catch (e) {
      // print('Error fetching alerts: $e');
      return const []; 
    }
  }
  
  AlertSeverity _parseSeverity(String? severity) {
    switch (severity?.toLowerCase()) {
      case 'critical':
        return AlertSeverity.critical;
      case 'warning':
        return AlertSeverity.warning;
      case 'info':
        return AlertSeverity.info;
      default:
        return AlertSeverity.info;
    }
  }
}