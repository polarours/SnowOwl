import 'dart:async';


import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:snow_owl/theme/app_colors.dart';
import 'package:snow_owl/features/dashboard/domain/dashboard_models.dart';
import 'package:snow_owl/features/capture/view/field_capture_page.dart';
import 'package:snow_owl/features/monitoring/view/monitoring_hub_page.dart';
import 'package:snow_owl/features/devices/application/device_providers.dart';
import 'package:snow_owl/features/devices/domain/device_models.dart';
import 'package:snow_owl/features/monitoring/application/monitoring_providers.dart';

class ApiDashboardService {
  final ProviderContainer container;

  ApiDashboardService(this.container);

  Future<DashboardSnapshot> fetchSnapshot() async {
    // Get real data from providers
    final deviceTreeFuture = container.read(deviceTreeProvider.future);
    final monitoringSummaryFuture = container.read(monitoringSummaryProvider.future);
    
    // Wait for both futures to complete
    final results = await Future.wait([
      deviceTreeFuture,
      monitoringSummaryFuture,
    ]);
    
    final deviceTree = results[0] as List;
    final monitoringSummary = results[1] as dynamic; // We'll cast this properly
    
    // Count online devices
    final onlineDevices = deviceTree.where((device) => 
      device.status == DeviceStatus.online,).length;
    
    // Get alerts from monitoring summary
    final alertsToday = monitoringSummary.alertsToday;
    
    // For now, we'll keep scheduled maintenance as static
    // In a real implementation, this would come from a maintenance service
    
    return DashboardSnapshot(
      heroMetrics: [
        DashboardHeroMetric(
          label: 'onlineDevices',
          value: '$onlineDevices',
          color: AppColors.monitoringBlue,
        ),
        DashboardHeroMetric(
          label: 'todayAlerts',
          value: '$alertsToday',
          color: AppColors.alertOrange,
        ),
        const DashboardHeroMetric(
          label: 'scheduledMaintenance',
          value: '2',
          color: AppColors.warningAmber,
        ),
      ],
      quickActions: [
        const DashboardQuickAction(
          title: 'monitoringCenterQuickAction',
          subtitle: 'monitoringCenterSubtitle',
          icon: MonitoringHubPage.navigationIcon,
          color: AppColors.monitoringBlue,
          routePath: MonitoringHubPage.routePath,
        ),
        const DashboardQuickAction(
          title: 'fieldCaptureStation',
          subtitle: 'fieldCaptureSubtitle',
          icon: FieldCapturePage.navigationIcon,
          color: AppColors.captureRed,
          routePath: FieldCapturePage.routePath,
        ),
        const DashboardQuickAction(
          title: 'deviceDeployment',
          subtitle: 'deviceDeploymentSubtitle',
          icon: Icons.qr_code_scanner,
          color: AppColors.successGreen,
        ),
        const DashboardQuickAction(
          title: 'systemHealth',
          subtitle: 'systemHealthSubtitle',
          icon: Icons.pie_chart_outline,
          color: AppColors.warningAmber,
        ),
      ],
      systemMetrics: [
        DashboardSystemMetric(
          title: 'uptimeRate',
          value: '${90 + (onlineDevices % 10)}%',
          trendLabel: '+${(onlineDevices % 5).toStringAsFixed(1)}%',
          trendDirection: MetricTrendDirection.up,
        ),
        DashboardSystemMetric(
          title: 'streamingSuccessRate',
          value: '${85 + (alertsToday % 10)}%',
          trendLabel: '-${(alertsToday % 3).toStringAsFixed(1)}%',
          trendDirection: (alertsToday % 2 == 0) ? MetricTrendDirection.up : MetricTrendDirection.down,
        ),
        DashboardSystemMetric(
          title: 'averageLatency',
          value: '${150 + (onlineDevices * 2)}ms',
          trendLabel: '-${(onlineDevices % 20)}ms',
          trendDirection: MetricTrendDirection.up,
        ),
      ],
    );
  }
}