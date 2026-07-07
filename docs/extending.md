# 如何扩展

## 新增图形类型（以 Star 为例）

1. `core/ShapeData.h` — 在 `ShapeType` 枚举追加 `Star`
2. `core/ShapeData.cpp` — 添加 `"star"` 映射到 `shapeTypeToString()` / `shapeTypeFromString()`
3. `core/ShapeData.cpp` — 决定几何存储方式（`points` 或 `rect`），添加 `shapeDataToJson()` / `shapeDataFromJson()` 的序列化分支。若使用 `points`，在 `translateShapeData()` 中添加 case
4. `core/ShapeData.cpp` — 在 `shapeSupportsFill()` 中决定是否支持填充
5. `core/I18n.h` — 在 `shapeDisplayNames()` 表中追加 `{"Star", "星形"}`
6. `graphics/ShapeItem.cpp` — 在 `buildBasePath()` 添加 `case ShapeType::Star` 分支构造 `QPainterPath`
7. `ui/PropertyPanelGeometryFields.h` — 在 `kFields` 哈希表中新增 `ShapeType::Star` 条目
8. `canvas/CanvasView.h` — 如需专用工具，在 `Tool` 枚举追加 `Star`
9. `canvas/CanvasViewCore.cpp` — 在 `toolToShapeType()` 添加映射
10. `ui/MainWindow.cpp` — 添加 `createToolAction(CanvasView::Tool::Star)`
11. `tests/` — 添加测试用例到 `ShapeItemTests.cpp` 和 `ShapeDataTests.cpp`

## 新增创建策略

1. 新建 `NewStrategy.h/.cpp`，继承 `ICreationStrategy`
2. 实现 `begin/update/finish/cancel/inProgress/isPreviewItem` 六个纯虚函数
3. 在 `CanvasView.h` 中添加 `std::unique_ptr<NewStrategy> m_newStrategy`
4. 在 `CanvasView` 构造函数中创建策略（传入 `CreationContext`）
5. 在 `setTool()` / `cancelAllStrategies()` / 事件分发中添加对 `m_newStrategy` 的处理

## 新增菜单/工具栏动作

1. 在 `MainWindow::setupActions()` 的 `descriptors` 列表中添加 `ActionDescriptor`
2. 在 `registerAction` lambda 中将生成的动作赋值给成员指针（如 `m_myAction`）
3. 在 `MainWindow.h` 中声明相应 `QAction*` 成员
4. 在 `setupMenus()` 中使用 `actionItem(m_myAction)` 或 `submenuItem(...)` 添加到菜单
5. 在 `setupToolbar()` 中添加到工具栏
6. 在 `retranslateUi()` 中设置国际化文本

## 新增翻译语言

1. 在 `AppLanguage` 枚举末尾追加值（如 `Japanese`）
2. 在 `appLanguageFromSettingsValue()` / `appLanguageToSettingsValue()` 中添加映射
3. 扩展 `LocalizedString` 结构体（或新增参数）以容纳多语言
4. 修改 `i18n::tr()` 函数签名和实现
5. 更新所有翻译表（`shapeDisplayNames` / `penStyleDisplayNames`）
6. 在 `MainWindow` 语言菜单添加新 QAction

## 新增文件格式版本（`version 3`）

1. 在 `FileManager.cpp` 的加载路径中支持 `version == 2 || version == 3`
2. 在保存时始终输出最新版本号
3. 写入升级迁移逻辑（如 v2 图形的字段映射到 v3）
4. 更新 `FileManagerTests` 添加 v2→v3 迁移测试
5. 更新 `docs/file-format.md`
