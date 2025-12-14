import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';

import 'package:snow_owl/core/layout/app_breakpoints.dart';
import 'package:snow_owl/l10n/l10n.dart';

class AppShell extends StatelessWidget {
  const AppShell({super.key, required this.navigationShell});

  final StatefulNavigationShell navigationShell;

  static const _destinations = <_AppDestination>[
    _AppDestination(
      icon: Icons.dashboard_outlined,
      selectedIcon: Icons.dashboard,
      label: 'dashboard', 
    ),
    _AppDestination(
      icon: Icons.remove_red_eye_outlined,
      selectedIcon: Icons.remove_red_eye,
      label: 'monitoring', 
    ),
    _AppDestination(
      icon: Icons.videocam_outlined,
      selectedIcon: Icons.videocam,
      label: 'capture', 
    ),
    _AppDestination(
      icon: Icons.devices_other_outlined,
      selectedIcon: Icons.devices_other,
      label: 'devices', 
    ),
    _AppDestination(
      icon: Icons.video_library_outlined,
      selectedIcon: Icons.video_library,
      label: 'streams', 
    ),

    _AppDestination(
      icon: Icons.notifications_active_outlined,
      selectedIcon: Icons.notifications_active,
      label: 'alerts',
    ),
    _AppDestination(
      icon: Icons.settings_outlined,
      selectedIcon: Icons.settings,
      label: 'settings',
    ),
  ];

  @override
  Widget build(BuildContext context) {
    final size = MediaQuery.sizeOf(context);
    final isDesktop = size.width >= AppBreakpoints.desktop;
    final isTablet = size.width >= AppBreakpoints.tablet &&
        size.width < AppBreakpoints.desktop;

    if (isDesktop) {
      return Scaffold(
        body: Row(
          children: [
            SafeArea(
              child: NavigationRail(
                minWidth: 72,
                extended: size.width >= AppBreakpoints.desktopExpanded,
                labelType: size.width >= AppBreakpoints.desktopExpanded
                    ? NavigationRailLabelType.none
                    : NavigationRailLabelType.all,
                selectedIndex: navigationShell.currentIndex,
                onDestinationSelected: (index) => _onDestinationSelected(index),
                destinations: [
                  for (final destination in _destinations)
                    NavigationRailDestination(
                      icon: Icon(destination.icon),
                      selectedIcon:
                          Icon(destination.selectedIcon ?? destination.icon),
                      label: Text(_getLocalizedLabel(context, destination.label)),
                    ),
                ],
              ),
            ),
            const VerticalDivider(width: 1),
            Expanded(child: navigationShell),
          ],
        ),
      );
    }

    return Scaffold(
      body: navigationShell,
      bottomNavigationBar: SafeArea(
        top: false,
        child: NavigationBar(
          height: isTablet ? 80 : null,
          destinations: [
            for (final destination in _destinations)
              NavigationDestination(
                icon: Icon(destination.icon),
                selectedIcon:
                    Icon(destination.selectedIcon ?? destination.icon),
                label: _getLocalizedLabel(context, destination.label),
              ),
          ],
          selectedIndex: navigationShell.currentIndex,
          onDestinationSelected: (index) => _onDestinationSelected(index),
        ),
      ),
    );
  }

  String _getLocalizedLabel(BuildContext context, String key) {
    switch (key) {
      case 'dashboard':
        return context.dashboard;
      case 'monitoring':
        return context.monitoring;
      case 'capture':
        return context.capture;
      case 'devices':
        return context.devices;
      case 'streams':
        return context.streams;

      case 'alerts':
        return context.alerts;
      case 'settings':
        return context.settings;
      default:
        return key;
    }
  }

  void _onDestinationSelected(int index) {
    navigationShell.goBranch(
      index,
      initialLocation: index == navigationShell.currentIndex,
    );
  }
}

class _AppDestination {
  const _AppDestination({
    required this.icon,
    required this.label,
    this.selectedIcon,
  });

  final IconData icon;
  final IconData? selectedIcon;
  final String label;
}