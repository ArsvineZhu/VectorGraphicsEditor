// =====================================================================
// AppLanguage.cpp
// ---------------------------------------------------------------------
// @brief AppLanguage 序列化的实现
// @details 序列化字符串刻意不与枚举顺序绑定，便于在用户配置文件 / 注册表中
//          直接阅读，也避免新增枚举值后旧值错位。
// @layer   core
// =====================================================================

#include "core/AppLanguage.h"

AppLanguage appLanguageFromSettingsValue(const QString& value) {
    // 仅识别 "en"；其他（含缺失、空串、"zh-CN"、其他）一律回退到简体中文
    if (value == "en") {
        return AppLanguage::English;
    }
    return AppLanguage::SimplifiedChinese;
}

QString appLanguageToSettingsValue(AppLanguage language) {
    switch (language) {
    case AppLanguage::English:
        return "en";
    case AppLanguage::SimplifiedChinese:
        return "zh-CN";
    }
    // default 兜底：新增枚举值时 GCC -Wswitch 会先在此处告警
    return "zh-CN";
}
