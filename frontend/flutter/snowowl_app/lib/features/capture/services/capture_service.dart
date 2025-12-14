import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:http/http.dart' as http;

import 'package:snow_owl/features/capture/domain/capture_models.dart';
import 'package:snow_owl/features/capture/domain/capture_session_extended.dart';
import 'package:snow_owl/theme/app_colors.dart';

abstract class CaptureService {
  Future<CaptureSessionState> loadSession(String deviceId);
  Future<ExtendedCaptureSession> loadExtendedSession(String deviceId);
}

class HttpCaptureService implements CaptureService {
  HttpCaptureService({required this.client, required this.baseUrl});

  final http.Client client;
  final Uri baseUrl;

  @override
  Future<CaptureSessionState> loadSession(String deviceId) async {
    final response =
        await client.get(baseUrl.resolve('/api/v1/capture/session/$deviceId'));
    if (response.statusCode != 200) {
      throw HttpException(
        'Failed to load capture session: ${response.statusCode}',
      );
    }

    final data = jsonDecode(response.body) as Map<String, dynamic>;
    // print('Received capture session data: $data');
    
    // Handle the new API structure
    final captureSession = data['capture_session'] as Map<String, dynamic>? ?? const {};
    // final streamInfo = data['stream_info'] as Map<String, dynamic>? ?? const {};
    final outputs = data['stream_outputs'] as Map<String, dynamic>? ?? const {};
    final rtmp = outputs['rtmp'] as Map<String, dynamic>? ?? const {};
    final hls = outputs['hls'] as Map<String, dynamic>? ?? const {};

    final presets = (captureSession['presets'] as List<dynamic>? ?? const [])
        .map(
          (preset) => CapturePreset(
            label: preset['label'] as String? ?? 'Default',
            resolution: preset['resolution'] as String? ?? 'Unknown',
            bitrateMbps: (preset['bitrate_mbps'] as num?)?.toDouble() ?? 4.0,
            frameRate: preset['frame_rate'] as int? ?? 30,
            isDefault: preset['is_default'] as bool? ?? false,
          ),
        )
        .toList();

    final metrics = (captureSession['metrics'] as List<dynamic>? ?? const [])
        .map(
          (metric) => CaptureMetric(
            label: metric['label'] as String? ?? '指标',
            value: metric['value'] as String? ?? '--',
            color: AppColors.captureRed,
          ),
        )
        .toList();

    // Extract URLs from the new API structure
    String? hlsUrl = data['hls_url'] as String? ?? hls['playlist'] as String?;
    String? rtmpUrl = data['rtmp_url'] as String? ?? rtmp['url'] as String?;
    String? streamKey = data['stream_key'] as String? ?? rtmp['stream_key'] as String?;
    
    // print('Parsed HLS data: $hls');
    // print('HLS URL from data["hls_url"]: ${data['hls_url']}');
    // print('HLS URL from hls["playlist"]: ${hls['playlist']}');
    // print('RTMP URL: $rtmpUrl');
    // print('Stream key: $streamKey');

    // Fallback to construct HLS URL from RTMP URL if needed
    if ((hlsUrl == null || hlsUrl.isEmpty) && rtmpUrl != null && rtmpUrl.isNotEmpty) {
      // Try to construct HLS URL from RTMP URL
      // Assuming format: rtmp://host:port/app/stream_key -> http://host:8888/hls/stream_key.m3u8
      try {
        final uri = Uri.parse(rtmpUrl);
        final pathSegments = uri.pathSegments;
        if (pathSegments.isNotEmpty) {
          final streamKey = pathSegments.last;
          final hlsUri = Uri(
            scheme: 'http',
            host: uri.host,
            port: 8888, // Default HLS port
            pathSegments: ['hls', '$streamKey.m3u8'],
          );
          hlsUrl = hlsUri.toString();
          // print('Constructed HLS URL: $hlsUrl');
        }
      } catch (e) {
        // print('Failed to construct HLS URL from RTMP URL: $e');
      }
    }
    
    // Ultimate fallback - construct a default HLS URL assuming MediaMTX setup
    if (hlsUrl == null || hlsUrl.isEmpty) {
      try {
        // For MediaMTX, HLS URL format is typically http://host:8888/hls/stream_key.m3u8
        // If we have a stream key, use that; otherwise use default 'stream'
        final streamKeyToUse = (streamKey != null && streamKey.isNotEmpty) ? streamKey : 'stream';
        final hlsUri = Uri(
          scheme: 'http',
          host: '127.0.0.1',
          port: 8888, // Default HLS port for MediaMTX
          pathSegments: ['hls', '$streamKeyToUse.m3u8'],
        );
        hlsUrl = hlsUri.toString();
        // print('Using MediaMTX HLS URL: $hlsUrl');
      } catch (e) {
        // print('Failed to construct MediaMTX HLS URL: $e');
      }
    }

    return CaptureSessionState(
      protocol: _protocolFromString(data['protocol'] as String? ?? 'rtmp'),
      serverUrl: rtmpUrl ?? data['rtmp_url'] as String? ?? '',
      streamKey: streamKey ?? '',
      playbackUrl: data['playback_url'] as String?,
      hlsUrl: hlsUrl,
      rtmpUrl: rtmpUrl,
      presets: presets.isEmpty ? _fallbackPresets() : presets,
      metrics: metrics.isEmpty ? _fallbackMetrics() : metrics,
      isLive: captureSession['is_live'] as bool? ?? true,
      networkScore: captureSession['network_score'] as int? ?? 80,
      lastUpdated: DateTime.now(),
    );
  }
  
  @override
  Future<ExtendedCaptureSession> loadExtendedSession(String deviceId) async {
    final basicSession = await loadSession(deviceId);
    
    final response =
        await client.get(baseUrl.resolve('/api/v1/capture/session/$deviceId'));
    if (response.statusCode != 200) {
      throw HttpException(
        'Failed to load extended capture session: ${response.statusCode}',
      );
    }

    final data = jsonDecode(response.body) as Map<String, dynamic>;
    
    final streamInfoJson = data['stream_info'] as Map<String, dynamic>? ?? {};
    final streamInfo = StreamInfo.fromJson(streamInfoJson);
    
    final captureSessionJson = data['capture_session'] as Map<String, dynamic>? ?? {};
    final captureSessionDetails = CaptureSessionDetails.fromJson(captureSessionJson);
    
    return ExtendedCaptureSession(
      basicSession: basicSession,
      streamInfo: streamInfo,
      captureSessionDetails: captureSessionDetails,
    );
  }

  CaptureProtocol _protocolFromString(String value) {
    switch (value.toLowerCase()) {
      case 'hls':
        return CaptureProtocol.rtmp;
      case 'webrtc':
        return CaptureProtocol.webrtc;
      case 'srt':
        return CaptureProtocol.srt;
      default:
        return CaptureProtocol.rtmp;
    }
  }

  List<CapturePreset> _fallbackPresets() {
    return const [
      CapturePreset(
        label: '1080p · 30fps',
        resolution: '1920×1080',
        bitrateMbps: 4.0,
        frameRate: 30,
        isDefault: true,
      ),
      CapturePreset(
        label: '720p · 30fps',
        resolution: '1280×720',
        bitrateMbps: 2.5,
        frameRate: 30,
      ),
    ];
  }

  List<CaptureMetric> _fallbackMetrics() {
    return const [
      CaptureMetric(
        label: 'Output Bitrate',
        value: '4.2 Mbps',
        color: AppColors.captureRed,
      ),
      CaptureMetric(
        label: 'Average Latency',
        value: '190 ms',
        color: AppColors.monitoringBlue,
      ),
      CaptureMetric(
        label: 'Packet Loss Rate',
        value: '0.4 %',
        color: AppColors.warningAmber,
      ),
    ];
  }
}

class StaticCaptureService implements CaptureService {
  const StaticCaptureService();

  @override
  Future<CaptureSessionState> loadSession(String deviceId) async {
    await Future<void>.delayed(const Duration(milliseconds: 180));

    final presetSets = <String, List<CapturePreset>>{
      'cam-01': const [
        CapturePreset(
          label: '1080p · 60fps',
          resolution: '1920×1080',
          bitrateMbps: 6.0,
          frameRate: 60,
          isDefault: true,
        ),
        CapturePreset(
          label: '720p · 30fps',
          resolution: '1280×720',
          bitrateMbps: 3.0,
          frameRate: 30,
        ),
        CapturePreset(
          label: '480p · 30fps',
          resolution: '854×480',
          bitrateMbps: 1.5,
          frameRate: 30,
        ),
      ],
      'cam-02': const [
        CapturePreset(
          label: '720p · 30fps',
          resolution: '1280×720',
          bitrateMbps: 2.8,
          frameRate: 30,
          isDefault: true,
        ),
        CapturePreset(
          label: '480p · 30fps',
          resolution: '854×480',
          bitrateMbps: 1.2,
          frameRate: 30,
        ),
      ],
      'cam-03': const [
        CapturePreset(
          label: '1080p · night vision optimized',
          resolution: '1920×1080',
          bitrateMbps: 4.5,
          frameRate: 30,
          isDefault: true,
        ),
        CapturePreset(
          label: '720p · night vision',
          resolution: '1280×720',
          bitrateMbps: 2.4,
          frameRate: 30,
        ),
      ],
    };

    final metricSets = <String, List<CaptureMetric>>{
      'cam-01': const [
        CaptureMetric(
          label: 'Output Bitrate',
          value: '5.8 Mbps',
          color: AppColors.captureRed,
        ),
        CaptureMetric(
          label: 'Average Latency',
          value: '162 ms',
          color: AppColors.monitoringBlue,
        ),
        CaptureMetric(
          label: 'Packet Loss Rate',
          value: '0.3 %',
          color: AppColors.warningAmber,
        ),
      ],
      'cam-02': const [
        CaptureMetric(
          label: 'Output Bitrate',
          value: '2.4 Mbps',
          color: AppColors.captureRed,
        ),
        CaptureMetric(
          label: 'Average Latency',
          value: '280 ms',
          color: AppColors.monitoringBlue,
        ),
        CaptureMetric(
          label: 'Packet Loss Rate',
          value: '0.8 %',
          color: AppColors.warningAmber,
        ),
      ],
      'cam-03': const [
        CaptureMetric(
          label: 'Output Bitrate',
          value: '3.9 Mbps',
          color: AppColors.captureRed,
        ),
        CaptureMetric(
          label: 'Average Latency',
          value: '210 ms',
          color: AppColors.monitoringBlue,
        ),
        CaptureMetric(
          label: 'Packet Loss Rate',
          value: '0.5 %',
          color: AppColors.warningAmber,
        ),
      ],
    };

    final protocol = switch (deviceId) {
      'cam-01' => CaptureProtocol.webrtc,
      'cam-02' => CaptureProtocol.rtmp,
      'cam-03' => CaptureProtocol.srt,
      _ => CaptureProtocol.rtmp,
    };

    final serverUrl = switch (protocol) {
      CaptureProtocol.webrtc => 'webrtc://edge.snowowl.live/live/$deviceId',
      CaptureProtocol.srt => 'srt://edge.snowowl.live:9000?streamid=$deviceId',
      CaptureProtocol.rtmp => 'rtmp://127.0.0.1:1935/live',
      CaptureProtocol.rtsp => 'rtsp://stream.snowowl.live/$deviceId',
    };

    final streamKey = switch (deviceId) {
      'cam-01' => 'gateway-a/$deviceId-primary',
      'cam-02' => 'garage/$deviceId-fallback',
      'cam-03' => 'night-vision/$deviceId',
      _ => 'stream',
    };

    final playbackUrl = switch (protocol) {
      CaptureProtocol.rtmp => 'rtmp://127.0.0.1:1935/live/$streamKey',
      CaptureProtocol.webrtc => 'https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4',
      CaptureProtocol.srt => 'https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4',
      CaptureProtocol.rtsp => 'rtsp://stream.snowowl.live/$deviceId',
    };

    final hlsUrl = 'https://media.snowowl.live/hls/$deviceId/index.m3u8';
    // final rtmpPlayback =
    //     protocol == CaptureProtocol.rtmp ? '$serverUrl/$streamKey' : null;

    // For testing purposes, use a sample video stream
    // const testStreamUrl = 'https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4';

    return CaptureSessionState(
      protocol: protocol,
      serverUrl: serverUrl,
      streamKey: streamKey,
      playbackUrl: playbackUrl,// ?? rtmpPlayback ?? testStreamUrl,
      hlsUrl: hlsUrl,
      rtmpUrl: '$serverUrl/$streamKey',
      presets: presetSets[deviceId] ?? presetSets['cam-01']!,
      metrics: metricSets[deviceId] ?? metricSets['cam-01']!,
      isLive: deviceId != 'cam-02',
      networkScore: switch (deviceId) {
        'cam-01' => 86,
        'cam-02' => 68,
        'cam-03' => 74,
        _ => 70,
      },
      lastUpdated: DateTime.now(),
    );
  }
  
  @override
  Future<ExtendedCaptureSession> loadExtendedSession(String deviceId) async {
    final basicSession = await loadSession(deviceId);
    
    // For static service, we simulate the API response
    // Create mock stream info
    final streamInfo = StreamInfo(
      status: 'active',
      bitrate: 4200,
      resolution: '1920x1080',
      fps: 30,
      codec: 'H.264',
      type: 'hls',
      configuration: {
        'rtmp': {'enabled': true, 'url': 'rtmp://127.0.0.1:1935/live/stream'},
        'hls': {'enabled': true, 'playlist': 'http://127.0.0.1:8888/hls/$deviceId/index.m3u8'},
      },
    );
    
    // Create mock capture session details
    final now = DateTime.now();
    final captureSessionDetails = CaptureSessionDetails(
      startedAt: now.subtract(const Duration(minutes: 10)),
      isLive: true,
      networkScore: 85,
      presets: basicSession.presets,
      metrics: basicSession.metrics,
    );
    
    return ExtendedCaptureSession(
      basicSession: basicSession,
      streamInfo: streamInfo,
      captureSessionDetails: captureSessionDetails,
    );
  }
}
