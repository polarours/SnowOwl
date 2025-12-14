import 'package:flutter/foundation.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/features/settings/domain/settings_models.dart';

final settingsStateProvider =
    StateNotifierProvider<SettingsStateNotifier, AppSettings>((ref) {
  // Determine default language based on system locale
  String defaultLanguage = 'en';
  if (PlatformDispatcher.instance.locale.languageCode == 'zh') {
    defaultLanguage = 'zh';
  }
  
  return SettingsStateNotifier(
    AppSettings(
      theme: ThemePreference.light,
      language: defaultLanguage,
      enableNotifications: true,
      autoUpdate: true,
      preferredProtocol: 'WebRTC',
      serverConfig: const ServerConfig(),
    ),
  );
});

class SettingsStateNotifier extends StateNotifier<AppSettings> {
  SettingsStateNotifier(super.state);

  void updateTheme(ThemePreference preference) {
    state = state.copyWith(theme: preference);
  }

  void updateLanguage(String language) {
    state = state.copyWith(language: language);
  }

  void toggleNotifications(bool value) {
    state = state.copyWith(enableNotifications: value);
  }

  void toggleAutoUpdate(bool value) {
    state = state.copyWith(autoUpdate: value);
  }

  void updatePreferredProtocol(String protocol) {
    state = state.copyWith(preferredProtocol: protocol);
  }
  
  void updateServerConfig(ServerConfig config) {
    state = state.copyWith(serverConfig: config);
  }
  
  void updateServerConnectionStatus(bool isConnected) {
    final updatedConfig = state.serverConfig.copyWith(isConnected: isConnected);
    state = state.copyWith(serverConfig: updatedConfig);
  }
  
  void updateApiUrl(String apiUrl) {
    final updatedConfig = state.serverConfig.copyWith(apiUrl: apiUrl);
    state = state.copyWith(serverConfig: updatedConfig);
  }
}