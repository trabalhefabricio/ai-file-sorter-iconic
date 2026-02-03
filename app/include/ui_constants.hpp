#pragma once

/**
 * UI Constants - Centralized dimensions, colors, and styling values
 * 
 * This file consolidates all magic numbers and hardcoded values from the UI layer
 * to improve maintainability and enable easier theming.
 */

namespace ui {

// =============================================================================
// Window and Dialog Dimensions
// =============================================================================
namespace dimensions {
    // Main Application Window
    constexpr int kMainWindowWidth = 1000;
    constexpr int kMainWindowHeight = 800;
    
    // Dialogs
    constexpr int kCategorizationDialogWidth = 1100;
    constexpr int kCategorizationDialogHeight = 720;
    
    constexpr int kProgressDialogWidth = 800;
    constexpr int kProgressDialogHeight = 600;
    
    constexpr int kDryRunPreviewWidth = 900;
    constexpr int kDryRunPreviewHeight = 480;
    
    constexpr int kFileTinderMinWidth = 800;
    constexpr int kFileTinderMinHeight = 600;
    
    constexpr int kCacheManagerWidth = 550;
    constexpr int kCacheManagerHeight = 400;
    
    constexpr int kWhitelistEditorWidth = 700;
    constexpr int kWhitelistEditorHeight = 650;
    
    constexpr int kLLMSelectionDialogWidth = 620;
    
    constexpr int kAboutDialogWidth = 600;
    constexpr int kAboutDialogHeight = 420;
    
    constexpr int kLicenseDialogWidth = 520;
    constexpr int kLicenseDialogHeight = 320;
    
    constexpr int kStartupErrorMinWidth = 700;
    constexpr int kStartupErrorMinHeight = 500;
    
    // Preview areas
    constexpr int kPreviewMinHeight = 300;
    
    // Progress bar
    constexpr int kProgressBarMinHeight = 30;
}

// =============================================================================
// Colors - Light Theme
// =============================================================================
namespace colors {
    // Primary Actions
    constexpr const char* kSuccess = "#66bb6a";      // Green - Keep, Confirm
    constexpr const char* kSuccessHover = "#4caf50";
    constexpr const char* kDanger = "#ef5350";       // Red - Delete, Cancel
    constexpr const char* kDangerHover = "#e53935";
    constexpr const char* kPrimary = "#007aff";      // Blue - Primary action
    constexpr const char* kPrimaryHover = "#005ec7";
    constexpr const char* kNeutral = "#bdc3c7";      // Gray - Secondary
    constexpr const char* kNeutralHover = "#95a5a6";
    
    // Progress indicators
    constexpr const char* kProgressBar = "#4CAF50";
    constexpr const char* kProgressBorder = "#ccc";
    
    // Backgrounds
    constexpr const char* kBackground = "#f0f0f0";
    constexpr const char* kBackgroundAlt = "#f0f8ff";
    constexpr const char* kSurface = "#ffffff";
    
    // Text
    constexpr const char* kTextPrimary = "#333333";
    constexpr const char* kTextSecondary = "#555555";
    constexpr const char* kTextMuted = "#888888";
    constexpr const char* kTextOnDark = "#ffffff";
    
    // Status colors for LLM selection
    constexpr const char* kLLMRemote = "#1565c0";    // Blue for remote/cloud
    constexpr const char* kLLMLocal = "#2e7d32";     // Green for local
}

// =============================================================================
// Font Sizes
// =============================================================================
namespace fonts {
    constexpr int kTitleSize = 14;
    constexpr int kBodySize = 11;
    constexpr int kSmallSize = 10;
    constexpr int kButtonSize = 16;
}

// =============================================================================
// Padding and Margins
// =============================================================================
namespace spacing {
    constexpr int kSmall = 5;
    constexpr int kMedium = 10;
    constexpr int kLarge = 15;
    constexpr int kXLarge = 20;
    constexpr int kButtonPaddingV = 15;
    constexpr int kButtonPaddingH = 30;
}

// =============================================================================
// Border Radius
// =============================================================================
namespace borders {
    constexpr int kSmall = 3;
    constexpr int kMedium = 5;
    constexpr int kLarge = 8;
}

// =============================================================================
// Timeouts and Thresholds
// =============================================================================
namespace limits {
    constexpr int kMaxLabelLength = 80;
    constexpr int kSupportPromptIncrement = 200;
    constexpr int kDefaultBatchSize = 100;
    constexpr int kDefaultTokenLimit = 512;
    constexpr int kDefaultContextLength = 2048;
    constexpr int kExtendedContextLength = 8192;
    constexpr int kMaxPredictTokens = 64;
}

} // namespace ui
