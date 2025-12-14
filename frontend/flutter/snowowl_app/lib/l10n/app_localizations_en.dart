import 'app_localizations.dart';

class AppLocalizationsEn extends AppLocalizations {
  AppLocalizationsEn([super.locale = 'en']);

  @override
  String get deviceManagementTitle => 'Device Management Center';

  @override
  String get deviceManagementDescription => 'Manage cameras, sensors and edge nodes, view online status and firmware versions.';

  @override
  String get selectDeviceToViewDetails => 'Select a device from the left to view details';

  @override
  String get deviceTreeLoadError => 'Failed to load device tree';

  @override
  String get retry => 'Retry';

  @override
  String get deviceDetailsLoadError => 'Failed to load device details';

  @override
  String get reload => 'Reload';

  @override
  String get configure => 'Configure';

  @override
  String get tags => 'Tags';

  @override
  String get history => 'History';

  @override
  String get pushUpdate => 'Push Update';

  @override
  String get reboot => 'Reboot';

  @override
  String get deviceID => 'Device ID';

  @override
  String get ipAddress => 'IP Address';

  @override
  String get firmware => 'Firmware';

  @override
  String get lastHeartbeat => 'Last Heartbeat';

  @override
  String get stream => 'Stream';

  @override
  String get manufacturer => 'Manufacturer';

  @override
  String get model => 'Model';

  @override
  String get registrationTime => 'Registration Time';

  @override
  String get description => 'Description';

  @override
  String get gateways => 'Gateways';

  @override
  String get encoders => 'Encoders';

  @override
  String get cameras => 'Cameras';

  @override
  String get sensors => 'Sensors';

  @override
  String get online => 'Online';

  @override
  String get offline => 'Offline';

  @override
  String get maintenance => 'Maintenance';

  @override
  String get justNow => 'Just now';

  @override
  String minutesAgo(Object minutes) {
    return '$minutes min ago';
  }

  @override
  String hoursAgo(Object hours) {
    return '$hours hours ago';
  }

  @override
  String daysAgo(Object days) {
    return '$days days ago';
  }

  @override
  String registeredDaysAgo(Object days) {
    return 'Registered $days days ago';
  }

  @override
  String get recentlyRegistered => 'Recently registered';

  @override
  String get sceneEditor => 'Scene Editor';

  @override
  String get designFacilityLayouts => 'Design facility layouts';
  
  @override
  String get devices => 'Devices';
  
  @override
  String get camera => 'Camera';
  
  @override
  String get sensor => 'Sensor';
  
  @override
  String get sceneSavedSuccessfully => 'Scene saved successfully';
  
  @override
  String get wallToolActivated => 'Wall tool activated';
  
  @override
  String get areaToolActivated => 'Area tool activated';
  
  @override
  String get selectionCleared => 'Selection cleared';
  
  @override
  String get selected => 'Selected';
  
  @override
  String get deleted => 'Deleted';
  
  @override
  String get added => 'Added';
}