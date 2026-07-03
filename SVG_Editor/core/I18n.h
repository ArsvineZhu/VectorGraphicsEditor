// =====================================================================
// I18n.h
// ---------------------------------------------------------------------
// @brief   i18n 统一入口 + 集中翻译表
// @details 原本 4 个 UI 模块各自持有 `textForLanguage(language, english, chinese)`
//          签名不一致的 helper，且 3 处翻译表（shapeDisplayName / penStyleDisplayName
//          / toolLabel）互相独立、容易漂移。本文件提供：
//          1. `tr(AppLanguage, key, english, chinese)`：统一文本挑选入口；
//          2. 集中翻译表：所有"枚举 → (en, zh)"映射统一在此声明。
//          不依赖 Qt Linguist / .ts 翻译文件；未来若迁移到 Qt 标准体系，
//          只需把 tr 的实现替换为 QObject::tr()。
// @layer   core（无 UI 依赖，可被任意层引用）
// =====================================================================

#pragma once

#include <QString>

#include <array>
#include <string_view>

#include "core/AppLanguage.h"
#include "core/ShapeData.h"

class QPointF;
class QRectF;
class QTransform;

namespace i18n {

/// @brief 二选一返回字符串（统一入口）。
/// @param language 目标语言
/// @param key      字符串 key（仅用于未来迁移到 .ts 时追踪；当前不影响返回值）
/// @param english  英文原文
/// @param chinese  简体中文原文
QString tr(AppLanguage language, std::string_view key, const QString& english, const QString& chinese);

/// @brief 同一 helper 的 const char* 重载，避免调用方在传字面量时手写 QString::fromUtf8。
inline QString tr(AppLanguage language, std::string_view key, const char* english, const char* chinese) {
    return tr(language, key, QString::fromUtf8(english), QString::fromUtf8(chinese));
}

} // namespace i18n

/// @brief (英文, 中文) 二元组的中央定义
struct LocalizedString {
    QString english;
    QString chinese;
};

/// @brief 用 `tr` + LocalizedString 挑选当前语言文本（替代分散的 `textForLanguage` 调用）
inline QString pickLocalized(AppLanguage language, std::string_view key, const LocalizedString& value) {
    return i18n::tr(language, key, value.english, value.chinese);
}

namespace i18n {

/// @brief 图形类别 → (英文, 中文) 翻译表
inline const std::array<LocalizedString, 7>& shapeDisplayNames() {
    static const std::array<LocalizedString, 7> table = {{
        {QStringLiteral("Point"), QStringLiteral("点")},
        {QStringLiteral("Line"), QStringLiteral("直线")},
        {QStringLiteral("Polyline"), QStringLiteral("折线")},
        {QStringLiteral("Circle"), QStringLiteral("圆")},
        {QStringLiteral("Ellipse"), QStringLiteral("椭圆")},
        {QStringLiteral("Rectangle"), QStringLiteral("矩形")},
        {QStringLiteral("Polygon"), QStringLiteral("多边形")},
    }};
    return table;
}

/// @brief 描边线型 → (英文, 中文) 翻译表
inline const std::array<LocalizedString, 4>& penStyleDisplayNames() {
    static const std::array<LocalizedString, 4> table = {{
        {QStringLiteral("Solid"), QStringLiteral("实线")},
        {QStringLiteral("Dash"), QStringLiteral("虚线")},
        {QStringLiteral("Dot"), QStringLiteral("点线")},
        {QStringLiteral("Dash Dot"), QStringLiteral("点划线")},
    }};
    return table;
}

} // namespace i18n
