# 国际化与主题系统

## 国际化

### 架构

- **语言枚举**：`AppLanguage { English, SimplifiedChinese }`
- **默认语言**：简体中文（`kDefaultLanguage`）
- **翻译接口**：`i18n::tr(language, key, en, zh)` — 不依赖 Qt Linguist / .ts 文件
- **翻译表**：`I18n.h` 集中维护 `shapeDisplayNames()`（7 条）和 `penStyleDisplayNames()`（4 条）

### 调用方式

```cpp
// 简单字面量
i18n::tr(m_language, "menu.file", "File", "文件");

// 使用翻译表
pickLocalized(language, "tool.circle", table[3]);
shapeDisplayName(ShapeType::Circle, language);
penStyleDisplayName(Qt::SolidLine, language);
```

### 持久化

- QSettings 键：`ui/language`
- 值：`"en"` / `"zh-CN"`
- `appLanguageFromSettingsValue("en")` → English，其余 → SimplifiedChinese

### 切换流程

1. 用户选择 Tutorial → Language → English / 简体中文
2. `MainWindow::setLanguage(language)` 调用
3. `saveLanguage()` 写入 QSettings
4. 通知 `CanvasView` / `PropertyPanel` / `TutorialDialog`
5. `retranslateUi()` 刷新所有硬编码字符串
6. 状态栏显示确认消息

### 新增翻译字符串

直接在调用处使用 `i18n::tr(...)`，无需修改翻译文件。

## 主题系统

### 枚举

```cpp
enum class ThemeMode : std::uint8_t { System, Light, Dark };
```

- `System`：跟随 OS 色觉设置（通过 `QStyleHints::colorScheme()`）
- `Light`：强制亮色调色板
- `Dark`：强制暗色调色板

### 实现

- `ThemeMode.h`：枚举 + QSettings 序列化函数
- `ThemeUtils.h/.cpp`：`buildThemePalette()` + `applyApplicationTheme()`
- 样式引擎：强制 `Fusion`（`application.setStyle("Fusion")`），跨平台一致

### 调色板

亮色/暗色两组色值在 `ThemeUtils.cpp` 匿名 namespace 中硬编码，覆盖所有 `QPalette::ColorRole`（Window、WindowText、Base、Button、Highlight 等 16 个角色）。

### 系统跟随

`MainWindow` 在构造时连接：

```cpp
connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, ..., [this](Qt::ColorScheme) {
    if (m_themeMode == ThemeMode::System) {
        applyApplicationTheme(*qApp, ThemeMode::System);
    }
});
```

### 持久化

- QSettings 键：`ui/themeMode`
- 值：`"system"` / `"light"` / `"dark"`

### 注意事项

- `m_selectionOverlay` 颜色非主题感知（始终 `#2d7ff9`）
- 画布背景始终白色（`m_scene->setBackgroundBrush(Qt::white)`）
