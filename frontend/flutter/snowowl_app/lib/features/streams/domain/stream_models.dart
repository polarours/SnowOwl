import 'package:json_annotation/json_annotation.dart';

part 'stream_models.g.dart';

enum StreamProtocol {
  rtmp,
  rtsp,
  hls,
  webrtc,
  file,
}

enum StreamStatus {
  online,
  offline,
  degraded,
}

@JsonSerializable()
class VideoStream {
  final String id;
  final String name;
  final String url;
  final StreamProtocol protocol;
  final StreamStatus status;
  final String location;
  final List<String> tags;
  final DateTime lastUpdated;
  final bool enableDetection;

  VideoStream({
    required this.id,
    required this.name,
    required this.url,
    required this.protocol,
    required this.status,
    required this.location,
    required this.tags,
    required this.lastUpdated,
    this.enableDetection = false,
  });

  factory VideoStream.fromJson(Map<String, dynamic> json) => _$VideoStreamFromJson(json);
  Map<String, dynamic> toJson() => _$VideoStreamToJson(this);

  VideoStream copyWith({
    String? id,
    String? name,
    String? url,
    StreamProtocol? protocol,
    StreamStatus? status,
    String? location,
    List<String>? tags,
    DateTime? lastUpdated,
    bool? enableDetection,
  }) {
    return VideoStream(
      id: id ?? this.id,
      name: name ?? this.name,
      url: url ?? this.url,
      protocol: protocol ?? this.protocol,
      status: status ?? this.status,
      location: location ?? this.location,
      tags: tags ?? this.tags,
      lastUpdated: lastUpdated ?? this.lastUpdated,
      enableDetection: enableDetection ?? this.enableDetection,
    );
  }
}

@JsonSerializable()
class StreamDetectionConfig {
  final bool motionDetection;
  final bool intrusionDetection;
  final bool fireDetection;
  final bool gasLeakDetection;
  final bool equipmentDetection;

  StreamDetectionConfig({
    this.motionDetection = false,
    this.intrusionDetection = false,
    this.fireDetection = false,
    this.gasLeakDetection = false,
    this.equipmentDetection = false,
  });

  factory StreamDetectionConfig.fromJson(Map<String, dynamic> json) => _$StreamDetectionConfigFromJson(json);
  Map<String, dynamic> toJson() => _$StreamDetectionConfigToJson(this);

  StreamDetectionConfig copyWith({
    bool? motionDetection,
    bool? intrusionDetection,
    bool? fireDetection,
    bool? gasLeakDetection,
    bool? equipmentDetection,
  }) {
    return StreamDetectionConfig(
      motionDetection: motionDetection ?? this.motionDetection,
      intrusionDetection: intrusionDetection ?? this.intrusionDetection,
      fireDetection: fireDetection ?? this.fireDetection,
      gasLeakDetection: gasLeakDetection ?? this.gasLeakDetection,
      equipmentDetection: equipmentDetection ?? this.equipmentDetection,
    );
  }
}