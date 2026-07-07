# 架构总览

项目按**依赖方向严格分四层**，下层绝不引用上层：

```
entry       app/main.cpp                          — QApplication 入口
  │
  ▼
ui          MainWindow                            — 菜单 / 工具栏 / 信号桥接
            MainWindowActions                     — 声明式菜单/工具栏组装 DSL
            PropertyPanel + GeometryFields        — 图形属性实时编辑器
            TutorialDialog + ManualContent        — 双语 HTML 手册
            ThemeMode / ThemeUtils                — 主题系统（Fusion 样式）

canvas      CanvasView (QGraphicsView)            — 工具状态机 + 鼠标事件分发
            DragCreationStrategy                  — 拖拽创建策略（Line/Rect/Circle/Ellipse）
            PathCreationStrategy                  — 多点创建策略（Polyline/Polygon）
            MultiShapeSession                     — 多选快照 / 变换撤销
            SelectionTransformOverlayItem         — 选择框覆盖层（4 手柄 + 虚线框）

graphics    ShapeItem (QGraphicsObject)           — 单图形绘制 + 命中测试
            ShapeFactory                          — 构造 / 克隆辅助

core        ShapeData                             — 数据模型 + JSON 序列化
            FileManager                           — 文档级 JSON I/O
            CanvasGeometry                        — 几何计算（帧变换、缩放、角度吸附）
            SelectionFrame                        — 有向包围盒 OBB
            AppLanguage / I18n                    — 国际化枚举 + 翻译表
            CanvasViewConstants                   — 全局常量
```

## 层间依赖规则

- **core**：零 Widgets 依赖，只依赖 `Qt6::Core` / `Qt6::Gui`。被所有上层引用。
- **graphics**：依赖 core。`ShapeItem` 持有 `ShapeData` 副本并负责渲染。
- **canvas**：依赖 core + graphics。`CanvasView` 协调工具、创建策略、选择系统。
- **ui**：依赖 core + canvas。`MainWindow` 组装菜单/工具栏，桥接信号。
- **entry**：依赖 ui。`main.cpp` 创建 `QApplication` 和 `MainWindow`。

`core` 被编译为**静态库** `vector_graphics_editor_core`，链接到主程序和所有测试。

## 设计模式

| 模式 | 应用位置 | 说明 |
|------|---------|------|
| 策略模式 | `ICreationStrategy` 体系 | 统一接口，两种创建策略，依赖注入通过 `CreationContext` 结构体解耦 |
| 工厂模式 | `ShapeFactory` | 集中创建 + 强制归一化；`cloneWithOffset` 封装"复制+平移+新 UUID+z 自增" |
| 快照模式 | `MultiShapeSession` | 变换/平移前拍摄完整数据快照，Esc 一键恢复 |
| 观察者模式 | Qt 信号槽 | `selectionStateChanged` / `shapeEdited` / `statusMessageChanged` / `shapeChanged` 4 条信号链 |
| 声明式 DSL | `MainWindowActions` | `ActionDescriptor` / `MenuDescriptor` / `ToolbarDescriptor` 数据驱动 UI 组装 |
| 防重入守卫 | `PropertyPanel::m_updatingWidgets` | 阻断 `setShapeData → emit → setShapeData` 循环 |

## 创建交互会话（三路互斥）

`CanvasView` 维护三条互斥的交互会话路径：

| 会话 | 工具 | 预览机制 | 提交 | 取消 |
|------|------|---------|------|------|
| 拖拽创建 | Line / Rect / Circle / Ellipse | `m_previewItem` 虚线预览 | 松鼠标 | 工具切换 / Esc |
| 路径创建 | Polyline / Polygon | `m_previewItem` 跟随鼠标 | Enter / 双击 | Esc |
| 平移会话 | Select（拖拽已选中） | `MultiShapeSession` 快照 | 松手提交 | Esc 恢复 |
| 变换会话 | Select（四角手柄） | `MultiShapeSession` 快照 | 松手提交 | Esc 恢复 |

## 关键数据流

### 属性编辑（实时反馈，无 Apply 按钮）

```
PropertyPanel 控件值改变
  → emit shapeEdited(data)
    → CanvasView::updateSelectedShape(data)
      → item->setShapeData(data) → 重绘
      → emit selectionStateChanged(item, 1)
    → PropertyPanel::setShapeData(data)
      → [m_updatingWidgets=true] 填充控件，不重复发射
```

### 平移已选中图形

```
mousePressEvent → armSelectionDrag               — 记录按下坐标
mouseMoveEvent  → beginSelectionDrag             — 快照所有选中项
                → updateSelectionDrag            — 实时写回 transform
mouseReleaseEvent → finishSelectionDrag          — 丢弃快照
```

### 缩放已选中图形

```
mousePressEvent → beginTransformSession(handle)   — 快照 + 记录锚点
mouseMoveEvent  → updateTransformSession          — 计算 delta transform 并写回
mouseReleaseEvent → finishTransformSession         — 提交
Esc → cancelTransformSession                      — 恢复快照
```
