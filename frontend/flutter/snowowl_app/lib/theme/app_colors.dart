import 'package:flutter/material.dart';

class AppColors {
  const AppColors._();

  // Dark Theme Colors
  static const Color backgroundDark = Color(0xFF1E1E1E); 
  static const Color backgroundSecondary = Color(0xFF252526); 
  static const Color backgroundTertiary = Color(0xFF2D2D30);
  static const Color surfaceDark = Color(0xFF3C3C3C); 
  
  // Light Theme Colors
  static const Color backgroundLight = Color(0xFFFFFFFF); 
  static const Color backgroundLightSecondary = Color(0xFFF3F3F3); 
  static const Color surfaceLight = Color(0xFFECECEC); 

  // Accent Colors
  static const Color monitoringBlue = Color(0xFF64D2FF);
  static const Color captureRed = Color(0xFFFF3B30);
  static const Color successGreen = Color(0xFF34C759);
  static const Color warningAmber = Color(0xFFFFCC00);
  static const Color alertOrange = Color(0xFFFF9500);

  // Glassmorphism Colors
  static const Color glassBorder = Color.fromRGBO(255, 255, 255, 0.08);
  static const Color glassFill = Color.fromRGBO(30, 30, 30, 0.7); 
  static const Color glassHighlight = Color.fromRGBO(255, 255, 255, 0.12);
  
  // Light Theme Glassmorphism Colors
  static const Color glassBorderLight = Color.fromRGBO(0, 0, 0, 0.08);
  static const Color glassFillLight = Color.fromRGBO(255, 255, 255, 0.85); 
  static const Color glassHighlightLight = Color.fromRGBO(0, 0, 0, 0.05);
}

extension ColorOpacity on Color {
  Color withOpacityPercent(double opacity) {
    final clamped = opacity.clamp(0.0, 1.0);
    final alpha = (clamped * 255).round();
    return withAlpha(alpha);
  }
}