import 'dart:ui';

import 'package:flutter/material.dart';

import 'package:snow_owl/theme/app_colors.dart';

class GlassPanel extends StatelessWidget {
  const GlassPanel({
    super.key,
    required this.child,
    this.borderRadius = const BorderRadius.all(Radius.circular(24)),
    this.padding,
    this.margin,
    this.blurSigma = 18,
    this.fillColor,
    this.borderColor,
    this.overlayGradient,
    this.shadows,
  });

  final Widget child;
  final BorderRadiusGeometry borderRadius;
  final EdgeInsetsGeometry? padding;
  final EdgeInsetsGeometry? margin;
  final double blurSigma;
  final Color? fillColor;
  final Color? borderColor;
  final Gradient? overlayGradient;
  final List<BoxShadow>? shadows;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final isDark = theme.brightness == Brightness.dark;
    final resolvedFill =
        fillColor ?? (isDark ? AppColors.glassFill : AppColors.glassFillLight);
    final resolvedBorder = borderColor ??
        (isDark ? AppColors.glassBorder : AppColors.glassBorderLight);
    final resolvedGradient = overlayGradient ??
        (isDark
            ? const LinearGradient(
                colors: [
                  Color.fromRGBO(255, 255, 255, 0.12),
                  Color.fromRGBO(255, 255, 255, 0.02),
                ],
                begin: Alignment.topLeft,
                end: Alignment.bottomRight,
              )
            : const LinearGradient(
                colors: [
                  Color.fromRGBO(255, 255, 255, 0.82),
                  Color.fromRGBO(255, 255, 255, 0.46),
                ],
                begin: Alignment.topLeft,
                end: Alignment.bottomRight,
              ));
    final resolvedShadows = shadows ??
        (isDark
            ? const [
                BoxShadow(
                  color: Color.fromRGBO(0, 0, 0, 0.45),
                  blurRadius: 32,
                  spreadRadius: -12,
                  offset: Offset(0, 24),
                ),
              ]
            : const [
                BoxShadow(
                  color: Color.fromRGBO(99, 122, 192, 0.16),
                  blurRadius: 36,
                  spreadRadius: -18,
                  offset: Offset(0, 28),
                ),
              ]);

    Widget panel = ClipRRect(
      borderRadius: borderRadius,
      child: BackdropFilter(
        filter: ImageFilter.blur(sigmaX: blurSigma, sigmaY: blurSigma),
        child: Container(
          padding: padding,
          decoration: BoxDecoration(
            borderRadius: borderRadius,
            border: Border.all(color: resolvedBorder, width: 1),
            color: resolvedFill,
            boxShadow: resolvedShadows,
            gradient: resolvedGradient,
          ),
          child: child,
        ),
      ),
    );

    if (margin != null) {
      panel = Padding(
        padding: margin!,
        child: panel,
      );
    }

    return panel;
  }
}