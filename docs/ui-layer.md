# ui 层详解

## MainWindow — 主窗口

`MainWindow` 是应用唯一顶层 `QMainWindow`，职责：

- 持有 `CanvasView`（中央 widget）、`PropertyPanel`（右侧 dock）、`TutorialDialog`
- 通过 `MainWindowActions` DSL 组装菜单/工具栏
- 桥接 `CanvasView` 信号到 `PropertyPanel` / 状态栏
- 管理文件 I/O（通过 `FileManager`）
- 管理语言切换和主题切换

### 按钮/动作的 DSL 组装

`MainWindowActions` 提供声明式 DSL，避免 `MainWindow.cpp` 被重复样板淹没：

```cpp
struct ActionDescriptor {
    const char* id;
    std::optional<QKeySequence::StandardKey> shortcut;
    bool checkable = false;
    std::function<void(QAction&)> configure;
    std::function<void()> triggered;
};

struct MenuDescriptor { QMenu* menu; QList<MenuItemDescriptor> items; };
struct ToolbarDescriptor { QToolBar* toolbar; QList<MenuItemDescriptor> items; };

// 便捷构造
MenuItemDescriptor actionItem(QAction* action);
MenuItemDescriptor submenuItem(QMenu* submenu);
MenuItemDescriptor separatorItem();

// 批量填充
void setupActionsFromDescriptors(QWidget* owner, const QList<ActionDescriptor>&, ...);
void populateMenu(const MenuDescriptor& descriptor);
void populateToolbar(const ToolbarDescriptor& descriptor);
```

### 菜单结构

- File：New / Open / Save / Save As / Export Image / Exit
- Edit：Copy / Paste / Delete / Clear Canvas
- View：Toggle Properties Panel / Theme（System / Light / Dark）
- Tools：Selection / Open Shapes（Point, Line, Polyline） / Closed Shapes（Circle, Ellipse, Rectangle, Polygon）
- Tutorial：Operation Manual / Language（English / 简体中文）

### 工具栏

Select | Point, Line, Polyline | Circle, Ellipse, Rectangle, Polygon | Delete, Copy, Paste | Open, Save, Clear | Toggle Panel

### 信号桥接

```cpp
connect(m_canvasView, selectionStateChanged, ..., [this](ShapeItem* item, int count) {
    if (count > 1) m_propertyPanel->setMultipleSelection(count);
    else if (item) m_propertyPanel->setShapeData(item->shapeData());
    else m_propertyPanel->clearSelection();
});

connect(m_canvasView, statusMessageChanged, statusBar(), showMessage);

connect(m_propertyPanel, shapeEdited, m_canvasView, updateSelectedShape);

connect(m_canvasView, deleteSelectionRequested, this, deleteSelection);
```

## PropertyPanel — 属性面板

实时编辑器（无 Apply 按钮），关键设计：

### 防重入机制

```
setShapeData(data) [m_updatingWidgets=true]
  → widget->setValue(...)
    → valueChanged signal
      → emitEditedShape() [guards: m_updatingWidgets → return]
  → m_updatingWidgets=false
```

### 几何字段按类型

`PropertyPanelGeometryFields.h` 的 `geometryFieldsFor(type)` 查表：

```
GeometryFieldSet {
    vector<GeometryField> fields;                    // 标签 + 范围
    function<void(ShapeData&, array<double,6>&)> applyTo;  // 写回逻辑
}
```

6 个 `QDoubleSpinBox` 槽位复用：不同类型的控件可见数和标签根据查表结果切换。

### 编辑限制

- `m_hasSelection && m_currentData.transform.isIdentity()` 时才允许编辑几何参数
- 已变换图形仅支持样式编辑
- 多选时几何和样式均禁用

### 颜色预览

```cpp
QString colorButtonStyle(const QColor& color) {
    return QString("background:%1; border:1px solid #909090; min-height:24px;")
        .arg(color.name(QColor::HexArgb));
}
```

## TutorialDialog — 操作手册

- 模态 `QDialog`，内置 `QTextBrowser`
- 双语 HTML：`tutorial::kManualHtmlEn` / `tutorial::kManualHtmlZh`（`TutorialManualContent.h`）
- 内容：界面布局、绘图工具、Polyline/Polygon 详解、选择编辑、文件操作、快捷键、语言切换、当前边界

## 主题系统

参见 [i18n-and-theme.md](i18n-and-theme.md)。
