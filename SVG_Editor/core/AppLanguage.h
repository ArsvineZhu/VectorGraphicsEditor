// =====================================================================
// AppLanguage.h
// ---------------------------------------------------------------------
// @brief   应用界面语言枚举（i18n 切换的最小数据单元）
// @details 该枚举是项目内 i18n 切换的唯一语言标识；当前仅支持英文与简体中文。
//          实际界面文本由各 UI 模块持有的 `textForLanguage(language, "en", "zh")`
//          模式按需产出，不依赖 Qt Linguist / .ts 翻译文件。
// @layer   core（无 UI 依赖，可被任意层引用）
// @warning 枚举值的顺序即为序列化时写入的"语言代码序号"，
//          若新增语言请追加到末尾而非插队，避免 JSON / 配置中既有索引失效。
// =====================================================================

#pragma once

#include <QString>

#include <cstdint>

/// @brief 应用当前界面语言。
/// @note   字段宽度 8 bit 仅作预留，便于未来扩展到 256 种以内的语言集合；
///         UI 默认展示语言为简体中文（见 @ref appLanguageToSettingsValue）。
enum class AppLanguage : std::uint8_t {
    English,           ///< 英文
    SimplifiedChinese, ///< 简体中文（UI 默认）
};

inline constexpr AppLanguage kDefaultLanguage = AppLanguage::SimplifiedChinese;

/// @brief QSettings 字符串反序列化为 `AppLanguage`。
/// @param value  QSettings 取出的字符串；不识别值（包括缺失、空串、"zh-CN"）回退到 SimplifiedChinese
/// @return       解析得到的 AppLanguage
AppLanguage appLanguageFromSettingsValue(const QString& value);

/// @brief 把 `AppLanguage` 序列化为 QSettings 字符串。
/// @param language 目标语言
/// @return         "en" / "zh-CN"（与 fromSettingsValue 约定成对）
QString appLanguageToSettingsValue(AppLanguage language);
