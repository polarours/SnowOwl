import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/l10n/l10n.dart';
import 'package:snow_owl/theme/app_colors.dart';
import 'package:snow_owl/widgets/glass_panel.dart';
import 'package:snow_owl/features/settings/application/settings_providers.dart';
import 'package:snow_owl/features/settings/domain/settings_models.dart';
import 'package:snow_owl/features/devices/application/device_providers.dart';

class SettingsPage extends ConsumerWidget {
  const SettingsPage({super.key});

  static const routePath = '/settings';
  static const routeName = 'settings';
  static const navigationIcon = Icons.settings_outlined;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final settings = ref.watch(settingsStateProvider);
    final notifier = ref.read(settingsStateProvider.notifier);
    final serverConfig = settings.serverConfig; // ignore: unused_local_variable // ignore: unused_local_variable
    final connectionStatus = ref.watch(backendConnectionStatusProvider);
    final isConnected = connectionStatus.valueOrNull == true; // ignore: unused_local_variable
    final l10n = context;
    
    return Scaffold(
      extendBody: true,
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            colors: [
              Theme.of(context).colorScheme.surface,
              Theme.of(context).colorScheme.surface,
            ],
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
          ),
        ),
        child: SafeArea(
          bottom: false,
          child: Padding(
            padding: const EdgeInsets.fromLTRB(24, 24, 24, 32),
            child: ListView(
              physics: const BouncingScrollPhysics(),
              children: [
                Text(
                  l10n.deviceManagementTitle,
                  style: Theme.of(context).textTheme.headlineMedium,
                ),
                const SizedBox(height: 10),
        Text(
          'Sync themes, languages, notifications and protocol preferences for a consistent tactical command experience.',
          style: Theme.of(context)
            .textTheme
            .bodyLarge
            ?.copyWith(
              color: Theme.of(context)
                .colorScheme
                .onSurface
                .withValues(alpha: 0.7),),
                ),
                const SizedBox(height: 24),
                GlassPanel(
                  padding: const EdgeInsets.all(24),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(l10n.appearance, style: Theme.of(context).textTheme.titleLarge),
                      const SizedBox(height: 16),
                      _SettingsTile(
                        title: l10n.themeMode,
                        subtitle: l10n.themeModeSubtitle,
                        child: SegmentedButton<ThemePreference>(
                          segments: [
                            ButtonSegment(
                              value: ThemePreference.system,
                              label: Text(l10n.system),
                            ),
                            ButtonSegment(
                              value: ThemePreference.light,
                              label: Text(l10n.light),
                            ),
                            ButtonSegment(
                              value: ThemePreference.dark,
                              label: Text(l10n.dark),
                            ),
                          ],
                          selected: <ThemePreference>{settings.theme},
                          onSelectionChanged: (value) =>
                              notifier.updateTheme(value.first),
                        ),
                      ),
                      _SettingsTile(
                        title: l10n.language,
                        subtitle: l10n.languageSubtitle,
                        child: DropdownButton<String>(
                          value: settings.language,
                          items: const [
                            DropdownMenuItem(
                              value: 'en',
                              child: Text('English'),
                            ),
                            DropdownMenuItem(
                              value: 'zh',
                              child: Text('简体中文'),
                            ),
                          ],
                          onChanged: (value) {
                            if (value != null) notifier.updateLanguage(value);
                          },
                        ),
                      ),
                    ],
                  ),
                ),
                const SizedBox(height: 24),
                GlassPanel(
                  padding: const EdgeInsets.all(24),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        l10n.notificationsAndAutomation,
                        style: Theme.of(context).textTheme.titleLarge,
                      ),
                      const SizedBox(height: 16),
                      SwitchListTile.adaptive(
                        value: settings.enableNotifications,
                        onChanged: notifier.toggleNotifications,
                        title: Text(l10n.pushAlerts),
                        subtitle: Text(l10n.pushAlertsSubtitle),
                      ),
                      const Divider(),
                      SwitchListTile.adaptive(
                        value: settings.autoUpdate,
                        onChanged: notifier.toggleAutoUpdate,
                        title: Text(l10n.automaticFirmwareUpdates),
                        subtitle: Text(l10n.automaticFirmwareUpdatesSubtitle),
                      ),
                    ],
                  ),
                ),
                const SizedBox(height: 24),
                GlassPanel(
                  padding: const EdgeInsets.all(24),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        l10n.streamingPreferences,
                        style: Theme.of(context).textTheme.titleLarge,
                      ),
                      const SizedBox(height: 16),
                      _SettingsTile(
                        title: l10n.preferredProtocol,
                        subtitle: l10n.preferredProtocolSubtitle,
                        child: DropdownButton<String>(
                          value: settings.preferredProtocol,
                          items: [
                            DropdownMenuItem(
                              value: 'WebRTC',
                              child: Text('WebRTC · ${l10n.lowLatency}'),
                            ),
                            DropdownMenuItem(
                              value: 'RTMP',
                              child: Text('RTMP · ${l10n.wideCompatibility}'),
                            ),
                            DropdownMenuItem(
                              value: 'SRT',
                              child: Text('SRT · ${l10n.stableReliable}'),
                            ),
                          ],
                          onChanged: (value) {
                            if (value != null) {
                              notifier.updatePreferredProtocol(value);
                            }
                          },
                        ),
                      ),
                      const SizedBox(height: 16),
                      TextButton.icon(
                        onPressed: () {},
                        icon: const Icon(Icons.analytics_outlined),
                        label: Text(l10n.viewNetworkAssessment),
                      ),
                    ],
                  ),
                ),
                const SizedBox(height: 24),
                GlassPanel(
                  padding: const EdgeInsets.all(24),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'Server Configuration',
                        style: Theme.of(context).textTheme.titleLarge,
                      ),
                      const SizedBox(height: 16),
                      _ServerConfigForm(),
                    ],
                  ),
                ),
                const SizedBox(height: 24),
                Align(
                  alignment: Alignment.centerRight,
                  child: FilledButton.icon(
                    onPressed: () {
                      ScaffoldMessenger.of(context).showSnackBar(
                        SnackBar(
                          content: Text(l10n.settingsSavedSuccessfully),
                          backgroundColor: AppColors.successGreen,
                        ),
                      );
                    },
                    icon: const Icon(Icons.save_outlined),
                    label: Text(l10n.saveSettings),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _SettingsTile extends StatelessWidget {
  const _SettingsTile({
    required this.title,
    required this.subtitle,
    required this.child,
  });

  final String title;
  final String subtitle;
  final Widget child;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(title, style: Theme.of(context).textTheme.titleMedium),
          const SizedBox(height: 4),
          Text(
            subtitle,
            style: Theme.of(context).textTheme.bodySmall?.copyWith(
                  color: Theme.of(context).colorScheme.onSurface.withValues(alpha: 0.7),
                ),
          ),
          const SizedBox(height: 12),
          Align(alignment: Alignment.centerRight, child: child),
        ],
      ),
    );
  }
}

class _ServerConfigForm extends ConsumerStatefulWidget {
  @override
  _ServerConfigFormState createState() => _ServerConfigFormState();
}

class _ServerConfigFormState extends ConsumerState<_ServerConfigForm> {
  final _apiUrlController = TextEditingController();
  
  @override
  void initState() {
    super.initState();
    final settings = ref.read(settingsStateProvider);
    final serverConfig = settings.serverConfig; // ignore: unused_local_variable
    
    _apiUrlController.text = serverConfig.apiUrl;
  }
  
  @override
  void dispose() {
    _apiUrlController.dispose();
    super.dispose();
  }
  
  void _updateServerConfig() {
    final settings = ref.read(settingsStateProvider);
    final notifier = ref.read(settingsStateProvider.notifier);
    
    final serverConfig = settings.serverConfig.copyWith(
      apiUrl: _apiUrlController.text,
    );
    
    notifier.updateServerConfig(serverConfig);
  }
  
  void _connectToServer() async {
    final notifier = ref.read(settingsStateProvider.notifier);
    notifier.updateServerConnectionStatus(true);
    
    // Test the connection
    final deviceService = ref.read(deviceServiceProvider(DeviceServiceType.api));
    final settings = ref.read(settingsStateProvider);
    final isConnected = await deviceService.testConnection(settings.serverConfig.apiUrl);
    
    if (isConnected) {
      // Refresh device list
      ref.invalidate(deviceTreeProvider);
      
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('Connected to server successfully'),
            backgroundColor: AppColors.successGreen,
          ),
        );
      }
    } else {
      notifier.updateServerConnectionStatus(false);
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('Failed to connect to server'),
            backgroundColor: AppColors.alertOrange,
          ),
        );
      }
    }
  }
  
  void _disconnectFromServer() {
    final notifier = ref.read(settingsStateProvider.notifier);
    notifier.updateServerConnectionStatus(false);
    
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content: Text('Disconnected from server'),
        backgroundColor: AppColors.alertOrange,
      ),
    );
  }
  
  @override
  Widget build(BuildContext context) {
    final settings = ref.watch(settingsStateProvider);
    final serverConfig = settings.serverConfig; // ignore: unused_local_variable // ignore: unused_local_variable
    final connectionStatus = ref.watch(backendConnectionStatusProvider);
    final isConnected = connectionStatus.valueOrNull == true; // ignore: unused_local_variable
    
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        TextField(
          controller: _apiUrlController,
          decoration: const InputDecoration(
            labelText: 'Server API URL',
            border: OutlineInputBorder(),
            hintText: 'http://192.168.1.100:8081',
          ),
          onChanged: (_) => _updateServerConfig(),
        ),
        const SizedBox(height: 16),
        Row(
          children: [
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
              decoration: BoxDecoration(
                color: isConnected ? Colors.green : Colors.red,
                borderRadius: BorderRadius.circular(12),
              ),
              child: Text(
                isConnected ? 'Connected' : 'Disconnected',
                style: const TextStyle(color: Colors.white, fontSize: 12),
              ),
            ),
            const SizedBox(width: 16),
            if (!isConnected)
              ElevatedButton(
                onPressed: _connectToServer,
                child: const Text('Connect'),
              )
            else
              ElevatedButton(
                onPressed: _disconnectFromServer,
                child: const Text('Disconnect'),
              ),
          ],
        ),
      ],
    );
  }
}