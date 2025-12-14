import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'core/router/app_router.dart';
import 'features/settings/application/settings_providers.dart';
import 'features/settings/domain/settings_models.dart';
import 'theme/app_scroll_behavior.dart';
import 'theme/app_theme.dart';

class SnowOwlApp extends ConsumerWidget {
  const SnowOwlApp({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final router = ref.watch(appRouterProvider);
    final settings = ref.watch(settingsStateProvider);

    final themeMode = settings.theme == ThemePreference.dark
        ? ThemeMode.dark
        : ThemeMode.light;

    return MaterialApp.router(
      title: 'SnowOwl',
      routerConfig: router,
      theme: buildLightTheme(),
      darkTheme: buildDarkTheme(),
      themeMode: themeMode,
      debugShowCheckedModeBanner: false,
      scrollBehavior: const AppScrollBehavior(),
      localizationsDelegates: const [
        // GlobalMaterialLocalizations.delegate,
        // GlobalWidgetsLocalizations.delegate,
        // GlobalCupertinoLocalizations.delegate,
      ],
      supportedLocales: const [
        Locale('en'),
        Locale.fromSubtags(languageCode: 'zh'),
      ],
      locale: _localeFromString(settings.language),
    );
  }
  
  Locale? _localeFromString(String languageCode) {
    switch (languageCode) {
      case 'en':
        return const Locale('en');
      case 'zh':
        return const Locale.fromSubtags(languageCode: 'zh');
      default:
        return null;
    }
  }
}
