import 'package:flutter/material.dart';
import 'package:media_kit/media_kit.dart';
import 'package:media_kit_video/media_kit_video.dart';

class SimpleVideoPlayer extends StatefulWidget {
  const SimpleVideoPlayer({
    super.key,
    this.streamUrl,
    this.aspectRatio = 16 / 9,
    this.onError,
  });

  final String? streamUrl;
  final double aspectRatio;
  final Function(String error)? onError;

  @override
  State<SimpleVideoPlayer> createState() => _SimpleVideoPlayerState();
}

class _SimpleVideoPlayerState extends State<SimpleVideoPlayer> {
  Player? _player;
  VideoController? _videoController;
  bool _isPlaying = false;
  bool _isLoading = true;
  String? _errorMessage;
  bool _isBuffering = false;

  @override
  void initState() {
    super.initState();
    _initializePlayer();
  }

  Future<void> _initializePlayer() async {
    if (widget.streamUrl == null || widget.streamUrl!.isEmpty) {
      setState(() {
        _isLoading = false;
        _errorMessage = null;
        _isPlaying = false;
      });
      return;
    }
    
    try {
      // print('Initializing video player with URL: ${widget.streamUrl}');
      
      setState(() {
        _isLoading = true;
        _errorMessage = null;
        _isBuffering = false;
      });

      await _player?.dispose();

      final player = Player(
        configuration: const PlayerConfiguration(
          title: 'SnowOwl Video Player',
        ),
      );
      
      final videoController = VideoController(player);

      player.stream.error.listen((error) {
        // print('Video player error: $error');
        if (mounted) {
          setState(() {
            _errorMessage = error;
            _isLoading = false;
            _isPlaying = false;
          });
          
          widget.onError?.call(error);
        }
      });

      player.stream.completed.listen((completed) {
        // print('Video playback completed: $completed');
        if (completed && mounted) {
          if (widget.streamUrl != null && widget.streamUrl!.isNotEmpty) {
            player.open(Media(widget.streamUrl!), play: true);
          }
        }
      });

      player.stream.buffering.listen((buffering) {
        // print('Video buffering: $buffering');
        if (mounted) {
          setState(() {
            _isBuffering = buffering;
          });
        }
      });

      player.stream.playing.listen((playing) {
        // print('Video playing status: $playing');
        if (mounted) {
          setState(() {
            _isPlaying = playing;
            if (playing) {
              _isLoading = false;
            }
          });
        }
      });

      await player.open(
        Media(widget.streamUrl!),
        play: true,
      );

      if (mounted) {
        setState(() {
          _player = player;
          _videoController = videoController;
          _errorMessage = null;
        });
        // print('Video player initialized successfully');
      }
    } catch (e) {
      // print('Error initializing video player: $e\nStack trace: $stackTrace');
      if (mounted) {
        setState(() {
          _errorMessage = e.toString();
          _isLoading = false;
          _isPlaying = false;
          _isBuffering = false;
        });
        
        widget.onError?.call(e.toString());
      }
    }
  }

  Future<void> _togglePlayPause() async {
    if (_player == null) return;

    try {
      if (_isPlaying) {
        await _player!.pause();
      } else {
        await _player!.play();
      }

      if (mounted) {
        setState(() {
          _isPlaying = !_isPlaying;
        });
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          _errorMessage = e.toString();
        });
        
        widget.onError?.call(e.toString());
      }
    }
  }

  @override
  void dispose() {
    _player?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    if (widget.streamUrl == null || widget.streamUrl!.isEmpty) {
      return Container(
        color: Colors.black,
        child: const Center(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(Icons.videocam_off, size: 48, color: Colors.white70),
              SizedBox(height: 16),
              Text(
                'No video source available',
                style: TextStyle(color: Colors.white70),
              ),
            ],
          ),
        ),
      );
    }
    
    return AspectRatio(
      aspectRatio: widget.aspectRatio,
      child: Container(
        color: Colors.black,
        child: Stack(
          alignment: Alignment.center,
          children: [
            if (_videoController != null)
              Video(controller: _videoController!)
            else
              Container(color: Colors.black),
            
            if (_isLoading || _isBuffering)
              Container(
                color: Colors.black.withValues(alpha: 0.7),
                child: const Center(
                  child: CircularProgressIndicator(
                    valueColor: AlwaysStoppedAnimation<Color>(Colors.white),
                    strokeWidth: 3,
                  ),
                ),
              ),
            
            if (_errorMessage != null)
              Container(
                color: Colors.black.withValues(alpha: 0.8),
                child: Center(
                  child: Padding(
                    padding: const EdgeInsets.all(16),
                    child: Column(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        const Icon(Icons.error_outline, color: Colors.red, size: 48),
                        const SizedBox(height: 16),
                        Text(
                          'Error: $_errorMessage',
                          textAlign: TextAlign.center,
                          style: const TextStyle(color: Colors.white),
                        ),
                        const SizedBox(height: 16),
                        if (_errorMessage != null &&
                            (_errorMessage!.contains('protocol is either unsupported') ||
                                _errorMessage!.contains('disabled at compile-time')))
                          const Text(
                            'This error typically occurs when the required video codecs are not installed. '
                            'Please ensure you have installed the media_kit_libs_video package.',
                            textAlign: TextAlign.center,
                            style: TextStyle(color: Colors.white70, fontSize: 12),
                          ),
                        const SizedBox(height: 16),
                        FilledButton(
                          onPressed: _initializePlayer,
                          child: const Text('Retry'),
                        ),
                      ],
                    ),
                  ),
                ),
              ),
            
            if (!_isLoading && !_isBuffering && _errorMessage == null && !_isPlaying)
              GestureDetector(
                onTap: _togglePlayPause,
                child: Container(
                  color: Colors.transparent,
                  child: Container(
                    padding: const EdgeInsets.all(16),
                    decoration: BoxDecoration(
                      color: Colors.black.withValues(alpha: 0.5),
                      borderRadius: BorderRadius.circular(30),
                    ),
                    child: const Icon(
                      Icons.play_arrow,
                      size: 48,
                      color: Colors.white,
                    ),
                  ),
                ),
              ),
          ],
        ),
      ),
    );
  }
}