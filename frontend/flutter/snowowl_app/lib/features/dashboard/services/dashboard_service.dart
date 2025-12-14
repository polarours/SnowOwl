import 'dart:async';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:snow_owl/theme/app_colors.dart';
import 'package:snow_owl/features/dashboard/domain/dashboard_models.dart';
import 'package:snow_owl/features/capture/view/field_capture_page.dart';
import 'package:snow_owl/features/monitoring/view/monitoring_hub_page.dart';

abstract class DashboardService {
  Future<DashboardSnapshot> fetchSnapshot();
}

class StaticDashboardService implements DashboardService {
  const StaticDashboardService();

  @override
  Future<DashboardSnapshot> fetchSnapshot() async {
    await Future<void>.delayed(const Duration(milliseconds: 250));

    return const DashboardSnapshot(
      heroMetrics: [
        DashboardHeroMetric(
          label: 'onlineDevices',
          value: '24',
          color: AppColors.monitoringBlue,
        ),
        DashboardHeroMetric(
          label: 'todayAlerts',
          value: '3',
          color: AppColors.alertOrange,
        ),
        DashboardHeroMetric(
          label: 'scheduledMaintenance',
          value: '2',
          color: AppColors.warningAmber,
        ),
      ],
      quickActions: [
        DashboardQuickAction(
          title: 'monitoringCenterQuickAction',
          subtitle: 'monitoringCenterSubtitle',
          icon: MonitoringHubPage.navigationIcon,
          color: AppColors.monitoringBlue,
          routePath: MonitoringHubPage.routePath,
        ),
        DashboardQuickAction(
          title: 'fieldCaptureStation',
          subtitle: 'fieldCaptureSubtitle',
          icon: FieldCapturePage.navigationIcon,
          color: AppColors.captureRed,
          routePath: FieldCapturePage.routePath,
        ),
        DashboardQuickAction(
          title: 'deviceDeployment',
          subtitle: 'deviceDeploymentSubtitle',
          icon: Icons.qr_code_scanner,
          color: AppColors.successGreen,
        ),
        DashboardQuickAction(
          title: 'systemHealth',
          subtitle: 'systemHealthSubtitle',
          icon: Icons.pie_chart_outline,
          color: AppColors.warningAmber,
        ),
      ],
      systemMetrics: [
        DashboardSystemMetric(
          title: 'uptimeRate',
          value: '96%',
          trendLabel: '+2.1%',
          trendDirection: MetricTrendDirection.up,
        ),
        DashboardSystemMetric(
          title: 'streamingSuccessRate',
          value: '92%',
          trendLabel: '-1.4%',
          trendDirection: MetricTrendDirection.down,
        ),
        DashboardSystemMetric(
          title: 'averageLatency',
          value: '180ms',
          trendLabel: '-35ms',
          trendDirection: MetricTrendDirection.up,
        ),
      ],
    );
  }
}

class ApiDashboardService implements DashboardService {
  final String baseUrl;

  ApiDashboardService({required this.baseUrl});

  @override
  Future<DashboardSnapshot> fetchSnapshot() async {
    try {
      final response = await http.get(Uri.parse('$baseUrl/api/v1/dashboard'));
      
      if (response.statusCode == 200) {
        final data = json.decode(response.body);
        
        final heroMetrics = <DashboardHeroMetric>[];
        if (data.containsKey('hero_metrics') && data['hero_metrics'] is List) {
          for (final metric in data['hero_metrics'] as List) {
            if (metric is Map<String, dynamic>) {
              heroMetrics.add(DashboardHeroMetric(
                label: metric['label'] as String,
                value: metric['value'] as String,
                color: _parseColor(metric['color'] as String?),
              ),);
            }
          }
        }
        
        final quickActions = <DashboardQuickAction>[];
        if (data.containsKey('quick_actions') && data['quick_actions'] is List) {
          for (final action in data['quick_actions'] as List) {
            if (action is Map<String, dynamic>) {
              quickActions.add(DashboardQuickAction(
                title: action['title'] as String,
                subtitle: action['subtitle'] as String,
                icon: _parseIcon(action['icon'] as String?),
                color: _parseColor(action['color'] as String?),
                routePath: action['route_path'] as String?,
              ),);
            }
          }
        }
        
        final systemMetrics = <DashboardSystemMetric>[];
        if (data.containsKey('system_metrics') && data['system_metrics'] is List) {
          for (final metric in data['system_metrics'] as List) {
            if (metric is Map<String, dynamic>) {
              systemMetrics.add(DashboardSystemMetric(
                title: metric['title'] as String,
                value: metric['value'] as String,
                trendLabel: metric['trend_label'] as String,
                trendDirection: _parseTrendDirection(metric['trend_direction'] as String?),
              ),);
            }
          }
        }
        
        return DashboardSnapshot(
          heroMetrics: heroMetrics,
          quickActions: quickActions,
          systemMetrics: systemMetrics,
        );
      } else {
        throw Exception('Failed to load dashboard: ${response.statusCode}');
      }
    } catch (e) {
      // print('Error fetching dashboard: $e');
      // Return zero-values instead of static data
      return const DashboardSnapshot(
        heroMetrics: [
          DashboardHeroMetric(
            label: 'onlineDevices',
            value: '0',
            color: AppColors.monitoringBlue,
          ),
          DashboardHeroMetric(
            label: 'todayAlerts',
            value: '0',
            color: AppColors.alertOrange,
          ),
          DashboardHeroMetric(
            label: 'scheduledMaintenance',
            value: '0',
            color: AppColors.warningAmber,
          ),
        ],
        quickActions: [
          DashboardQuickAction(
            title: 'monitoringCenterQuickAction',
            subtitle: 'monitoringCenterSubtitle',
            icon: MonitoringHubPage.navigationIcon,
            color: AppColors.monitoringBlue,
            routePath: MonitoringHubPage.routePath,
          ),
          DashboardQuickAction(
            title: 'fieldCaptureStation',
            subtitle: 'fieldCaptureSubtitle',
            icon: FieldCapturePage.navigationIcon,
            color: AppColors.captureRed,
            routePath: FieldCapturePage.routePath,
          ),
          DashboardQuickAction(
            title: 'deviceDeployment',
            subtitle: 'deviceDeploymentSubtitle',
            icon: Icons.qr_code_scanner,
            color: AppColors.successGreen,
          ),
          DashboardQuickAction(
            title: 'systemHealth',
            subtitle: 'systemHealthSubtitle',
            icon: Icons.pie_chart_outline,
            color: AppColors.warningAmber,
          ),
        ],
        systemMetrics: [
          DashboardSystemMetric(
            title: 'uptimeRate',
            value: '0%',
            trendLabel: '0%',
            trendDirection: MetricTrendDirection.up,
          ),
          DashboardSystemMetric(
            title: 'streamingSuccessRate',
            value: '0%',
            trendLabel: '0%',
            trendDirection: MetricTrendDirection.down,
          ),
          DashboardSystemMetric(
            title: 'averageLatency',
            value: '0ms',
            trendLabel: '0ms',
            trendDirection: MetricTrendDirection.up,
          ),
        ],
      );
    }
  }
  
  Color _parseColor(String? color) {
    if (color == null) return AppColors.monitoringBlue;
    
    switch (color) {
      case 'monitoringBlue':
        return AppColors.monitoringBlue;
      case 'alertOrange':
        return AppColors.alertOrange;
      case 'warningAmber':
        return AppColors.warningAmber;
      case 'successGreen':
        return AppColors.successGreen;
      case 'captureRed':
        return AppColors.captureRed;
      default:
        return AppColors.monitoringBlue;
    }
  }
  
  IconData _parseIcon(String? icon) {
    if (icon == null) return Icons.help_outline;
    
    switch (icon) {
      case 'monitoring':
        return MonitoringHubPage.navigationIcon;
      case 'capture':
        return FieldCapturePage.navigationIcon;
      case 'qr_code_scanner':
        return Icons.qr_code_scanner;
      case 'pie_chart':
        return Icons.pie_chart_outline;
      default:
        return Icons.help_outline;
    }
  }
  
  MetricTrendDirection _parseTrendDirection(String? direction) {
    switch (direction?.toLowerCase()) {
      case 'up':
        return MetricTrendDirection.up;
      case 'down':
        return MetricTrendDirection.down;
      default:
        return MetricTrendDirection.up;
    }
  }
}