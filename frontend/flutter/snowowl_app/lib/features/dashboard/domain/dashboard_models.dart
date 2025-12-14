import 'package:flutter/material.dart';

class DashboardHeroMetric {
  const DashboardHeroMetric({
    required this.label,
    required this.value,
    required this.color,
  });

  final String label;
  final String value;
  final Color color;
}

class DashboardQuickAction {
  const DashboardQuickAction({
    required this.title,
    required this.subtitle,
    required this.icon,
    required this.color,
    this.routePath,
  });

  final String title;
  final String subtitle;
  final IconData icon;
  final Color color;
  final String? routePath;
}

enum MetricTrendDirection { up, down }

class DashboardSystemMetric {
  const DashboardSystemMetric({
    required this.title,
    required this.value,
    required this.trendLabel,
    required this.trendDirection,
  });

  final String title;
  final String value;
  final String trendLabel;
  final MetricTrendDirection trendDirection;
}

class DashboardSnapshot {
  const DashboardSnapshot({
    required this.heroMetrics,
    required this.quickActions,
    required this.systemMetrics,
  });

  final List<DashboardHeroMetric> heroMetrics;
  final List<DashboardQuickAction> quickActions;
  final List<DashboardSystemMetric> systemMetrics;
}
