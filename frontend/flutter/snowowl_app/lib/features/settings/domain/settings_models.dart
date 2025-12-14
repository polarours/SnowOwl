enum ThemePreference {
  system,
  light,
  dark,
}

class AppSettings {
  const AppSettings({
    required this.theme,
    required this.language,
    required this.enableNotifications,
    required this.autoUpdate,
    required this.preferredProtocol,
    required this.serverConfig,
  });

  final ThemePreference theme;
  final String language;
  final bool enableNotifications;
  final bool autoUpdate;
  final String preferredProtocol;
  final ServerConfig serverConfig;

  AppSettings copyWith({
    ThemePreference? theme,
    String? language,
    bool? enableNotifications,
    bool? autoUpdate,
    String? preferredProtocol,
    ServerConfig? serverConfig,
  }) {
    return AppSettings(
      theme: theme ?? this.theme,
      language: language ?? this.language,
      enableNotifications: enableNotifications ?? this.enableNotifications,
      autoUpdate: autoUpdate ?? this.autoUpdate,
      preferredProtocol: preferredProtocol ?? this.preferredProtocol,
      serverConfig: serverConfig ?? this.serverConfig,
    );
  }

  @override
  bool operator ==(Object other) {
    if (identical(this, other)) return true;
    
    return other is AppSettings &&
        other.theme == theme &&
        other.language == language &&
        other.enableNotifications == enableNotifications &&
        other.autoUpdate == autoUpdate &&
        other.preferredProtocol == preferredProtocol &&
        other.serverConfig == serverConfig;
  }

  @override
  int get hashCode {
    return theme.hashCode ^
        language.hashCode ^
        enableNotifications.hashCode ^
        autoUpdate.hashCode ^
        preferredProtocol.hashCode ^
        serverConfig.hashCode;
  }
}

class ServerConfig {
  final String apiUrl;
  final bool isConnected;

  const ServerConfig({
    this.apiUrl = 'http://localhost:8081',
    this.isConnected = false,
  });

  ServerConfig copyWith({
    String? apiUrl,
    bool? isConnected,
  }) {
    return ServerConfig(
      apiUrl: apiUrl ?? this.apiUrl,
      isConnected: isConnected ?? this.isConnected,
    );
  }

  @override
  bool operator ==(Object other) {
    if (identical(this, other)) return true;
    
    return other is ServerConfig &&
        other.apiUrl == apiUrl &&
        other.isConnected == isConnected;
  }

  @override
  int get hashCode => apiUrl.hashCode ^ isConnected.hashCode;
}