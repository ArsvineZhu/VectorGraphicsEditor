// =====================================================================
// ThemeMode.h
// ---------------------------------------------------------------------
// @brief   主题模式枚举 + 与 QSettings 之间的字符串互转辅助
// @details
//   - `System` 表示「跟随系统 / 风格提示」（Qt 6.5+ 引入的 `QStyleHints::colorScheme()`）。
//   - `Light` / `Dark` 表示用户强制指定亮 / 暗主题。
//   - 持久化使用字符串 "system" / "light" / "dark"（不是数字），
//     便于在用户配置文件 / 注册表中人读，也避免新增枚举值后旧值错位。
// @layer   ui（纯数据 + 序列化，不依赖 Widgets 之外的 Qt 模块）
// =====================================================================

#pragma once

#include <QString>

#include <cstdint>

/// @brief 应用主题模式。
enum class ThemeMode : std::uint8_t {
    System, ///< 跟随系统：调用方需用 `QStyleHints::colorScheme()` 解析为 Light / Dark
    Light,  ///< 强制亮色
    Dark,   ///< 强制暗色
};

/// @brief 将 `ThemeMode` 序列化为 `QSettings` 字符串。
/// @param mode  主题模式
/// @return      "system" / "light" / "dark"（默认兜底 "system"）
/// @note  新增枚举值时务必同步本函数；当前 GCC `-Wswitch` 在 default 分支兜底。
QString themeModeToSettingsValue(ThemeMode mode);

/// @brief `QSettings` 字符串反序列化为 `ThemeMode`。
/// @param value  从 `QSettings` 取出的字符串（不区分大小写比较会保持简单）
/// @return       "light" → Light；"dark" → Dark；其他（含 "system"、空串、未知）一律回退为 System
ThemeMode themeModeFromSettingsValue(const QString& value);
