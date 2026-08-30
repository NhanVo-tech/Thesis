/// Centralized color theme for Smart Car Access
/// This file serves as single source of truth for all colors used in the app

import 'package:flutter/material.dart';

class AppColors {
  // ============================================
  // PRIMARY COLORS
  // ============================================
  
  /// Deep Blue - Primary brand color
  static const Color primary = Color(0xFF273671);
  
  /// Sky Blue - Secondary brand color for accents
  static const Color secondary = Color(0xFF41a5de);
  
  /// Light Purple/Blue - Used for input backgrounds
  static const Color backgroundLight = Color(0xFFedf0f8);
  
  // ============================================
  // TEXT & TYPOGRAPHY
  // ============================================
  
  /// Dark text - Primary text color
  static const Color textPrimary = Color(0xFF273671);
  
  /// Gray text - Secondary text (labels, metadata)
  static const Color textSecondary = Color(0xFF8c8e98);
  
  /// Light gray - Hint text and placeholders
  static const Color textHint = Color(0xFFb2b7bf);
  
  /// White text - For light backgrounds
  static const Color textLight = Colors.white;
  
  // ============================================
  // BACKGROUND COLORS
  // ============================================
  
  /// Main app background
  static const Color background = Color(0xFFF5F5F5);
  
  /// Card/surface background
  static const Color surface = Colors.white;
  
  /// Subtle background for groups
  static const Color backgroundSubtle = Color(0xFFFAFAFA);
  
  // ============================================
  // STATUS COLORS
  // ============================================
  
  /// Success state (enable, online, unlocked)
  static const Color success = Color(0xFF4CAF50);
  
  /// Error state (failed action, error message)
  static const Color error = Color(0xFFE53935);
  
  /// Warning state (needs attention)
  static const Color warning = Color(0xFFFFC107);
  
  /// Info state (informational messages)
  static const Color info = Color(0xFF2196F3);
  
  // ============================================
  // SEMANTIC COLORS FOR CAR STATES
  // ============================================
  
  /// Car is locked/secure
  static const Color statusLocked = Color(0xFF4CAF50);
  
  /// Car is unlocked
  static const Color statusUnlocked = Color(0xFFFFC107);
  
  /// Car is online
  static const Color statusOnline = Color(0xFF4CAF50);
  
  /// Car is offline
  static const Color statusOffline = Color(0xFF9E9E9E);
  
  /// Battery low
  static const Color batteryLow = Color(0xFFF44336);
  
  /// Battery medium
  static const Color batteryMedium = Color(0xFFFFC107);
  
  /// Battery good
  static const Color batteryGood = Color(0xFF4CAF50);
  
  // ============================================
  // SHADOW & OVERLAY
  // ============================================
  
  /// Light shadow
  static const Color shadow = Color(0x1A000000);
  
  /// Medium shadow
  static const Color shadowMedium = Color(0x33000000);
  
  /// Dark overlay
  static const Color overlay = Color(0x80000000);
  
  // ============================================
  // GRADIENTS
  // ============================================
  
  /// Primary gradient - used for headers, cards
  static const LinearGradient primaryGradient = LinearGradient(
    begin: Alignment.topLeft,
    end: Alignment.bottomRight,
    colors: [primary, secondary],
  );
  
  /// Subtle gradient for backgrounds
  static const LinearGradient subtleGradient = LinearGradient(
    begin: Alignment.topLeft,
    end: Alignment.bottomRight,
    colors: [Color(0xFFEDEEF2), Color(0xFFF5F7FA)],
  );
  
  // ============================================
  // DISABLED/INACTIVE STATES
  // ============================================
  
  /// Disabled button color
  static const Color disabled = Color(0xFFBDBDBD);
  
  /// Disabled text color
  static const Color disabledText = Color(0xFF9E9E9E);
  
  // ============================================
  // DIVIDER & BORDER COLORS
  // ============================================
  
  /// Light divider
  static const Color divider = Color(0xFFE0E0E0);
  
  /// Primary border
  static const Color border = Color(0xFFD0D0D0);
  
  /// Subtle border (for cards)
  static const Color borderSubtle = Color(0xFFEEEEEE);
}

/// Material 3 Theme Data using AppColors
class AppTheme {
  static ThemeData get lightTheme {
    return ThemeData(
      useMaterial3: true,
      colorScheme: ColorScheme.fromSeed(
        seedColor: AppColors.primary,
        primary: AppColors.primary,
        secondary: AppColors.secondary,
        surface: AppColors.surface,
        error: AppColors.error,
      ),
      scaffoldBackgroundColor: AppColors.background,
      
      // AppBar Theme
      appBarTheme: const AppBarTheme(
        backgroundColor: AppColors.surface,
        elevation: 0,
        iconTheme: IconThemeData(color: AppColors.textPrimary),
        titleTextStyle: TextStyle(
          color: AppColors.textPrimary,
          fontSize: 24,
          fontWeight: FontWeight.bold,
        ),
      ),
      
      // Text Field Theme
      inputDecorationTheme: InputDecorationTheme(
        filled: true,
        fillColor: AppColors.backgroundLight,
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(30),
          borderSide: BorderSide.none,
        ),
        enabledBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(30),
          borderSide: BorderSide.none,
        ),
        focusedBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(30),
          borderSide: const BorderSide(
            color: AppColors.primary,
            width: 2,
          ),
        ),
        errorBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(30),
          borderSide: const BorderSide(
            color: AppColors.error,
            width: 2,
          ),
        ),
        hintStyle: const TextStyle(
          color: AppColors.textHint,
          fontSize: 16,
        ),
        contentPadding: const EdgeInsets.symmetric(
          horizontal: 20,
          vertical: 16,
        ),
      ),
      
      // Button Theme
      elevatedButtonTheme: ElevatedButtonThemeData(
        style: ElevatedButton.styleFrom(
          backgroundColor: AppColors.primary,
          foregroundColor: AppColors.textLight,
          padding: const EdgeInsets.symmetric(
            horizontal: 32,
            vertical: 16,
          ),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(30),
          ),
          textStyle: const TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.w600,
          ),
        ),
      ),
    );
  }
}
