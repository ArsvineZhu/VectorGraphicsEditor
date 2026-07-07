# canvas 层详解

`CanvasView` 是项目最核心的交互协调者，文件拆分：

| 文件 | 职责 |
|------|------|
| `CanvasView.h` | 类声明，`Tool` 枚举，信号，成员 |
| `CanvasViewCore.cpp` | 构造/析构、工具切换、增删、复制粘贴、文档 I/O、导出、z 管理 |
| `CanvasViewInput.cpp` | 鼠标/键盘事件分发 |
| `CanvasViewSelection.cpp` | 选择管理、覆盖层更新、变换/平移会话 |

## 工具模型

```cpp
enum class Tool : std::uint8_t { Select, Point, Line, Polyline, Circle, Ellipse, Rectangle, Polygon };
```

工具切换（`setTool`）会：
1. 取消当前选择拖动
2. 取消当前变换会话
3. 取消所有创建策略
4. 更新 RubberBand 拖动模式（仅 Select 工具启用）
5. 刷新选择装饰和覆盖层
6. 发出状态栏消息

## 事件路由（`CanvasViewInput.cpp`）

```
mousePressEvent:
  Select + 命中 overlay 手柄 → beginTransformSession
  Point 工具 → addShape(Point) 即时创建
  Line/Rect/Circle/Ellipse → m_dragStrategy->begin()
  Polyline/Polygon → activePathStrategy()->begin()
  否则 → QGraphicsView::mousePressEvent → armSelectionDrag(若按下已选中)

mouseMoveEvent:
  transformSession active → updateTransformSession
  selectionDragSession active → updateSelectionDrag
  dragStrategy inProgress → m_dragStrategy->update()
  pathStrategy inProgress → strategy->update()
  Select + 左键 → 超阈值 → beginSelectionDrag

mouseReleaseEvent:
  left + transformSession → finishTransformSession
  left + dragStrategy → m_dragStrategy->finish()
  left + selectionDragSession → finishSelectionDrag

keyPressEvent:
  Enter → 路径策略 finish
  Esc → cancelSelectionDrag / cancelTransformSession / cancelAllStrategies
  Delete/Backspace → emit deleteSelectionRequested
  Ctrl+C → copySelectedItem
  Ctrl+V → pasteCopiedItem
```

## 选择系统（`CanvasViewSelection.cpp`）

- 单选：`selectedShapeItem()` 返回唯一 `ShapeItem`
- 多选：`selectedShapeItems()` 从 scene 选集中过滤
- 选择变化：`handleSelectionChanged()` → 归一化变换 → 刷新通知
- 选择框：`buildSelectionFrameFromSelection()` — 单选保留局部轴，多选退回联合 AABB
- 覆盖层：`updateSelectionOverlay()` — 仅 Select 工具下可见，非 Select 工具回退到 item 自绘

## 平移会话

```
armSelectionDrag(pos, scenePoint)   — 记录按下坐标
beginSelectionDrag()                — MultiShapeSession 快照
updateSelectionDrag(scenePoint)     — delta 应用到每个 transform
finishSelectionDrag()               — 丢弃快照
cancelSelectionDrag()               — 恢复快照（Esc / 工具切换）
```

## 变换会话（缩放）

```
beginTransformSession(handle, point)  — 快照 + 记录锚点
updateTransformSession(point, mod)    — scaledFrameFromHandle → delta transform → 写回
finishTransformSession()              — 提交（单选时丢弃 frame 缓存）
cancelTransformSession()              — MultiShapeSession::cancel() 恢复快照
```

### 单选 vs 多选

- 单选：`transformBetweenFrames(localFrame, targetFrame)` 直接写回 `ShapeData::transform`
- 多选：每个图形的 `shapeData.transform *= deltaTransform`

## 创建策略

### ICreationStrategy（抽象接口）

```cpp
class ICreationStrategy {
public:
    virtual ~ICreationStrategy() = default;
    virtual void begin(const QPointF& scenePoint) = 0;
    virtual void update(const QPointF& scenePoint) = 0;
    virtual void finish(const QPointF& scenePoint) = 0;
    virtual void cancel() = 0;
    virtual bool inProgress() const = 0;
    virtual bool isPreviewItem(const ShapeItem* item) const = 0;
};
```

### DragCreationStrategy（拖拽创建）

- 工具：Line / Rectangle / Circle / Ellipse
- begin：记录起点，创建虚线预览 ShapeItem
- update：实时更新预览几何
- finish：检查有效性（非退化），提交最终图形
- 有效性：Line 长度 > 1px，Rectangle/Circle/Ellipse 宽高 > 1px

### PathCreationStrategy（多点创建）

- 工具：Polyline / Polygon
- begin：首次创建预览 Item；后续调用累加顶点
- update：从首个顶点到鼠标当前位置实时连线
- finish：检查最小顶点数（折线 ≥ 2，多边形 ≥ 3），提交
- 结束方式：Enter / 双击；取消方式：Esc

### CreationContext（依赖注入）

```cpp
struct CreationContext {
    std::function<qreal()> nextZValue;
    std::function<ShapeStyle(ShapeType)> currentStyleFor;
    std::function<void(const ShapeData&, bool)> addShape;
    std::function<QString()> generateId;
    std::function<std::optional<ShapeType>()> currentShapeType;
    QGraphicsScene* scene = nullptr;
};
```

## MultiShapeSession — 快照撤销

```cpp
struct MultiShapeEntry {
    ShapeItem* item = nullptr;
    ShapeData snapshot;
};

class MultiShapeSession {
public:
    bool active = false;
    QList<MultiShapeEntry> entries;
    SelectionFrame originalFrame;

    void begin(const QList<ShapeItem*>& items, const SelectionFrame& frame);
    void cancel() const;      // 遍历 entries，恢复 item->setShapeData(snapshot)
};
```

## SelectionTransformOverlayItem — 覆盖层

- 继承 `QGraphicsObject`，z 值固定 1e6
- 不消费鼠标事件（`setAcceptedMouseButtons(Qt::NoButton)`）
- 绘制：1px 蓝色虚线选框（`#2d7ff9`）+ 4 个 10×10 白色方形手柄
- 命中区：仅 4 个手柄矩形可被命中（`handleAt()`）
- 平移/缩放会话中隐藏手柄（`setHandlesVisible(false)`）
