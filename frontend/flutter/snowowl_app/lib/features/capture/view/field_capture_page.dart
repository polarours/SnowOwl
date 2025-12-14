import 'dart:convert';
import 'package:http/http.dart' as http;

import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/theme/app_colors.dart';
import 'package:snow_owl/widgets/glass_panel.dart';
import 'package:snow_owl/features/devices/application/device_providers.dart';
import 'package:snow_owl/features/devices/domain/device_models.dart';
import 'package:snow_owl/features/capture/application/capture_providers.dart';
import 'package:snow_owl/features/capture/domain/capture_models.dart';
import 'package:snow_owl/features/capture/domain/capture_session_extended.dart';
import 'package:snow_owl/features/shared/video_player_widget.dart';

import 'package:snow_owl/l10n/l10n.dart';

class FieldCapturePage extends ConsumerWidget {
  const FieldCapturePage({super.key});

  static const routePath = '/capture';
  static const routeName = 'field-capture';
  static const navigationIcon = Icons.videocam_outlined;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final session = ref.watch(captureSessionProvider);
    final extendedSession = ref.watch(extendedCaptureSessionProvider);
    final deviceDetails = ref.watch(selectedDeviceDetailsProvider);
    final selectedDeviceId = ref.watch(selectedDeviceIdProvider);
    final isDark = Theme.of(context).brightness == Brightness.dark;

    return Scaffold(
      extendBody: true,
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            colors: isDark
                ? const [
                    AppColors.backgroundDark,
                    AppColors.backgroundSecondary,
                  ]
                : const [
                    AppColors.backgroundLight,
                    AppColors.backgroundLightSecondary,
                  ],
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
          ),
        ),
        child: SafeArea(
          bottom: false,
          child: session.when(
            data: (data) {
              if (data == null) {
                return _CaptureDeviceSelection();
              }
              return _CaptureContent(
                state: data,
                extendedState: extendedSession.valueOrNull,
                deviceDetails: deviceDetails.valueOrNull,
                deviceId: selectedDeviceId ?? '',
                onReset: () {
                  ref.read(selectedDeviceControllerProvider.notifier).select('');
                },
                resolution: '1920x1080',
                bitrate: 2000,
                fps: 30,
                protocol: CaptureProtocol.rtmp,
                sceneName: 'Default Scene',
              );
            },
            loading: () => const Center(child: CircularProgressIndicator()),
            error: (error, stackTrace) => _CaptureError(
              onRetry: () => ref.invalidate(captureSessionProvider),
            ),
          ),
        ),
      ),
    );
  }
}

class _CaptureDeviceSelection extends ConsumerWidget {
  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final deviceTree = ref.watch(deviceTreeProvider);
    
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'choose your capture device',
            style: Theme.of(context).textTheme.headlineMedium,
          ),
          const SizedBox(height: 16),
          Text(
            'select a device to use as the current capture device, then you can adjust the parameters',
            style: Theme.of(context).textTheme.bodyLarge?.copyWith(
              color: Theme.of(context).colorScheme.onSurface.withValues(alpha: 0.7),
            ),
          ),
          const SizedBox(height: 24),
          Expanded(
            child: deviceTree.when(
              data: (devices) {
                return ListView.builder(
                  itemCount: devices.length,
                  itemBuilder: (context, index) {
                    final device = devices[index];
                    return Padding(
                      padding: const EdgeInsets.only(bottom: 16),
                      child: _DeviceCard(
                        device: device,
                        onTap: () {
                          ref.read(selectedDeviceControllerProvider.notifier).select(device.id);
                        },
                      ),
                    );
                  },
                );
              },
              loading: () => const Center(child: CircularProgressIndicator()),
              error: (error, stackTrace) => Center(
                child: Text('Failed to load device list: $error'),
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _DeviceCard extends StatelessWidget {
  const _DeviceCard({
    required this.device,
    required this.onTap,
  });

  final DeviceNode device;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    return GlassPanel(
      padding: const EdgeInsets.all(16),
      child: ListTile(
        leading: CircleAvatar(
          backgroundColor: Theme.of(context).colorScheme.primary.withValues(alpha: 0.1),
          child: Icon(device.kind.icon),
        ),
        title: Text(_getLocalizedName(context, device.name)),
        subtitle: Text('${device.kind} · ${_getLocalizedStatus(context, device.status)}'),
        trailing: Icon(
          Icons.arrow_forward_ios,
          size: 16,
          color: Theme.of(context).colorScheme.onSurface.withValues(alpha: 0.6),
        ),
        onTap: onTap,
      ),
    );
  }
  
  String _getLocalizedName(BuildContext context, String key) {
    switch (key) {
      case 'headquartersCampus':
        return context.headquartersCampus;
      case 'buildingAMainControl':
        return context.buildingAMainControl;
      case 'buildingAEntrance1':
        return context.buildingAEntrance1;
      case 'buildingAUndergroundGarage':
        return context.buildingAUndergroundGarage;
      case 'buildingBLaboratory':
        return context.buildingBLaboratory;
      case 'labGasSensor':
        return context.labGasSensor;
      case 'southChinaBranch':
        return context.southChinaBranch;
      case 'storageNightVisionCamera':
        return context.storageNightVisionCamera;
      case 'storageTemperatureHumidity':
        return context.storageTemperatureHumidity;
      default:
        return key;
    }
  }
  
  String _getLocalizedStatus(BuildContext context, DeviceStatus status) {
    switch (status) {
      case DeviceStatus.online:
        return context.online;
      case DeviceStatus.offline:
        return context.offline;
      case DeviceStatus.maintenance:
        return context.maintenance;
    }
  }
}

class _CaptureContent extends ConsumerStatefulWidget {
  const _CaptureContent({
    required this.state,
    this.extendedState,
    required this.deviceDetails,
    required this.onReset,
    required this.deviceId,
    required this.resolution,
    required this.bitrate,
    required this.fps,
    required this.protocol,
    required this.sceneName,
  });

  final CaptureSessionState state;
  final ExtendedCaptureSession? extendedState;
  final DeviceDetails? deviceDetails;
  final String deviceId;
  final VoidCallback onReset;

  // Configuration state
  final String resolution;
  final int bitrate;
  final int fps;
  final CaptureProtocol protocol;
  final String sceneName;

  @override
  ConsumerState<_CaptureContent> createState() => _CaptureContentState();
}

class _CaptureContentState extends ConsumerState<_CaptureContent> {
  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final protocolLabels = {
      CaptureProtocol.rtmp: 'RTMP',
      CaptureProtocol.webrtc: 'WebRTC',
      CaptureProtocol.srt: 'SRT',
      CaptureProtocol.rtsp: 'RTSP',
    };
    final isDark = theme.brightness == Brightness.dark;
    final mutedColor = isDark ? Colors.white70 : Colors.black54;
    final faintColor = isDark ? Colors.white24 : Colors.black26;
    final chipBackground =
        isDark ? Colors.black45 : Colors.white.withValues(alpha: 0.82);
    final diagnosticsGradient = isDark
        ? const [Color(0xFF0f0c29), Color(0xFF302b63), Color(0xFF24243e)]
        : const [Color(0xFFE4EDFF), Color(0xFFF7FAFF)];
    
    // Get HLS URL from either the basic state or extended state
    final hlsUrl = widget.state.hlsUrl ?? widget.extendedState?.basicSession.hlsUrl;
    final hlsHost = hlsUrl == null ? null : Uri.tryParse(hlsUrl)?.host;

    return Padding(
      padding: const EdgeInsets.fromLTRB(24, 24, 24, 32),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'real-time capture monitoring',
                      style: theme.textTheme.headlineMedium?.copyWith(
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    if (widget.deviceDetails != null)
                      Text(
                        widget.deviceDetails!.device.name,
                        style: theme.textTheme.bodyLarge?.copyWith(
                          color: theme.colorScheme.primary,
                          fontWeight: FontWeight.w500,
                        ),
                      ),
                  ],
                ),
              ),
              TextButton.icon(
                onPressed: widget.onReset,
                icon: const Icon(Icons.change_circle_outlined),
                label: const Text('Change Device'),
                style: TextButton.styleFrom(
                  padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                  shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(20),
                    side: BorderSide(color: theme.colorScheme.primary.withValues(alpha: 0.5)),
                  ),
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),
          // Video comparison area - Original on left, configured on right
          Expanded(
            flex: 3,
            child: Row(
              children: [
                // Original video stream
                Expanded(
                  flex: 1,
                  child: GlassPanel(
                    padding: const EdgeInsets.all(2),
                    borderRadius: BorderRadius.circular(24),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Padding(
                          padding: const EdgeInsets.all(12),
                          child: Text(
                            'Original Stream',
                            style: theme.textTheme.titleMedium?.copyWith(
                              fontWeight: FontWeight.bold,
                              color: Colors.white,
                            ),
                          ),
                        ),
                        Expanded(
                          child: ClipRRect(
                            borderRadius: BorderRadius.circular(22),
                            child: SimpleVideoPlayer(
                              streamUrl: widget.state.playbackUrl ?? '',
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
                const SizedBox(width: 16),
                // Configured video stream (shows preview of configuration changes)
                Expanded(
                  flex: 1,
                  child: GlassPanel(
                    padding: const EdgeInsets.all(2),
                    borderRadius: BorderRadius.circular(24),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Padding(
                          padding: const EdgeInsets.all(12),
                          child: Text(
                            'Configured Stream',
                            style: theme.textTheme.titleMedium?.copyWith(
                              fontWeight: FontWeight.bold,
                              color: Colors.white,
                            ),
                          ),
                        ),
                        Expanded(
                          child: ClipRRect(
                            borderRadius: BorderRadius.circular(22),
                            child: SimpleVideoPlayer(
                              streamUrl: widget.state.playbackUrl ?? '',
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(height: 24),
          // control panel - use remaining space
          Expanded(
            flex: 2,
            child: _CaptureControls(
              state: widget.state,
              extendedState: widget.extendedState,
              deviceDetails: widget.deviceDetails,
              protocolLabels: protocolLabels,
              isDark: isDark,
              mutedColor: mutedColor,
              faintColor: faintColor,
              chipBackground: chipBackground,
              diagnosticsGradient: diagnosticsGradient,
              hlsHost: hlsHost,
              deviceId: widget.deviceId,
            ),
          ),
        ],
      ),
    );
  }
}

class _CaptureControls extends ConsumerStatefulWidget {
  const _CaptureControls({
    required this.state,
    this.extendedState,
    required this.deviceDetails,
    required this.protocolLabels,
    required this.isDark,
    required this.mutedColor,
    required this.faintColor,
    required this.chipBackground,
    required this.diagnosticsGradient,
    required this.hlsHost,
    required this.deviceId,
  });

  final CaptureSessionState state;
  final ExtendedCaptureSession? extendedState;
  final DeviceDetails? deviceDetails;
  final Map<CaptureProtocol, String> protocolLabels;
  final bool isDark;
  final Color mutedColor;
  final Color faintColor;
  final Color chipBackground;
  final List<Color> diagnosticsGradient;
  final String? hlsHost;
  final String deviceId;

  @override
  ConsumerState<_CaptureControls> createState() => _CaptureControlsState();
}

class _CaptureControlsState extends ConsumerState<_CaptureControls> {
  // Capture parameters
  late int _bitrate;
  late int _fps;
  late String _resolution;
  late String _sceneName;
  bool _isStreaming = false;
  
  // Preview state parameters
  // late String _previewResolution;
  // late int _previewFps; 
  // late int _previewBitrate;
  // late CaptureProtocol _previewProtocol;

  @override
  void initState() {
    super.initState();
    // initialize with default values or from extended state if available
    if (widget.extendedState != null) {
      final streamInfo = widget.extendedState!.streamInfo;
      _resolution = streamInfo.resolution;
      _bitrate = streamInfo.bitrate;
      _fps = streamInfo.fps;
      // _previewProtocol = CaptureProtocol.rtmp;
      _sceneName = 'Default Scene';
    } else {
      // initialize with default values
      _bitrate = 2000; // kbps
      _fps = 30;
      _resolution = '1920x1080';
      // _previewProtocol = CaptureProtocol.rtmp;
      _sceneName = 'Default Scene';
    }
    
    // Initialize preview parameters
    // _previewResolution = _resolution;
    // _previewFps = _fps;
    // _previewBitrate = _bitrate;
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final hlsUrl = widget.state.hlsUrl;

    return SingleChildScrollView(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(
            widget.deviceDetails == null
                ? 'One-stop monitoring of streaming parameters, network health, preset switching, and real-time metrics.'
                : '${widget.deviceDetails!.device.name} · ${widget.deviceDetails!.device.status}${widget.deviceDetails!.streamEndpoint.isEmpty ? '' : ' · ${widget.deviceDetails!.streamEndpoint}'}',
            style: theme.textTheme.bodyLarge?.copyWith(color: widget.mutedColor),
          ),
          if (widget.deviceDetails != null) ...[
            const SizedBox(height: 8),
            Text(
              'Debug: HLS=${widget.state.hlsUrl ?? 'null'}, Playback=${widget.state.playbackUrl ?? 'null'}, RTMP=${widget.state.rtmpUrl ?? 'null'}',
              style: theme.textTheme.bodySmall?.copyWith(color: Colors.red),
            ),
          ],
          const SizedBox(height: 24),
          Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Expanded(
                child: _CaptureTile(
                  title: 'Scene Name',
                  subtitle: 'Set the scene name for the current capture',
                  trailing: SizedBox(
                    width: 150,
                    child: TextField(
                      decoration: const InputDecoration(
                        border: OutlineInputBorder(),
                        isDense: true,
                        contentPadding: EdgeInsets.symmetric(horizontal: 8, vertical: 8),
                      ),
                      controller: TextEditingController(text: _sceneName),
                      onChanged: (value) {
                        setState(() {
                          _sceneName = value;
                        });
                        
                        // Send updated scene name to the server
                        _updateSceneName(value);
                      },
                    ),
                  ),
                ),
              ),
              const SizedBox(width: 20),
              Expanded(
                child: _CaptureTile(
                  title: 'Streaming URL',
                  subtitle: widget.state.serverUrl,
                  trailing: GlassPanel(
                    padding: const EdgeInsets.symmetric(
                      horizontal: 16,
                      vertical: 10,
                    ),
                    borderRadius: BorderRadius.circular(16),
                    child: Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Icon(
                          Icons.key_rounded,
                          size: 16,
                          color: widget.mutedColor,
                        ),
                        const SizedBox(width: 8),
                        Text(
                          widget.state.streamKey,
                          style: theme.textTheme.bodySmall,
                        ),
                      ],
                    ),
                  ),
                ),
              ),
            ],
          ),
          const SizedBox(height: 20),
          Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Expanded(
                child: _CaptureTile(
                  title: 'Resolution',
                  subtitle: 'Set the video resolution',
                  trailing: DropdownButton<String>(
                    value: _resolution,
                    items: const [
                      DropdownMenuItem(value: '1920x1080', child: Text('1920x1080')),
                      DropdownMenuItem(value: '1280x720', child: Text('1280x720')),
                      DropdownMenuItem(value: '640x480', child: Text('640x480')),
                    ],
                    onChanged: (value) {
                      if (value != null) {
                        setState(() {
                          _resolution = value;
                        });
                        
                        // Send updated resolution to the server
                        _updateResolution(value);
                      }
                    },
                  ),
                ),
              ),
              const SizedBox(width: 20),
              Expanded(
                child: _CaptureTile(
                  title: 'Frame Rate',
                  subtitle: 'Set the video frame rate',
                  trailing: DropdownButton<int>(
                    value: _fps,
                    items: const [
                      DropdownMenuItem(value: 30, child: Text('30 fps')),
                      DropdownMenuItem(value: 25, child: Text('25 fps')),
                      DropdownMenuItem(value: 15, child: Text('15 fps')),
                    ],
                    onChanged: (value) {
                      if (value != null) {
                        setState(() {
                          _fps = value;
                        });
                        
                        // Send updated frame rate to the server
                        _updateFrameRate(value);
                      }
                    },
                  ),
                ),
              ),
              const SizedBox(width: 20),
              Expanded(
                child: _CaptureTile(
                  title: 'Bitrate',
                  subtitle: 'Set the video bitrate',
                  trailing: DropdownButton<int>(
                    value: _bitrate,
                    items: const [
                      DropdownMenuItem(value: 2000, child: Text('2000 kbps')),
                      DropdownMenuItem(value: 1500, child: Text('1500 kbps')),
                      DropdownMenuItem(value: 1000, child: Text('1000 kbps')),
                      DropdownMenuItem(value: 500, child: Text('500 kbps')),
                    ],
                    onChanged: (value) {
                      if (value != null) {
                        setState(() {
                          _bitrate = value;
                        });
                        
                        // Send updated bitrate to the server
                        _updateBitrate(value);
                      }
                    },
                  ),
                ),
              ),
            ],
          ),
          const SizedBox(height: 20),
          Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Expanded(
                child: _CaptureTile(
                  title: 'Streaming Protocol',
                  subtitle: 'Preferred protocol, can switch fallback anytime',
                  trailing: Wrap(
                    spacing: 12,
                    children: CaptureProtocol.values.map((protocol) {
                      final isActive = protocol == widget.state.protocol;
                      final label = widget.protocolLabels[protocol] ?? protocol.name.toUpperCase();
                      return FilterChip(
                        label: Text(label),
                        selected: isActive,
                        onSelected: (_) {
                          // Send updated protocol to the server
                          _updateProtocol(protocol);
                        },
                      );
                    }).toList(),
                  ),
                ),
              ),
              const SizedBox(width: 20),
              Expanded(
                child: _CaptureTile(
                  title: 'HLS Streaming URL',
                  subtitle: hlsUrl ?? 'HLS playlist not generated yet',
                  trailing: hlsUrl == null
                      ? null
                      : GlassPanel(
                          padding: const EdgeInsets.symmetric(
                            horizontal: 16,
                            vertical: 10,
                          ),
                          borderRadius: BorderRadius.circular(16),
                          child: Row(
                            mainAxisSize: MainAxisSize.min,
                            children: [
                              const Icon(Icons.link, size: 16),
                              const SizedBox(width: 8),
                              Text(
                                'M3U8 · ${widget.hlsHost ?? 'HLS'}',
                                style: theme.textTheme.bodySmall,
                              ),
                            ],
                          ),
                        ),
                ),
              ),
            ],
          ),
          const SizedBox(height: 24),
          Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Expanded(
                child: _CaptureTile(
                  title: 'Encoding Preset',
                  subtitle: 'Quickly switch for different scenarios',
                  trailing: Wrap(
                    spacing: 12,
                    runSpacing: 12,
                    children: widget.state.presets
                        .map(
                          (preset) => ChoiceChip(
                            label: Text(preset.label),
                            selected: preset.isDefault,
                            onSelected: (_) {
                              // Apply preset configuration
                              _applyPreset(preset);
                            },
                          ),
                        )
                        .toList(),
                  ),
                ),
              ),
              const SizedBox(width: 24),
              Expanded(
                child: GlassPanel(
                  padding: const EdgeInsets.all(24),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Text(
                        'Network Diagnostics',
                        style: theme.textTheme.titleMedium,
                      ),
                      const SizedBox(height: 8),
                      Text(
                        'Track RTT · Jitter · Packet Loss trends by the minute, with automatic alerts for anomalies.',
                        style: theme.textTheme.bodySmall
                            ?.copyWith(color: widget.mutedColor),
                      ),
                      const SizedBox(height: 18),
                      SizedBox(
                        height: 100,
                        child: ClipRRect(
                          borderRadius: BorderRadius.circular(18),
                          child: Container(
                            decoration: BoxDecoration(
                              gradient: LinearGradient(
                                colors: widget.diagnosticsGradient,
                              ),
                            ),
                            child: Center(
                              child: Icon(
                                Icons.show_chart_rounded,
                                color: widget.faintColor,
                                size: 60,
                              ),
                            ),
                          ),
                        ),
                      ),
                      const SizedBox(height: 14),
                      Row(
                        children: [
                          ElevatedButton.icon(
                            onPressed: () {
                              setState(() {
                                _isStreaming = !_isStreaming;
                              });
                              
                              // Start or stop streaming
                              _toggleStreaming(_isStreaming);
                            },
                            icon: Icon(
                              _isStreaming
                                  ? Icons.stop
                                  : Icons.play_arrow_rounded,
                            ),
                            label: Text(
                              _isStreaming ? 'Stop Streaming' : 'Start Streaming',
                            ),
                          ),
                          const SizedBox(width: 12),
                          TextButton.icon(
                            onPressed: () {
                              // Open advanced settings
                              _openAdvancedSettings();
                            },
                            icon: const Icon(Icons.tune_rounded),
                            label: const Text('Advanced Settings'),
                          ),
                        ],
                      ),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
  
  // Send updated scene name to the server
  void _updateSceneName(String name) {
    // print('Updating scene name to: $name');
    
    // Send HTTP request to the server
    final url = Uri.parse('${widget.state.serverUrl}/api/v1/capture/session/${widget.deviceId}/config');
    http.post(
      url,
      headers: {'Content-Type': 'application/json'},
      body: jsonEncode({
        'device_id': widget.deviceId,
        'action': 'update_scene_name',
        'scene_name': name,
      }),
    ).catchError((error) {
      // print('Failed to update scene name: $error');
      return http.Response('Error', 500); // Return a Response object
    });
  }
  
  // Send updated resolution to the server
  void _updateResolution(String resolution) {
    // print('Updating resolution to: $resolution');
    
    // Send HTTP request to the server
    final url = Uri.parse('${widget.state.serverUrl}/api/v1/capture/session/${widget.deviceId}/config');
    http.post(
      url,
      headers: {'Content-Type': 'application/json'},
      body: jsonEncode({
        'device_id': widget.deviceId,
        'action': 'update_resolution',
        'resolution': resolution,
      }),
    ).catchError((error) {
      // print('Failed to update resolution: $error');
      return http.Response('Error', 500); // Return a Response object
    });
  }
  
  // Send updated frame rate to the server
  void _updateFrameRate(int fps) {
    // print('Updating frame rate to: $fps fps');
    
    // Send HTTP request to the server
    final url = Uri.parse('${widget.state.serverUrl}/api/v1/capture/session/${widget.deviceId}/config');
    http.post(
      url,
      headers: {'Content-Type': 'application/json'},
      body: jsonEncode({
        'device_id': widget.deviceId,
        'action': 'update_frame_rate',
        'fps': fps,
      }),
    ).catchError((error) {
      // print('Failed to update frame rate: $error');
      return http.Response('Error', 500); // Return a Response object
    });
  }
  
  // Send updated bitrate to the server
  void _updateBitrate(int bitrate) {
    // print('Updating bitrate to: $bitrate kbps');
    
    // Send HTTP request to the server
    final url = Uri.parse('${widget.state.serverUrl}/api/v1/capture/session/${widget.deviceId}/config');
    http.post(
      url,
      headers: {'Content-Type': 'application/json'},
      body: jsonEncode({
        'device_id': widget.deviceId,
        'action': 'update_bitrate',
        'bitrate': bitrate,
      }),
    ).catchError((error) {
      // print('Failed to update bitrate: $error');
      return http.Response('Error', 500); // Return a Response object
    });
  }
  
  // Send updated protocol to the server
  void _updateProtocol(CaptureProtocol protocol) {
    // print('Updating protocol to: $protocol');
    
    // Send HTTP request to the server
    final url = Uri.parse('${widget.state.serverUrl}/api/v1/capture/session/${widget.deviceId}/config');
    http.post(
      url,
      headers: {'Content-Type': 'application/json'},
      body: jsonEncode({
        'device_id': widget.deviceId,
        'action': 'update_protocol',
        'protocol': protocol.toString(),
      }),
    ).catchError((error) {
      // print('Failed to update protocol: $error');
      return http.Response('Error', 500); // Return a Response object
    });
  }
  
  // Apply preset configuration
  void _applyPreset(CapturePreset preset) {
    setState(() {
      _resolution = preset.resolution;
      _bitrate = (preset.bitrateMbps * 1000).toInt(); // Convert to kbps
      _fps = preset.frameRate;
    });
    
    // print('Applying preset: ${preset.label}');
    
    // Send HTTP request to the server
    final url = Uri.parse('${widget.state.serverUrl}/api/v1/capture/session/${widget.deviceId}/config');
    http.post(
      url,
      headers: {'Content-Type': 'application/json'},
      body: jsonEncode({
        'device_id': widget.deviceId,
        'action': 'apply_preset',
        'preset': {
          'label': preset.label,
          'resolution': preset.resolution,
          'bitrate_mbps': preset.bitrateMbps,
          'frame_rate': preset.frameRate,
        },
      }),
    ).catchError((error) {
      // print('Failed to apply preset: $error');
      return http.Response('Error', 500); // Return a Response object
    });
  }
  
  // Start or stop streaming
  void _toggleStreaming(bool start) {
    // TODO: Implement logic to start or stop streaming
    // print('${start ? "Starting" : "Stopping"} streaming');
  }
  
  // Open advanced settings
  void _openAdvancedSettings() {
    // TODO: Implement logic to open advanced settings
    // print('Opening advanced settings');
  }
  
  // String _getLocalizedName(BuildContext context, String key) {
  //   switch (key) {
  //     case 'headquartersCampus':
  //       return context.headquartersCampus;
  //     case 'buildingAMainControl':
  //       return context.buildingAMainControl;
  //     case 'buildingAEntrance1':
  //       return context.buildingAEntrance1;
  //     case 'buildingAUndergroundGarage':
  //       return context.buildingAUndergroundGarage;
  //     case 'buildingBLaboratory':
  //       return context.buildingBLaboratory;
  //     case 'labGasSensor':
  //       return context.labGasSensor;
  //     case 'southChinaBranch':
  //       return context.southChinaBranch;
  //     case 'storageNightVisionCamera':
  //       return context.storageNightVisionCamera;
  //     case 'storageTemperatureHumidity':
  //       return context.storageTemperatureHumidity;
  //     case 'shanghaiHeadquarters':
  //       return context.shanghaiHeadquarters;
  //     case 'shanghaiUndergroundParking':
  //       return context.shanghaiUndergroundParking;
  //     case 'shenzhenStorage':
  //       return context.shenzhenStorage;
  //     default:
  //       return key;
  //   }
  // }
  
  // String _getLocalizedStatus(BuildContext context, DeviceStatus status) {
  //   switch (status) {
  //     case DeviceStatus.online:
  //       return context.online;
  //     case DeviceStatus.offline:
  //       return context.offline;
  //     case DeviceStatus.maintenance:
  //       return context.maintenance;
  //   }
  // }
}

class _CaptureTile extends StatelessWidget {
  const _CaptureTile({
    required this.title,
    required this.subtitle,
    this.trailing,
  });

  final String title;
  final String subtitle;
  final Widget? trailing;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final isDark = theme.brightness == Brightness.dark;
    final subtitleColor = isDark ? Colors.white70 : Colors.black54;
    return GlassPanel(
      padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 20),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(title, style: theme.textTheme.titleMedium),
                const SizedBox(height: 6),
                Text(
                  subtitle,
                  style:
                      theme.textTheme.bodySmall?.copyWith(color: subtitleColor),
                ),
              ],
            ),
          ),
          if (trailing != null) trailing!,
        ],
      ),
    );
  }
}

class _CaptureError extends StatelessWidget {
  const _CaptureError({required this.onRetry});

  final VoidCallback onRetry;

  @override
  Widget build(BuildContext context) {
    return Center(
      child: GlassPanel(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text('Streaming Error', style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 12),
            FilledButton.icon(
              onPressed: onRetry,
              icon: const Icon(Icons.refresh),
              label: const Text('Retry'),
            ),
          ],
        ),
      ),
    );
  }
}

