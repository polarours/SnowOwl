import 'package:flutter/material.dart';

enum CaptureProtocol {
  rtmp,
  rtsp,
  webrtc,
  srt,
}

class CapturePreset {
  const CapturePreset({
    required this.label,
    required this.resolution,
    required this.bitrateMbps,
    required this.frameRate,
    this.isDefault = false,
  });

  final String label;
  final String resolution;
  final double bitrateMbps;
  final int frameRate;
  final bool isDefault;
}

class CaptureMetric {
  const CaptureMetric({
    required this.label,
    required this.value,
    required this.color,
  });

  final String label;
  final String value;
  final Color color;
}

class CaptureSessionState {
  const CaptureSessionState({
    required this.protocol,
    required this.serverUrl,
    required this.streamKey,
    this.playbackUrl,
    this.hlsUrl,
    this.rtmpUrl,
    required this.presets,
    required this.metrics,
    required this.isLive,
    required this.networkScore,
    required this.lastUpdated,
  });

  final CaptureProtocol protocol;
  final String serverUrl;
  final String streamKey;
  final String? playbackUrl;
  final String? hlsUrl;
  final String? rtmpUrl;
  final List<CapturePreset> presets;
  final List<CaptureMetric> metrics;
  final bool isLive;
  final int networkScore;
  final DateTime lastUpdated;
}
