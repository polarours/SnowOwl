import 'package:flutter/widgets.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/features/settings/application/settings_providers.dart';

extension AppLocalizationsX on BuildContext {
  String _getLocaleText(String Function(bool isChinese) textSelector) {
    final container = ProviderScope.containerOf(this);
    final settings = container.read(settingsStateProvider);
    return textSelector(settings.language == 'zh');
  }
  
  String get deviceManagementTitle {
    return _getLocaleText((isChinese) => 
      isChinese ? '设备管理中心' : 'Device Management Center',);
  }
  
  String get deviceManagementDescription {
    return _getLocaleText((isChinese) => 
      isChinese ? '管理摄像头、传感器与边缘节点，统一查看在线状态与固件版本。' 
               : 'Manage cameras, sensors and edge nodes, view online status and firmware versions.',);
  }
  
  String get selectDeviceToViewDetails {
    return _getLocaleText((isChinese) => 
      isChinese ? '选择左侧设备查看详情' : 'Select a device from the left to view details',);
  }
  
  String get deviceTreeLoadError {
    return _getLocaleText((isChinese) => 
      isChinese ? '无法加载设备树' : 'Failed to load device tree',);
  }
  
  String get retry {
    return _getLocaleText((isChinese) => 
      isChinese ? '重试' : 'Retry',);
  }
  
  String get deviceDetailsLoadError {
    return _getLocaleText((isChinese) => 
      isChinese ? '加载设备详情失败' : 'Failed to load device details',);
  }
  
  String get reload {
    return _getLocaleText((isChinese) => 
      isChinese ? '重新加载' : 'Reload',);
  }
  
  String get configure {
    return _getLocaleText((isChinese) => 
      isChinese ? '配置' : 'Configure',);
  }
  
  String get tags {
    return _getLocaleText((isChinese) => 
      isChinese ? '标签' : 'Tags',);
  }
  
  String get history {
    return _getLocaleText((isChinese) => 
      isChinese ? '记录' : 'History',);
  }
  
  String get pushUpdate {
    return _getLocaleText((isChinese) => 
      isChinese ? '推送升级包' : 'Push Update',);
  }
  
  String get reboot {
    return _getLocaleText((isChinese) => 
      isChinese ? '重启设备' : 'Reboot',);
  }
  
  String get deviceID {
    return _getLocaleText((isChinese) => 
      isChinese ? '设备 ID' : 'Device ID',);
  }
  
  String get ipAddress {
    return _getLocaleText((isChinese) => 
      isChinese ? 'IP 地址' : 'IP Address',);
  }
  
  String get firmware {
    return _getLocaleText((isChinese) => 
      isChinese ? '固件版本' : 'Firmware',);
  }
  
  String get lastHeartbeat {
    return _getLocaleText((isChinese) => 
      isChinese ? '最近心跳' : 'Last Heartbeat',);
  }
  
  String get stream {
    return _getLocaleText((isChinese) => 
      isChinese ? '流媒体' : 'Stream',);
  }
  
  String get manufacturer {
    return _getLocaleText((isChinese) => 
      isChinese ? '制造商' : 'Manufacturer',);
  }
  
  String get model {
    return _getLocaleText((isChinese) => 
      isChinese ? '型号' : 'Model',);
  }
  
  String get registrationTime {
    return _getLocaleText((isChinese) => 
      isChinese ? '注册时间' : 'Registration Time',);
  }
  
  String get description {
    return _getLocaleText((isChinese) => 
      isChinese ? '描述' : 'Description',);
  }
  
  String get gateways {
    return _getLocaleText((isChinese) => 
      isChinese ? '网关设备' : 'Gateways',);
  }
  
  String get encoders {
    return _getLocaleText((isChinese) => 
      isChinese ? '编码器' : 'Encoders',);
  }
  
  String get cameras {
    return _getLocaleText((isChinese) => 
      isChinese ? '摄像头' : 'Cameras',);
  }
  
  String get sensors {
    return _getLocaleText((isChinese) => 
      isChinese ? '传感器' : 'Sensors',);
  }
  
  String get online {
    return _getLocaleText((isChinese) => 
      isChinese ? '在线' : 'Online',);
  }
  
  String get offline {
    return _getLocaleText((isChinese) => 
      isChinese ? '离线' : 'Offline',);
  }
  
  String get maintenance {
    return _getLocaleText((isChinese) => 
      isChinese ? '维护中' : 'Maintenance',);
  }
  
  String get justNow {
    return _getLocaleText((isChinese) => 
      isChinese ? '刚刚' : 'Just now',);
  }
  
  String minutesAgo(int minutes) {
    return _getLocaleText((isChinese) => 
      isChinese ? '$minutes 分钟前' : '$minutes min ago',);
  }
  
  String hoursAgo(int hours) {
    return _getLocaleText((isChinese) => 
      isChinese ? '$hours 小时前' : '$hours hours ago',);
  }
  
  String daysAgo(int days) {
    return _getLocaleText((isChinese) => 
      isChinese ? '$days 天前' : '$days days ago',);
  }
  
  String registeredDaysAgo(int days) {
    return _getLocaleText((isChinese) => 
      isChinese ? '$days 天前注册' : 'Registered $days days ago',);
  }
  
  String get recentlyRegistered {
    return _getLocaleText((isChinese) => 
      isChinese ? '最近注册' : 'Recently registered',);
  }
  
  String get dashboard {
    return _getLocaleText((isChinese) => 
      isChinese ? '仪表盘' : 'Dashboard',);
  }
  
  String get monitoring {
    return _getLocaleText((isChinese) => 
      isChinese ? '监控' : 'Monitoring',);
  }
  
  String get capture {
    return _getLocaleText((isChinese) => 
      isChinese ? '采集' : 'Capture',);
  }
  
  String get devices {
    return _getLocaleText((isChinese) => 
      isChinese ? '设备' : 'Devices',);
  }
  
  String get streams {
    return _getLocaleText((isChinese) => 
      isChinese ? '视频流' : 'Streams',);
  }
  
  String get alerts {
    return _getLocaleText((isChinese) => 
      isChinese ? '警报' : 'Alerts',);
  }
  
  String get settings {
    return _getLocaleText((isChinese) => 
      isChinese ? '设置' : 'Settings',);
  }
  
  String get monitoringCenter {
    return _getLocaleText((isChinese) => 
      isChinese ? '监控中心' : 'Monitoring Center',);
  }
  
  String get detectionEnabled {
    return _getLocaleText((isChinese) => 
      isChinese ? '检测已启用' : 'Detection Enabled',);
  }
  
  String get detectionDisabled {
    return _getLocaleText((isChinese) => 
      isChinese ? '检测已禁用' : 'Detection Disabled',);
  }
  
  String get toggleDetection {
    return _getLocaleText((isChinese) => 
      isChinese ? '切换检测' : 'Toggle Detection',);
  }
  
  String get monitoringCenterSubtitle {
    return _getLocaleText((isChinese) => 
      isChinese ? '实时查看所有摄像头和传感器数据流，监控网络状态和设备健康度。' 
               : 'View all camera and sensor data streams in real-time, monitor network status and device health.',);
  }
  
  String get onlineChannels {
    return _getLocaleText((isChinese) => 
      isChinese ? '在线通道' : 'Online Channels',);
  }
  
  String get totalBandwidth {
    return _getLocaleText((isChinese) => 
      isChinese ? '总带宽' : 'Total Bandwidth',);
  }
  
  String get backboneLinkStats {
    return _getLocaleText((isChinese) => 
      isChinese ? '骨干链路统计' : 'Backbone Link Stats',);
  }
  
  String get averageLatency {
    return _getLocaleText((isChinese) => 
      isChinese ? '平均延迟' : 'Average Latency',);
  }
  
  String get sampledOver60Seconds {
    return _getLocaleText((isChinese) => 
      isChinese ? '60秒采样' : 'Sampled over 60s',);
  }
  
  String get pendingAlerts {
    return _getLocaleText((isChinese) => 
      isChinese ? '待处理警报' : 'Pending Alerts',);
  }
  
  String get networkSummaryFailure {
    return _getLocaleText((isChinese) => 
      isChinese ? '网络摘要加载失败' : 'Network Summary Load Failure',);
  }
  
  String get networkSummaryFailureCaption {
    return _getLocaleText((isChinese) => 
      isChinese ? '无法从监控服务获取网络摘要数据，请检查连接状态。' 
               : 'Unable to fetch network summary data from monitoring service, please check connection status.',);
  }
  
  String get streamServiceUnavailable {
    return _getLocaleText((isChinese) => 
      isChinese ? '流媒体服务不可用' : 'Stream Service Unavailable',);
  }
  
  String get retryLoad {
    return _getLocaleText((isChinese) => 
      isChinese ? '重新加载' : 'Retry Load',);
  }
  
  String get enterFullscreen {
    return _getLocaleText((isChinese) => 
      isChinese ? '进入全屏' : 'Enter Fullscreen',);
  }
  
  String get jitter {
    return _getLocaleText((isChinese) => 
      isChinese ? '网络抖动' : 'Jitter',);
  }
  
  String get deviceOnline {
    return _getLocaleText((isChinese) => 
      isChinese ? '设备在线' : 'Device Online',);
  }
  
  String get deviceOffline {
    return _getLocaleText((isChinese) => 
      isChinese ? '设备离线' : 'Device Offline',);
  }
  
  String get videoStreams {
    return _getLocaleText((isChinese) => 
      isChinese ? '视频流' : 'Video Streams',);
  }
  
  String get videoStreamsDescription {
    return _getLocaleText((isChinese) => 
      isChinese ? '管理和监控所有视频流，启用实时检测算法。' 
               : 'Manage and monitor all video streams, enable real-time detection algorithms.',);
  }
  
  String get viewStream {
    return _getLocaleText((isChinese) => 
      isChinese ? '查看流' : 'View Stream',);
  }
  
  String get failedToLoadStreams {
    return _getLocaleText((isChinese) => 
      isChinese ? '加载视频流失败' : 'Failed to Load Streams',);
  }
  
  String get detectionControls {
    return _getLocaleText((isChinese) => 
      isChinese ? '检测控制' : 'Detection Controls',);
  }
  
  String get streamId {
    return _getLocaleText((isChinese) => 
      isChinese ? '流 ID' : 'Stream ID',);
  }
  
  String get streamUrl {
    return _getLocaleText((isChinese) => 
      isChinese ? '流地址' : 'Stream URL',);
  }
  
  String get protocol {
    return _getLocaleText((isChinese) => 
      isChinese ? '协议' : 'Protocol',);
  }
  
  // Add missing localization getters
  String get onlineDevices {
    return _getLocaleText((isChinese) => 
      isChinese ? '在线设备' : 'Online Devices',);
  }
  
  String get todayAlerts {
    return _getLocaleText((isChinese) => 
      isChinese ? '今日警报' : 'Today\'s Alerts',);
  }
  
  String get scheduledMaintenance {
    return _getLocaleText((isChinese) => 
      isChinese ? '计划维护' : 'Scheduled Maintenance',);
  }
  
  String get monitoringCenterQuickAction {
    return _getLocaleText((isChinese) => 
      isChinese ? '监控中心' : 'Monitoring Center',);
  }
  
  String get fieldCaptureStation {
    return _getLocaleText((isChinese) => 
      isChinese ? '现场采集站' : 'Field Capture Station',);
  }
  
  String get fieldCaptureSubtitle {
    return _getLocaleText((isChinese) => 
      isChinese ? '部署新的边缘采集设备，配置传感器网络拓扑。' : 'Deploy new edge capture devices, configure sensor network topology.',);
  }
  
  String get deviceDeployment {
    return _getLocaleText((isChinese) => 
      isChinese ? '设备部署' : 'Device Deployment',);
  }
  
  String get deviceDeploymentSubtitle {
    return _getLocaleText((isChinese) => 
      isChinese ? '批量配置新设备，自动注册至管理系统。' : 'Bulk configure new devices, automatically register to management system.',);
  }
  
  String get systemHealth {
    return _getLocaleText((isChinese) => 
      isChinese ? '系统健康' : 'System Health',);
  }
  
  String get systemHealthSubtitle {
    return _getLocaleText((isChinese) => 
      isChinese ? '检查各子系统运行状态，诊断潜在故障风险。' : 'Check subsystem operational status, diagnose potential failure risks.',);
  }
  
  String get uptimeRate {
    return _getLocaleText((isChinese) => 
      isChinese ? '运行率' : 'Uptime Rate',);
  }
  
  String get streamingSuccessRate {
    return _getLocaleText((isChinese) => 
      isChinese ? '流传输成功率' : 'Streaming Success Rate',);
  }
  
  String get addDevice {
    return _getLocaleText((isChinese) => 
      isChinese ? '添加设备' : 'Add Device',);
  }
  
  String get cancel {
    return _getLocaleText((isChinese) => 
      isChinese ? '取消' : 'Cancel',);
  }
  
  String get deviceCreatedSuccessfully {
    return _getLocaleText((isChinese) => 
      isChinese ? '设备创建成功' : 'Device created successfully',);
  }
  
  String get failedToCreateDevice {
    return _getLocaleText((isChinese) => 
      isChinese ? '设备创建失败' : 'Failed to create device',);
  }
  
  String get deleteDevice {
    return _getLocaleText((isChinese) => 
      isChinese ? '删除设备' : 'Delete Device',);
  }
  
  String get confirmDeleteDevice {
    return _getLocaleText((isChinese) => 
      isChinese ? '确定要删除此设备吗？此操作不可撤销。' : 'Are you sure you want to delete this device? This action cannot be undone.',);
  }
  
  String get deviceDeletedSuccessfully {
    return _getLocaleText((isChinese) => 
      isChinese ? '设备删除成功' : 'Device deleted successfully',);
  }
  
  String get failedToDeleteDevice {
    return _getLocaleText((isChinese) => 
      isChinese ? '设备删除失败' : 'Failed to delete device',);
  }
  
  String get deviceDoesNotSupportStreaming {
    return _getLocaleText((isChinese) => 
      isChinese ? '该设备不支持流媒体功能' : 'This device does not support streaming',);
  }
  
  String get appearance {
    return _getLocaleText((isChinese) => 
      isChinese ? '外观' : 'Appearance',);
  }
  
  String get themeMode {
    return _getLocaleText((isChinese) => 
      isChinese ? '主题模式' : 'Theme Mode',);
  }
  
  String get themeModeSubtitle {
    return _getLocaleText((isChinese) => 
      isChinese ? '选择应用程序的主题配色方案' : 'Choose the app\'s color scheme',);
  }
  
  String get system {
    return _getLocaleText((isChinese) => 
      isChinese ? '跟随系统' : 'System',);
  }
  
  String get light {
    return _getLocaleText((isChinese) => 
      isChinese ? '浅色' : 'Light',);
  }
  
  String get dark {
    return _getLocaleText((isChinese) => 
      isChinese ? '深色' : 'Dark',);
  }
  
  String get language {
    return _getLocaleText((isChinese) => 
      isChinese ? '语言' : 'Language',);
  }
  
  String get languageSubtitle {
    return _getLocaleText((isChinese) => 
      isChinese ? '更改应用程序显示语言' : 'Change the app display language',);
  }
  
  String get notificationsAndAutomation {
    return _getLocaleText((isChinese) => 
      isChinese ? '通知与自动化' : 'Notifications & Automation',);
  }
  
  String get pushAlerts {
    return _getLocaleText((isChinese) => 
      isChinese ? '推送警报' : 'Push Alerts',);
  }
  
  String get pushAlertsSubtitle {
    return _getLocaleText((isChinese) => 
      isChinese ? '接收重要事件的实时推送通知' : 'Receive real-time push notifications for important events',);
  }
  
  String get automaticFirmwareUpdates {
    return _getLocaleText((isChinese) => 
      isChinese ? '自动固件更新' : 'Automatic Firmware Updates',);
  }
  
  String get automaticFirmwareUpdatesSubtitle {
    return _getLocaleText((isChinese) => 
      isChinese ? '允许设备在空闲时段自动更新固件' : 'Allow devices to automatically update firmware during idle periods',);
  }
  
  String get streamingPreferences {
    return _getLocaleText((isChinese) => 
      isChinese ? '流媒体偏好' : 'Streaming Preferences',);
  }
  
  String get preferredProtocol {
    return _getLocaleText((isChinese) => 
      isChinese ? '首选协议' : 'Preferred Protocol',);
  }
  
  String get preferredProtocolSubtitle {
    return _getLocaleText((isChinese) => 
      isChinese ? '选择默认的流媒体传输协议' : 'Choose the default streaming protocol',);
  }
  
  String get lowLatency {
    return _getLocaleText((isChinese) => 
      isChinese ? '低延迟' : 'Low Latency',);
  }
  
  String get wideCompatibility {
    return _getLocaleText((isChinese) => 
      isChinese ? '广泛兼容' : 'Wide Compatibility',);
  }
  
  String get stableReliable {
    return _getLocaleText((isChinese) => 
      isChinese ? '稳定可靠' : 'Stable & Reliable',);
  }
  
  String get viewNetworkAssessment {
    return _getLocaleText((isChinese) => 
      isChinese ? '查看网络评估' : 'View Network Assessment',);
  }
  
  String get saveSettings {
    return _getLocaleText((isChinese) => 
      isChinese ? '保存设置' : 'Save Settings',);
  }
  
  String get settingsSavedSuccessfully {
    return _getLocaleText((isChinese) => 
      isChinese ? '设置保存成功' : 'Settings saved successfully',);
  }
  
  String get deviceType {
    return _getLocaleText((isChinese) => 
      isChinese ? '设备类型' : 'Device Type',);
  }
  
  String get headquartersCampus {
    return _getLocaleText((isChinese) => 
      isChinese ? '总部园区' : 'Headquarters Campus',);
  }
  
  String get buildingAMainControl {
    return _getLocaleText((isChinese) => 
      isChinese ? 'A栋中控' : 'Building A Main Control',);
  }
  
  String get buildingAEntrance1 {
    return _getLocaleText((isChinese) => 
      isChinese ? 'A栋 · 一号入口' : 'Building A Entrance 1',);
  }
  
  String get buildingAUndergroundGarage {
    return _getLocaleText((isChinese) => 
      isChinese ? 'A栋 · 地下车库' : 'Building A Underground Garage',);
  }
  
  String get buildingBLaboratory {
    return _getLocaleText((isChinese) => 
      isChinese ? 'B栋实验室' : 'Building B Laboratory',);
  }
  
  String get labGasSensor {
    return _getLocaleText((isChinese) => 
      isChinese ? '实验室 · 气体传感器' : 'Lab Gas Sensor',);
  }
  
  String get southChinaBranch {
    return _getLocaleText((isChinese) => 
      isChinese ? '华南分部' : 'South China Branch',);
  }
  
  String get storageNightVisionCamera {
    return _getLocaleText((isChinese) => 
      isChinese ? '仓储区 · 夜视摄像头' : 'Storage Night Vision Camera',);
  }
  
  String get storageTemperatureHumidity {
    return _getLocaleText((isChinese) => 
      isChinese ? '仓储区 · 温湿度' : 'Storage Temperature/Humidity',);
  }
  
  String get serverConnected {
    return _getLocaleText((isChinese) => 
      isChinese ? '服务器已连接' : 'Server Connected',);
  }
  
  String get serverDisconnected {
    return _getLocaleText((isChinese) => 
      isChinese ? '服务器断开连接' : 'Server Disconnected',);
  }
  
  String get serverNotConfigured {
    return _getLocaleText((isChinese) => 
      isChinese ? '服务器未配置' : 'Server Not Configured',);
  }
  
  String get intrusionDetectionGate1 {
    return _getLocaleText((isChinese) => 
      isChinese ? '入侵检测 · 一号门' : 'Intrusion Detection · Gate 1',);
  }
  
  String get aiDetectedSuspiciousLoitering {
    return _getLocaleText((isChinese) => 
      isChinese ? 'AI 检测到可疑人员长时间停留，已自动推送至巡逻机器人' 
               : 'AI detected suspicious loitering, automatically pushed to patrol robot',);
  }
  
  String get shanghaiHeadquarters {
    return _getLocaleText((isChinese) => 
      isChinese ? '上海 · 总部园区' : 'Shanghai · Headquarters',);
  }
  
  String get temperatureAnomalyRecovered {
    return _getLocaleText((isChinese) => 
      isChinese ? '温度异常回落' : 'Temperature Anomaly Recovered',);
  }
  
  String get coldChainTempReturnedNormal {
    return _getLocaleText((isChinese) => 
      isChinese ? '冷链仓温度恢复正常范围，系统自动解除告警' 
               : 'Cold chain temperature returned to normal range, system automatically cleared alert',);
  }
  
  String get shenzhenStorage {
    return _getLocaleText((isChinese) => 
      isChinese ? '深圳 · 仓储区' : 'Shenzhen · Storage Area',);
  }
  
  String get streamBitrateFluctuation {
    return _getLocaleText((isChinese) => 
      isChinese ? '推流码率抖动' : 'Stream Bitrate Fluctuation',);
  }
  
  String get networkQualityFluctuation {
    return _getLocaleText((isChinese) => 
      isChinese ? '采集端网络质量波动，建议切换 SRT 模式保障稳定性' 
               : 'Network quality fluctuation at capture end, recommend switching to SRT mode for stability',);
  }
  
  String get hangzhouFieldStation {
    return _getLocaleText((isChinese) => 
      isChinese ? '杭州 · 现场采集台' : 'Hangzhou · Field Station',);
  }
  
  String get cameraFirmwareUpdateAvailable {
    return _getLocaleText((isChinese) => 
      isChinese ? '摄像头固件更新可用' : 'Camera Firmware Update Available',);
  }
  
  String get firmwareUpdateAvailable {
    return _getLocaleText((isChinese) => 
      isChinese ? '发现 1.2.8 -> 1.3.0 发布笔记，建议窗口期升级提升低光性能' 
               : 'Found v1.2.8 -> v1.3.0 release notes, recommend upgrading during window period to improve low-light performance',);
  }
  
  String get nanjingLab {
    return _getLocaleText((isChinese) => 
      isChinese ? '南京 · 实验室' : 'Nanjing · Lab',);
  }
  
  String get acknowledged {
    return _getLocaleText((isChinese) => 
      isChinese ? '已确认' : 'Acknowledged',);
  }
  
  String get acknowledge {
    return _getLocaleText((isChinese) => 
      isChinese ? '确认' : 'Acknowledge',);
  }
  
  String get playback {
    return _getLocaleText((isChinese) => 
      isChinese ? '回放' : 'Playback',);
  }
  
  String get audio {
    return _getLocaleText((isChinese) => 
      isChinese ? '音频' : 'Audio',);
  }
  
  String get volume {
    return _getLocaleText((isChinese) => 
      isChinese ? '音量' : 'Volume',);
  }
  
  String get noiseReduction {
    return _getLocaleText((isChinese) => 
      isChinese ? '降噪' : 'Noise Reduction',);
  }
  
  String get echoCancellation {
    return _getLocaleText((isChinese) => 
      isChinese ? '回声消除' : 'Echo Cancellation',);
  }
  
  String get startingIntercomWith {
    return _getLocaleText((isChinese) => 
      isChinese ? '正在启动与' : 'Starting intercom with ',);
  }
  
  String get startIntercom {
    return _getLocaleText((isChinese) => 
      isChinese ? '启动对讲' : 'Start Intercom',);
  }
  
  String get audioSettingsAppliedTo {
    return _getLocaleText((isChinese) => 
      isChinese ? '音频设置已应用于' : 'Audio settings applied to ',);
  }
  
  String get apply {
    return _getLocaleText((isChinese) => 
      isChinese ? '应用' : 'Apply',);
  }
  
  String get deviceHasMicrophone {
    return _getLocaleText((isChinese) => 
      isChinese ? '麦克风可用' : 'Microphone available',);
  }
  
  String get audioControls {
    return _getLocaleText((isChinese) => 
      isChinese ? '音频控制' : 'Audio Controls',);
  }
  
  String get accessControl {
    return _getLocaleText((isChinese) => 
      isChinese ? '门禁' : 'Access Control',);
  }
  
  String get aiAnalytics {
    return _getLocaleText((isChinese) => 
      isChinese ? 'AI 分析' : 'AI Analytics',);
  }
  
  String get highPriority {
    return _getLocaleText((isChinese) => 
      isChinese ? '高优先级' : 'High Priority',);
  }
  
  String get parking {
    return _getLocaleText((isChinese) => 
      isChinese ? '停车场' : 'Parking',);
  }
  
  String get lowLight {
    return _getLocaleText((isChinese) => 
      isChinese ? '低光' : 'Low Light',);
  }
  
  String get backupChannel {
    return _getLocaleText((isChinese) => 
      isChinese ? '备用通道' : 'Backup Channel',);
  }
  
  String get nightVision {
    return _getLocaleText((isChinese) => 
      isChinese ? '夜视' : 'Night Vision',);
  }
  
  String get outdoor {
    return _getLocaleText((isChinese) => 
      isChinese ? '户外' : 'Outdoor',);
  }
  
  String get patrol {
    return _getLocaleText((isChinese) => 
      isChinese ? '巡逻' : 'Patrol',);
  }
  
  String get shanghaiUndergroundParking {
    return _getLocaleText((isChinese) => 
      isChinese ? '上海 · 张江高科 A1 · B2 停车区' : 'Shanghai · Zhangjiang Hi-Tech A1 · B2 Parking Area',);
  }
  
  String get openingRealTimeMonitoringFor {
    return _getLocaleText((isChinese) => 
      isChinese ? '正在打开设备' : 'Opening real-time monitoring for device',);
  }
  
  String get editScene {
    return _getLocaleText((isChinese) => 
      isChinese ? '编辑场景' : 'Edit Scene',);
  }
  
  String get sceneName {
    return _getLocaleText((isChinese) => 
      isChinese ? '场景名称' : 'Scene Name',);
  }
  
  String get enterANameForYourScene {
    return _getLocaleText((isChinese) => 
      isChinese ? '输入场景名称' : 'Enter a name for your scene',);
  }
  
  String get selectDevices {
    return _getLocaleText((isChinese) => 
      isChinese ? '选择设备:' : 'Select Devices:',);
  }
  
  String get failedToLoadDevices {
    return _getLocaleText((isChinese) => 
      isChinese ? '加载设备失败' : 'Failed to load devices',);
  }
  
  String get saveScene {
    return _getLocaleText((isChinese) => 
      isChinese ? '保存场景' : 'Save Scene',);
  }
  
  String get scene {
    return _getLocaleText((isChinese) => 
      isChinese ? '场景' : 'Scene',);
  }
  
  String get savedWith {
    return _getLocaleText((isChinese) => 
      isChinese ? '已保存，包含' : 'saved with',);
  }
  
  String get devicesCount {
    return _getLocaleText((isChinese) => 
      isChinese ? '个设备' : 'devices',);
  }
  
  String get sceneConfiguration {
    return _getLocaleText((isChinese) => 
      isChinese ? '场景配置' : 'Scene Configuration',);
  }
  
  String get availableDevices {
    return _getLocaleText((isChinese) => 
      isChinese ? '可用设备' : 'Available Devices',);
  }
  
  String get sceneDevices {
    return _getLocaleText((isChinese) => 
      isChinese ? '场景设备' : 'Scene Devices',);
  }
  
  String get filterDevices {
    return _getLocaleText((isChinese) => 
      isChinese ? '过滤设备' : 'Filter Devices',);
  }
  
  String get expandAll {
    return _getLocaleText((isChinese) => 
      isChinese ? '全部展开' : 'Expand All',);
  }
  
  String get collapseAll {
    return _getLocaleText((isChinese) => 
      isChinese ? '全部折叠' : 'Collapse All',);
  }
  
  String get reset {
    return _getLocaleText((isChinese) => 
      isChinese ? '重置' : 'Reset',);
  }
  
  String get file {
    return _getLocaleText((isChinese) => 
      isChinese ? '文件' : 'File',);
  }
  
  String get northWarehouseGate1 {
    return _getLocaleText((isChinese) => 
      isChinese ? '北仓库 · 一号门' : 'North Warehouse · Gate 1',);
  }
  
  String get shanghaiPudong {
    return _getLocaleText((isChinese) => 
      isChinese ? '上海 · 浦东新区' : 'Shanghai · Pudong',);
  }
  
  String get stable42Mbps {
    return _getLocaleText((isChinese) => 
      isChinese ? '稳定 · 4.2Mbps' : 'Stable · 4.2Mbps',);
  }
  
  String get warehouse {
    return _getLocaleText((isChinese) => 
      isChinese ? '仓库' : 'Warehouse',);
  }
  
  String get entranceExit {
    return _getLocaleText((isChinese) => 
      isChinese ? '进出口' : 'Entrance/Exit',);
  }
  
  String get buildingAControlRoom {
    return _getLocaleText((isChinese) => 
      isChinese ? 'A栋 · 中控大厅' : 'Building A · Control Room',);
  }
  
  String get hangzhouBinjiang {
    return _getLocaleText((isChinese) => 
      isChinese ? '杭州 · 滨江' : 'Hangzhou · Binjiang',);
  }
  
  String get jitter24Mbps {
    return _getLocaleText((isChinese) => 
      isChinese ? '抖动 · 2.4Mbps' : 'Jitter · 2.4Mbps',);
  }
  
  String get hall {
    return _getLocaleText((isChinese) => 
      isChinese ? '大厅' : 'Hall',);
  }
  
  String get southParkingLot {
    return _getLocaleText((isChinese) => 
      isChinese ? '南区 · 停车场' : 'South · Parking Lot',);
  }
  
  String get shenzhenBaoan {
    return _getLocaleText((isChinese) => 
      isChinese ? '深圳 · 宝安' : 'Shenzhen · Baoan',);
  }
  
  String get normal31Mbps {
    return _getLocaleText((isChinese) => 
      isChinese ? '正常 · 3.1Mbps' : 'Normal · 3.1Mbps',);
  }
  
  String get labVentilationDuct {
    return _getLocaleText((isChinese) => 
      isChinese ? '实验室 · 通风管道' : 'Lab · Ventilation Duct',);
  }
  
  String get nanjingJiangning {
    return _getLocaleText((isChinese) => 
      isChinese ? '南京 · 江宁' : 'Nanjing · Jiangning',);
  }
  
  String get offline0Kbps {
    return _getLocaleText((isChinese) => 
      isChinese ? '离线 · 0Kbps' : 'Offline · 0Kbps',);
  }
  
  String get laboratory {
    return _getLocaleText((isChinese) => 
      isChinese ? '实验室' : 'Laboratory',);
  }
  
  String get systemOverview {
    return _getLocaleText((isChinese) => 
      isChinese ? '系统概览' : 'System Overview',);
  }
  
  String get systemOverviewDescription {
    return _getLocaleText((isChinese) => 
      isChinese ? '实时监控仪表板，包含系统指标和警报' : 'Real-time monitoring dashboard with system metrics and alerts',);
  }
  
  String get sceneEditor {
    return _getLocaleText((isChinese) => 
      isChinese ? '场景编辑器' : 'Scene Editor',);
  }
  
  String get designFacilityLayouts {
    return _getLocaleText((isChinese) => 
      isChinese ? '设计设施布局' : 'Design facility layouts',);
  }
  
  String get warehouseScene {
    return _getLocaleText((isChinese) => 
      isChinese ? '仓库场景' : 'Warehouse Scene',);
  }
  
  String get virtualWarehouseLayout {
    return _getLocaleText((isChinese) => 
      isChinese ? '虚拟仓库布局' : 'Virtual warehouse layout',);
  }
  
  String get sceneSavedSuccessfully {
    return _getLocaleText((isChinese) => 
      isChinese ? '场景保存成功' : 'Scene saved successfully',);
  }
  
  String get selectRealDevice {
    return _getLocaleText((isChinese) => 
      isChinese ? '选择真实设备' : 'Select Real Device',);
  }
  
  String get realDevice {
    return _getLocaleText((isChinese) => 
      isChinese ? '真实设备' : 'Real Device',);
  }
  
  String get save {
    return _getLocaleText((isChinese) => 
      isChinese ? '保存' : 'Save',);
  }
  
  String get edit {
    return _getLocaleText((isChinese) => 
      isChinese ? '编辑' : 'Edit',);
  }
  
  String get name {
    return _getLocaleText((isChinese) => 
      isChinese ? '名称' : 'Name',);
  }
  
  String get camera {
    return _getLocaleText((isChinese) => 
      isChinese ? '摄像头' : 'Camera',);
  }
  
  String get sensor {
    return _getLocaleText((isChinese) => 
      isChinese ? '传感器' : 'Sensor',);
  }
  
  String get added {
    return _getLocaleText((isChinese) => 
      isChinese ? '已添加' : 'Added',);
  }
  
  String get deleted {
    return _getLocaleText((isChinese) => 
      isChinese ? '已删除' : 'Deleted',);
  }
  
  String get selected {
    return _getLocaleText((isChinese) => 
      isChinese ? '已选中' : 'Selected',);
  }
  
  String get selectionCleared {
    return _getLocaleText((isChinese) => 
      isChinese ? '已清除选择' : 'Selection cleared',);
  }
  
  String get areaToolActivated {
    return _getLocaleText((isChinese) => 
      isChinese ? '区域工具已激活' : 'Area tool activated',);
  }
  
  String get wallToolActivated {
    return _getLocaleText((isChinese) => 
      isChinese ? '墙体工具已激活' : 'Wall tool activated',);
  }
  
  String get backgroundImageSelectorNotImplemented {
    return _getLocaleText((isChinese) => 
      isChinese ? '背景图片选择器尚未实现' : 'Background image selector not implemented',);
  }
}