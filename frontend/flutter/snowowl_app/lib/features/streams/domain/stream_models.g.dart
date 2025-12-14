// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'stream_models.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

VideoStream _$VideoStreamFromJson(Map<String, dynamic> json) => VideoStream(
      id: json['id'] as String,
      name: json['name'] as String,
      url: json['url'] as String,
      protocol: $enumDecode(_$StreamProtocolEnumMap, json['protocol']),
      status: $enumDecode(_$StreamStatusEnumMap, json['status']),
      location: json['location'] as String,
      tags: (json['tags'] as List<dynamic>).map((e) => e as String).toList(),
      lastUpdated: DateTime.parse(json['lastUpdated'] as String),
      enableDetection: json['enableDetection'] as bool? ?? false,
    );

Map<String, dynamic> _$VideoStreamToJson(VideoStream instance) =>
    <String, dynamic>{
      'id': instance.id,
      'name': instance.name,
      'url': instance.url,
      'protocol': _$StreamProtocolEnumMap[instance.protocol]!,
      'status': _$StreamStatusEnumMap[instance.status]!,
      'location': instance.location,
      'tags': instance.tags,
      'lastUpdated': instance.lastUpdated.toIso8601String(),
      'enableDetection': instance.enableDetection,
    };

const _$StreamProtocolEnumMap = {
  StreamProtocol.rtmp: 'rtmp',
  StreamProtocol.rtsp: 'rtsp',
  StreamProtocol.hls: 'hls',
  StreamProtocol.webrtc: 'webrtc',
  StreamProtocol.file: 'file',
};

const _$StreamStatusEnumMap = {
  StreamStatus.online: 'online',
  StreamStatus.offline: 'offline',
  StreamStatus.degraded: 'degraded',
};

StreamDetectionConfig _$StreamDetectionConfigFromJson(Map<String, dynamic> json) =>
    StreamDetectionConfig(
      motionDetection: json['motionDetection'] as bool? ?? false,
      intrusionDetection: json['intrusionDetection'] as bool? ?? false,
      fireDetection: json['fireDetection'] as bool? ?? false,
      gasLeakDetection: json['gasLeakDetection'] as bool? ?? false,
      equipmentDetection: json['equipmentDetection'] as bool? ?? false,
    );

Map<String, dynamic> _$StreamDetectionConfigToJson(StreamDetectionConfig instance) =>
    <String, dynamic>{
      'motionDetection': instance.motionDetection,
      'intrusionDetection': instance.intrusionDetection,
      'fireDetection': instance.fireDetection,
      'gasLeakDetection': instance.gasLeakDetection,
      'equipmentDetection': instance.equipmentDetection,
    };
