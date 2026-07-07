# graphics 层详解

## ShapeItem — 渲染节点

`ShapeItem` 继承 `QGraphicsObject`，是 `ShapeData` 与 Qt Graphics View 的桥接。

### 核心职责

- 持有归一化后的 `ShapeData` 副本
- `buildBasePath()` 按 type 构造 `QPainterPath`
- `buildPath()` 对基础路径施加 `QTransform`
- `paint()` 绘制路径、填充、选中虚框
- `shape()` 用 `QPainterPathStroker` 外扩命中区域

### 路径构建（`buildBasePath`）

| type | 路径 |
|------|------|
| Point | `addEllipse(point, radius, radius)`，radius = max(3, strokeWidth*1.5) |
| Line | `moveTo(pt0); lineTo(pt1)` |
| Polyline | `moveTo(pt0); lineTo(pt1); ...; lineTo(ptN)` |
| Polygon（非预览） | `addPolygon(points)`（自动闭合） |
| Polygon（预览） | 同 Polyline（不闭合） |
| Circle / Ellipse | `addEllipse(rect)` |
| Rectangle | `addRect(rect)` |

### 命中测试策略（`buildInteractionPath`）

| 条件 | 命中区域 |
|------|---------|
| strokeEnabled + 无填充 | stroker width = max(8, strokeWidth+6) |
| strokeEnabled + 有填充 | 同上，`united(path)` |
| !strokeEnabled + 有填充 | 仅填充区域 |
| !strokeEnabled + 无填充 | stroker width = max(8, strokeWidth+6) 兜底 |

### 绘制参数

- `boundingRect()`：`shape().boundingRect() + 4px` 四向留白
- `shape()`：通过 `QPainterPathStroker` 外扩的命中路径
- 预览模式：描边提亮 15%，改为虚线，`ItemIsSelectable = false`
- 选中装饰：`m_selectionDecorationVisible` 控制是否自绘虚线框

### Pen / Brush

- 描边：`QPen(color, width, style, RoundCap, RoundJoin)`
- 预览模式：`color.lighter(115)` + `Qt::DashLine`
- 填充：仅 `shapeSupportsFill(type) && fillEnabled` 时使用 `QBrush(fillColor)`
- 描边关闭：`Qt::NoPen`

## ShapeFactory — 工厂

```cpp
class ShapeFactory {
public:
    static std::unique_ptr<ShapeItem> createItem(const ShapeData& data);
    static ShapeData cloneWithOffset(const ShapeData& data, const QPointF& offset);
};
```

### createItem

- 先调用 `normalizedShapeData(data)` 再构造 `ShapeItem`
- 返回 `unique_ptr`，调用方决定如何加入 scene

### cloneWithOffset（粘贴用）

1. 分配新 UUID（`QUuid::createUuid()`）
2. `zValue += 1.0`
3. 整体平移 `offset`
4. 再次归一化
