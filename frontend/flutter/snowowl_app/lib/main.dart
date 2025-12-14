import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:media_kit/media_kit.dart';

import 'app.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  
  try {
    MediaKit.ensureInitialized();
  } catch (e) {
    // print('Note: MediaKit initialization error: $e');
  }
  
  runApp(
    const ProviderScope(
      child: SnowOwlApp(),
    ),
  );
}