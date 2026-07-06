// =====================================================================
// ThemeUtils.h
// ---------------------------------------------------------------------
// @brief   主题应用工具：根据 `ThemeMode` 构建 QPalette 并应用到 QApplication
// @details
//   - `buildThemePalette`  根据模式直接返回调色板（不修改全局状态）
//   - `applyApplicationTheme` 先把 style 切到 `Fusion`（跨平台外观一致），
//                            再设置调色板
//   - `resolvedThemeMode`（cpp 内部匿名）将 `System` 解析为 `Light` / `Dark`，
//     依据 `QStyleHints::colorScheme()`（Qt 6.5+）
// @layer   ui
// =====================================================================

#pragma once

#include <QPalette>

#include "ui/ThemeMode.h"

class QApplication;

inline constexpr auto kAccentColorHex = "#2d7ff9";

/// @brief 根据主题模式构建一个完整的 QPalette。
/// @param mode  `System` 会被解析为 `Light` / `Dark`
/// @return      对应主题的 QPalette
QPalette buildThemePalette(ThemeMode mode);

/// @brief 把 QApplication 的 style 切到 Fusion 并应用主题调色板。
/// @param application  目标 QApplication（通常就是 main 里的那个）
/// @param mode         主题模式
/// @note  Fusion 是 Qt 自带且跨平台外观最一致的样式；本项目强制使用它避免
///        不同平台原生样式导致的颜色不一致。
void applyApplicationTheme(QApplication& application, ThemeMode mode);
