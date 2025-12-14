import 'dart:convert';
import 'package:http/http.dart' as http;

import 'package:snow_owl/features/streams/domain/stream_models.dart';
import 'package:snow_owl/features/devices/domain/device_models.dart';

class StreamApiService {
  final String baseUrl;

  StreamApiService({required this.baseUrl});

  Future<List<VideoStream>> fetchStreams() async {
    try {
      final response = await http.get(Uri.parse('$baseUrl/api/v1/devices'));
      
      if (response.statusCode == 200) {
        final List<dynamic> devicesData = json.decode(response.body);
        
        // Filter for stream devices and convert to VideoStream objects
        final List<VideoStream> streams = [];
        
        for (final device in devicesData) {
          final deviceKind = _parseDeviceKind(device['kind']);
          
          // Check if this is a stream device (RTMP, RTSP, etc.)
          if (_isStreamDevice(deviceKind)) {
            final protocol = _mapToDeviceProtocol(deviceKind);
            final status = device['enabled'] ? StreamStatus.online : StreamStatus.offline;
            
            streams.add(VideoStream(
              id: device['id'].toString(),
              name: device['name'],
              url: device['uri'],
              protocol: protocol,
              status: status,
              location: device['metadata']?['location'] ?? 'Unknown',
              tags: [], // Would extract from metadata in real implementation
              lastUpdated: DateTime.now(), // Would come from backend in real implementation
            ),);
          }
        }
        
        return streams;
      } else {
        throw Exception('Failed to load streams: ${response.statusCode}');
      }
    } catch (e) {
      // print('Error fetching streams: $e');
      // Fallback to mock data if API fails
      return await _getMockStreams();
    }
  }

  Future<VideoStream> addStream(VideoStream stream) async {
    try {
      final response = await http.post(
        Uri.parse('$baseUrl/api/v1/devices'),
        headers: {'Content-Type': 'application/json'},
        body: json.encode({
          'name': stream.name,
          'kind': _mapProtocolToDeviceKind(stream.protocol),
          'uri': stream.url,
          'enabled': true,
        }),
      );
      
      if (response.statusCode == 200) {
        final data = json.decode(response.body);
        return stream.copyWith(id: data['id'].toString());
      } else {
        throw Exception('Failed to add stream: ${response.statusCode}');
      }
    } catch (e) {
      // print('Error adding stream: $e');
      throw Exception('Failed to add stream: $e');
    }
  }

  Future<void> updateStream(VideoStream stream) async {
    try {
      // In a real implementation, this would be a PUT request to update the device
      await Future.delayed(const Duration(milliseconds: 300));
    } catch (e) {
      // print('Error updating stream: $e');
      throw Exception('Failed to update stream: $e');
    }
  }

  Future<void> deleteStream(String streamId) async {
    try {
      final response = await http.delete(
        Uri.parse('$baseUrl/api/v1/devices/$streamId'),
      );
      
      if (response.statusCode != 200) {
        throw Exception('Failed to delete stream: ${response.statusCode}');
      }
    } catch (e) {
      // print('Error deleting stream: $e');
      throw Exception('Failed to delete stream: $e');
    }
  }

  Future<void> updateDetectionConfig(String streamId, StreamDetectionConfig config) async {
    try {
      // This would update detection configuration on the backend
      await Future.delayed(const Duration(milliseconds: 300));
    } catch (e) {
      // print('Error updating detection config: $e');
      throw Exception('Failed to update detection config: $e');
    }
  }
  
  DeviceKind _parseDeviceKind(String? kind) {
    switch (kind?.toLowerCase()) {
      case 'camera':
        return DeviceKind.camera;
      case 'rtsp':
        return DeviceKind.rtsp;
      case 'rtmp':
        return DeviceKind.rtmp;
      case 'file':
        return DeviceKind.file;
      default:
        return DeviceKind.camera;
    }
  }
  
  bool _isStreamDevice(DeviceKind kind) {
    // Define which device kinds should be treated as streams
    return kind == DeviceKind.rtsp || 
           kind == DeviceKind.rtmp || 
           kind == DeviceKind.file;
  }
  
  StreamProtocol _mapToDeviceProtocol(DeviceKind kind) {
    switch (kind) {
      case DeviceKind.rtsp:
        return StreamProtocol.rtsp;
      case DeviceKind.rtmp:
        return StreamProtocol.rtmp;
      case DeviceKind.file:
        return StreamProtocol.file;
      default:
        return StreamProtocol.rtmp;
    }
  }
  
  String _mapProtocolToDeviceKind(StreamProtocol protocol) {
    switch (protocol) {
      case StreamProtocol.rtsp:
        return 'rtsp';
      case StreamProtocol.rtmp:
        return 'rtmp';
      case StreamProtocol.file:
        return 'file';
      default:
        return 'rtmp';
    }
  }
  
  Future<List<VideoStream>> _getMockStreams() async {
    // Mock data
    await Future.delayed(const Duration(milliseconds: 500));
    
    return [
      VideoStream(
        id: '1',
        name: 'Pipeline Camera 1',
        url: 'rtmp://localhost/live/stream1',
        protocol: StreamProtocol.rtmp,
        status: StreamStatus.online,
        location: 'Pipeline Sector 1',
        tags: ['pipeline', 'monitoring'],
        lastUpdated: DateTime.now(),
      ),
      VideoStream(
        id: '2',
        name: 'Facility Entrance',
        url: 'rtmp://localhost/live/stream2',
        protocol: StreamProtocol.rtmp,
        status: StreamStatus.online,
        location: 'Facility Entrance',
        tags: ['security', 'intrusion'],
        lastUpdated: DateTime.now(),
      ),
      VideoStream(
        id: '3',
        name: 'Equipment Bay',
        url: 'rtmp://localhost/live/stream3',
        protocol: StreamProtocol.rtmp,
        status: StreamStatus.degraded,
        location: 'Equipment Bay A',
        tags: ['equipment', 'monitoring'],
        lastUpdated: DateTime.now(),
      ),
    ];
  }
}