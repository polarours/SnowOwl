import 'package:snow_owl/features/capture/domain/capture_models.dart';

class ExtendedCaptureSession {
  final CaptureSessionState basicSession;
  final StreamInfo streamInfo;
  final CaptureSessionDetails captureSessionDetails;

  ExtendedCaptureSession({
    required this.basicSession,
    required this.streamInfo,
    required this.captureSessionDetails,
  });
}

class StreamInfo {
  final String status;
  final int bitrate;
  final String resolution;
  final int fps;
  final String codec;
  final String type;
  final Map<String, dynamic> configuration;

  StreamInfo({
    required this.status,
    required this.bitrate,
    required this.resolution,
    required this.fps,
    required this.codec,
    required this.type,
    required this.configuration,
  });

  factory StreamInfo.fromJson(Map<String, dynamic> json) {
    return StreamInfo(
      status: json['status'] as String? ?? 'unknown',
      bitrate: json['bitrate'] as int? ?? 0,
      resolution: json['resolution'] as String? ?? 'unknown',
      fps: json['fps'] as int? ?? 0,
      codec: json['codec'] as String? ?? 'unknown',
      type: json['type'] as String? ?? 'unknown',
      configuration: json['configuration'] as Map<String, dynamic>? ?? {},
    );
  }
}

class CaptureSessionDetails {
  final DateTime startedAt;
  final bool isLive;
  final int networkScore;
  final List<CapturePreset> presets;
  final List<CaptureMetric> metrics;

  CaptureSessionDetails({
    required this.startedAt,
    required this.isLive,
    required this.networkScore,
    required this.presets,
    required this.metrics,
  });

  factory CaptureSessionDetails.fromJson(Map<String, dynamic> json) {
    return CaptureSessionDetails(
      startedAt: DateTime.fromMillisecondsSinceEpoch(
          (json['started_at'] as int? ?? 0) * 1000,),
      isLive: json['is_live'] as bool? ?? true,
      networkScore: json['network_score'] as int? ?? 80,
      presets: (json['presets'] as List<dynamic>? ?? [])
          .map((preset) => CapturePreset(
                label: preset['label'] as String? ?? 'Default',
                resolution: preset['resolution'] as String? ?? 'Unknown',
                bitrateMbps:
                    (preset['bitrate_mbps'] as num?)?.toDouble() ?? 4.0,
                frameRate: preset['frame_rate'] as int? ?? 30,
                isDefault: preset['is_default'] as bool? ?? false,
              ),)
          .toList(),
      metrics: (json['metrics'] as List<dynamic>? ?? [])
          .map((metric) => CaptureMetric(
                label: metric['label'] as String? ?? '指标',
                value: metric['value'] as String? ?? '--',
                color: metric['color'],
              ),)
          .toList(),
    );
  }
}