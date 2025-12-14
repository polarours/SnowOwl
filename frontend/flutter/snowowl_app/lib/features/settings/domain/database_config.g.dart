// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'database_config.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

DatabaseConfig _$DatabaseConfigFromJson(Map<String, dynamic> json) =>
    DatabaseConfig(
      host: json['host'] as String? ?? 'localhost',
      port: (json['port'] as num?)?.toInt() ?? 5432,
      databaseName: json['databaseName'] as String? ?? 'snowowl_dev',
      username: json['username'] as String? ?? 'postgres',
      password: json['password'] as String? ?? '',
      isConnected: json['isConnected'] as bool? ?? false,
      apiUrl: json['apiUrl'] as String? ?? 'http://localhost:8081',
    );

Map<String, dynamic> _$DatabaseConfigToJson(DatabaseConfig instance) =>
    <String, dynamic>{
      'host': instance.host,
      'port': instance.port,
      'databaseName': instance.databaseName,
      'username': instance.username,
      'password': instance.password,
      'isConnected': instance.isConnected,
      'apiUrl': instance.apiUrl,
    };
