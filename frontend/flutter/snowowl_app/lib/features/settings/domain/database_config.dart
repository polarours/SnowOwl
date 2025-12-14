import 'package:json_annotation/json_annotation.dart';

part 'database_config.g.dart';

@JsonSerializable()
class DatabaseConfig {
  final String host;
  final int port;
  final String databaseName;
  final String username;
  final String password;
  final bool isConnected;
  final String apiUrl;

  DatabaseConfig({
    this.host = 'localhost',
    this.port = 5432,
    this.databaseName = 'snowowl_dev',
    this.username = 'postgres',
    this.password = '',
    this.isConnected = false,
    this.apiUrl = 'http://localhost:8081',
  });

  DatabaseConfig copyWith({
    String? host,
    int? port,
    String? databaseName,
    String? username,
    String? password,
    bool? isConnected,
    String? apiUrl,
  }) {
    return DatabaseConfig(
      host: host ?? this.host,
      port: port ?? this.port,
      databaseName: databaseName ?? this.databaseName,
      username: username ?? this.username,
      password: password ?? this.password,
      isConnected: isConnected ?? this.isConnected,
      apiUrl: apiUrl ?? this.apiUrl,
    );
  }

  factory DatabaseConfig.fromJson(Map<String, dynamic> json) =>
      _$DatabaseConfigFromJson(json);

  Map<String, dynamic> toJson() => _$DatabaseConfigToJson(this);
  
  @override
  bool operator ==(Object other) {
    if (identical(this, other)) return true;
    
    return other is DatabaseConfig &&
      other.host == host &&
      other.port == port &&
      other.databaseName == databaseName &&
      other.username == username &&
      other.password == password &&
      other.isConnected == isConnected &&
      other.apiUrl == apiUrl;
  }

  @override
  int get hashCode {
    return host.hashCode ^
      port.hashCode ^
      databaseName.hashCode ^
      username.hashCode ^
      password.hashCode ^
      isConnected.hashCode ^
      apiUrl.hashCode;
  }
}