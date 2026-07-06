// =====================================================================
// I18n.cpp
// ---------------------------------------------------------------------
// @brief I18n.h 中 tr 的实现
// @details 当前实现仅根据 AppLanguage 二选一；未来若要切到 Qt 标准 i18n，
//          在此把 tr 替换为 QObject::tr() 调用即可，调用方不需要改动。
// @layer   core
// =====================================================================

#include "core/I18n.h"

namespace i18n {

QString tr(AppLanguage language, std::string_view /*key*/, const QString& english, const QString& chinese) {
    return language == AppLanguage::SimplifiedChinese ? chinese : english;
}

} // namespace i18n
