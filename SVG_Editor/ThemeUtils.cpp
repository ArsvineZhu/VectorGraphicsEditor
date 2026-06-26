// =====================================================================
// ThemeUtils.cpp
// ---------------------------------------------------------------------
// @brief ThemeUtils.h 中声明方法的实现
// @details
//   - 调色板的所有色值（背景 / 前景 / 高亮 / 中间色阶）成对给出，
//     覆盖了项目 UI 中实际用到的所有 QPalette role。
//   - `resolvedThemeMode()` 是内部工具：把 `System` 解析为 Light / Dark，
//     供 `buildThemePalette` 复用。
//   - `themeModeToSettingsValue` / `themeModeFromSettingsValue` 的函数体
//     也放在本文件（虽声明在 ThemeMode.h），与 QSettings 持久化逻辑就近维护。
// =====================================================================

#include "ThemeUtils.h"

#include <QApplication>
#include <QGuiApplication>
#include <QStyleHints>

namespace {

/// @brief 亮色调色板：浅灰底 + 深色文本 + 蓝色高亮。
/// @note  暗色 / 亮色两组色值均为项目内自定，未引用任何外部主题包。
QPalette buildLightPalette() {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#f4f5f7"));
    palette.setColor(QPalette::WindowText, QColor("#1f2328"));
    palette.setColor(QPalette::Base, QColor("#ffffff"));
    palette.setColor(QPalette::AlternateBase, QColor("#eef1f4"));
    palette.setColor(QPalette::ToolTipBase, QColor("#ffffff"));
    palette.setColor(QPalette::ToolTipText, QColor("#1f2328"));
    palette.setColor(QPalette::Text, QColor("#1f2328"));
    palette.setColor(QPalette::Button, QColor("#ffffff"));
    palette.setColor(QPalette::ButtonText, QColor("#1f2328"));
    palette.setColor(QPalette::BrightText, QColor("#ffffff"));
    palette.setColor(QPalette::Highlight, QColor("#2d7ff9"));
    palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    palette.setColor(QPalette::Midlight, QColor("#d9dee3"));
    palette.setColor(QPalette::Mid, QColor("#c4cbd3"));
    palette.setColor(QPalette::Dark, QColor("#a7b1bc"));
    palette.setColor(QPalette::Shadow, QColor("#7f8b97"));
    return palette;
}

/// @brief 暗色调色板：深色背景 + 浅色文本 + 同一蓝色高亮。
QPalette buildDarkPalette() {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#1e1e1e"));
    palette.setColor(QPalette::WindowText, QColor("#f3f6f9"));
    palette.setColor(QPalette::Base, QColor("#252526"));
    palette.setColor(QPalette::AlternateBase, QColor("#2d2d30"));
    palette.setColor(QPalette::ToolTipBase, QColor("#2d2d30"));
    palette.setColor(QPalette::ToolTipText, QColor("#f3f6f9"));
    palette.setColor(QPalette::Text, QColor("#f3f6f9"));
    palette.setColor(QPalette::Button, QColor("#2d2d30"));
    palette.setColor(QPalette::ButtonText, QColor("#f3f6f9"));
    palette.setColor(QPalette::BrightText, QColor("#ffffff"));
    palette.setColor(QPalette::Highlight, QColor("#2d7ff9"));
    palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    palette.setColor(QPalette::Midlight, QColor("#3f3f46"));
    palette.setColor(QPalette::Mid, QColor("#56565f"));
    palette.setColor(QPalette::Dark, QColor("#151515"));
    palette.setColor(QPalette::Shadow, QColor("#000000"));
    palette.setColor(QPalette::PlaceholderText, QColor("#a0a8b2"));
    return palette;
}

/// @brief 把 `System` 解析为 `Light` / `Dark`。
/// @param mode  用户选择的主题模式
/// @return      解析后的最终模式（System 已被解析）
/// @note  依赖 `QGuiApplication::styleHints()`，调用方需保证 QGuiApplication 已构造。
ThemeMode resolvedThemeMode(ThemeMode mode) {
    if (mode != ThemeMode::System) {
        return mode;
    }

    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark ? ThemeMode::Dark : ThemeMode::Light;
}

} // namespace

QString themeModeToSettingsValue(ThemeMode mode) {
    // 字符串值刻意不与枚举顺序绑定：便于在用户配置文件中直接阅读。
    switch (mode) {
    case ThemeMode::System:
        return "system";
    case ThemeMode::Light:
        return "light";
    case ThemeMode::Dark:
        return "dark";
    }

    // default 兜底：新增枚举值时 GCC `-Wswitch` 会先在此处告警。
    return "system";
}

ThemeMode themeModeFromSettingsValue(const QString& value) {
    // 仅识别 "light" / "dark"；其他（包括 "system"、空串、未知）一律回退为 System。
    if (value == "light") {
        return ThemeMode::Light;
    }
    if (value == "dark") {
        return ThemeMode::Dark;
    }
    return ThemeMode::System;
}

QPalette buildThemePalette(ThemeMode mode) {
    return resolvedThemeMode(mode) == ThemeMode::Dark ? buildDarkPalette() : buildLightPalette();
}

void applyApplicationTheme(QApplication& application, ThemeMode mode) {
    // 切到 Fusion 样式：跨平台外观一致；不依赖系统原生样式
    application.setStyle("Fusion");
    application.setPalette(buildThemePalette(mode));
}
