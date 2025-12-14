import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import 'package:snow_owl/features/dashboard/view/dashboard_page.dart';
import 'package:snow_owl/features/dashboard/view/scene_editor_page.dart';
import 'package:snow_owl/features/monitoring/view/monitoring_hub_page.dart';
import 'package:snow_owl/features/capture/view/field_capture_page.dart';
import 'package:snow_owl/features/devices/view/devices_page.dart';
import 'package:snow_owl/features/alerts/view/alerts_page.dart';
import 'package:snow_owl/features/settings/view/settings_page.dart';
import 'package:snow_owl/features/streams/view/streams_page.dart';
import 'package:snow_owl/features/streams/view/stream_viewer_page.dart';
import 'package:snow_owl/features/streams/domain/stream_models.dart';
import 'package:snow_owl/features/shell/view/app_shell.dart';

final _rootNavigatorKey = GlobalKey<NavigatorState>();

final appRouterProvider = Provider<GoRouter>((ref) {
  return GoRouter(
    navigatorKey: _rootNavigatorKey,
    initialLocation: DashboardPage.routePath,
    routes: [
      StatefulShellRoute.indexedStack(
        builder: (context, state, navigationShell) =>
            AppShell(navigationShell: navigationShell),
        branches: [
          StatefulShellBranch(
            routes: [
              GoRoute(
                path: DashboardPage.routePath,
                name: DashboardPage.routeName,
                builder: (context, state) => const DashboardPage(),
              ),
            ],
          ),
          StatefulShellBranch(
            routes: [
              GoRoute(
                path: MonitoringHubPage.routePath,
                name: MonitoringHubPage.routeName,
                builder: (context, state) => const MonitoringHubPage(),
              ),
            ],
          ),
          StatefulShellBranch(
            routes: [
              GoRoute(
                path: FieldCapturePage.routePath,
                name: FieldCapturePage.routeName,
                builder: (context, state) => const FieldCapturePage(),
              ),
            ],
          ),
          StatefulShellBranch(
            routes: [
              GoRoute(
                path: DevicesPage.routePath,
                name: DevicesPage.routeName,
                builder: (context, state) => const DevicesPage(),
              ),
            ],
          ),
          StatefulShellBranch(
            routes: [
              GoRoute(
                path: StreamsPage.routePath,
                name: StreamsPage.routeName,
                builder: (context, state) => const StreamsPage(),
              ),
            ],
          ),

          StatefulShellBranch(
            routes: [
              GoRoute(
                path: AlertsPage.routePath,
                name: AlertsPage.routeName,
                builder: (context, state) => const AlertsPage(),
              ),
            ],
          ),
          StatefulShellBranch(
            routes: [
              GoRoute(
                path: SettingsPage.routePath,
                name: SettingsPage.routeName,
                builder: (context, state) => const SettingsPage(),
              ),
            ],
          ),
        ],
      ),
      GoRoute(
        path: SceneEditorPage.routePath,
        name: SceneEditorPage.routeName,
        builder: (context, state) => const SceneEditorPage(),
      ),
      GoRoute(
        path: StreamViewerPage.routePath,
        name: StreamViewerPage.routeName,
        builder: (context, state) {
          return StreamViewerPage(
            stream: VideoStream(
              id: 'placeholder',
              name: 'Placeholder Stream',
              url: 'rtmp://localhost/live/stream',
              protocol: StreamProtocol.rtmp,
              status: StreamStatus.online,
              location: 'Placeholder Location',
              tags: const [],
              lastUpdated: DateTime.now(),
            ),
          );
        },
      ),
    ],
  );
});