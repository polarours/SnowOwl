import 'package:media_kit/media_kit.dart';
import 'package:media_kit_video/media_kit_video.dart';
import 'package:snow_owl/features/monitoring/domain/monitoring_models.dart';

class MonitoringPlayerAdapter {
  static Future<MonitoringPlayer> createPlayer(MonitoringStream stream) async {
    switch (stream.protocol.toLowerCase()) {
      case 'rtmp':
      case 'rtsp':
      case 'hls':
        return _createMediaKitPlayer(stream);
      case 'webrtc':
        return _createWebRTCPlayer(stream);
      default:
        return _createMediaKitPlayer(stream);
    }
  }

  static Future<MonitoringPlayer> _createMediaKitPlayer(MonitoringStream stream) async {
    final player = Player();
    final controller = VideoController(player);
    
    try {
      await player.open(Media(stream.url));
      
      return MonitoringPlayer(
        player: player,
        controller: controller,
        dispose: () async {
          await player.dispose();
        },
      );
    } catch (e) {
      await player.dispose();
      rethrow;
    }
  }

  static Future<MonitoringPlayer> _createWebRTCPlayer(MonitoringStream stream) async {
    return _createMediaKitPlayer(stream);
  }
}

class MonitoringPlayer {
  final Player player;
  final VideoController controller;
  final Future<void> Function() dispose;

  MonitoringPlayer({
    required this.player,
    required this.controller,
    required this.dispose,
  });
}