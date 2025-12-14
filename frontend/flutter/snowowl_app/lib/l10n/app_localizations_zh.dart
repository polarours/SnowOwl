import 'app_localizations.dart';

class AppLocalizationsZh extends AppLocalizations {
  AppLocalizationsZh([super.locale = 'zh']);

  @override
  String get deviceManagementTitle => '设备管理中心';

  @override
  String get deviceManagementDescription => '管理摄像头、传感器和边缘节点，查看在线状态和固件版本。';

  @override
  String get selectDeviceToViewDetails => '从左侧选择设备以查看详情';

  @override
  String get deviceTreeLoadError => '加载设备树失败';

  @override
  String get retry => '重试';

  @override
  String get deviceDetailsLoadError => '加载设备详情失败';

  @override
  String get reload => '重新加载';

  @override
  String get configure => '配置';

  @override
  String get tags => '标签';

  @override
  String get history => '历史';

  @override
  String get pushUpdate => '推送更新';

  @override
  String get reboot => '重启';

  @override
  String get deviceID => '设备ID';

  @override
  String get ipAddress => 'IP地址';

  @override
  String get firmware => '固件';

  @override
  String get lastHeartbeat => '最后心跳';

  @override
  String get stream => '流媒体';

  @override
  String get manufacturer => '制造商';

  @override
  String get model => '型号';

  @override
  String get registrationTime => '注册时间';

  @override
  String get description => '描述';

  @override
  String get gateways => '网关';

  @override
  String get encoders => '编码器';

  @override
  String get cameras => '摄像头';

  @override
  String get sensors => '传感器';

  @override
  String get online => '在线';

  @override
  String get offline => '离线';

  @override
  String get maintenance => '维护';

  @override
  String get justNow => '刚刚';

  @override
  String minutesAgo(Object minutes) {
    return '$minutes分钟前';
  }

  @override
  String hoursAgo(Object hours) {
    return '$hours小时前';
  }

  @override
  String daysAgo(Object days) {
    return '$days天前';
  }

  @override
  String registeredDaysAgo(Object days) {
    return '$days天前注册';
  }

  @override
  String get recentlyRegistered => '最近注册';

  @override
  String get sceneEditor => '场景编辑器';

  @override
  String get designFacilityLayouts => '设计设施布局';
  
  @override
  String get devices => '设备';
  
  @override
  String get camera => '摄像头';
  
  @override
  String get sensor => '传感器';
  
  @override
  String get sceneSavedSuccessfully => '场景保存成功';
  
  @override
  String get wallToolActivated => '墙体工具已激活';
  
  @override
  String get areaToolActivated => '区域工具已激活';
  
  @override
  String get selectionCleared => '选择已清除';
  
  @override
  String get selected => '已选择';
  
  @override
  String get deleted => '已删除';
  
  @override
  String get added => '已添加';
}