# core 层详解

`core` 是零 Widgets 依赖的静态库 `vector_graphics_editor_core`，被所有上层与测试共享。

## ShapeData — 数据结构

`ShapeData` 是项目中唯一跨四层的数据结构。

### 几何存储策略

| type | 几何字段 | 序列化格式 |
|------|---------|-----------|
| Point | `points[0]` | `{x, y}` |
| Line | `points[0..1]` | `{x1, y1, x2, y2}` |
| Polyline | `points[0..n]` | `{points: [{x,y},...]}` |
| Polygon | `points[0..n]`（自动闭合） | `{points: [{x,y},...]}` |
| Circle | `rect`（归一化为正方形） | `{cx, cy, r}` |
| Ellipse | `rect` | `{cx, cy, rx, ry}` |
| Rectangle | `rect` | `{x, y, width, height}` |

### ShapeStyle

```cpp
struct ShapeStyle {
    bool strokeEnabled = true;
    QColor strokeColor = Qt::black;
    qreal strokeWidth = 2.0;
    Qt::PenStyle strokeStyle = Qt::SolidLine;
    QColor fillColor = QColor("#80c8ff");
    bool fillEnabled = false;
};
```

Fill 仅在 `Circle / Ellipse / Rectangle / Polygon` 上可用，`Point / Line / Polyline` 的 `fillEnabled` 被
`shapeSupportsFill()` 强制关闭。

### 归一化（`normalizedShapeData`）

- 所有 `rect` → `.normalized()` + 非负夹紧
- Circle → 正方形外接框，取 max(width, height) 作为直径
- JSON 序列化前自动归一化

### 变换模型

`基础几何 + 仿射变换`，变换左乘到 `ShapeData::transform`。序列化为 6 个浮点：
`{m11, m12, m21, m22, dx, dy}`。仅支持平移+缩放（不允许旋转/剪切）。

## FileManager — 文档级 I/O

```cpp
class FileManager {
public:
    static bool saveToFile(const QString& path, const DocumentData& doc, QString* error);
    static std::optional<DocumentData> loadFromFile(const QString& path, QString* error);
};
```

- 保存：写入缩进 JSON（`QJsonDocument::Indented`），便于人读/调试
- 加载：校验 version、type、transform、重复 id，缺省字段兜底处理
- 变换合法性：拒绝含有旋转/剪切的 transform
- 错误消息：所有失败路径通过 `errorMessage` 传递具体原因

## SelectionFrame — 有向包围盒

```cpp
struct SelectionFrame {
    QPointF topLeft;
    QPointF xAxis;   // 局部 (1,0) 方向
    QPointF yAxis;   // 局部 (0,1) 方向
};
```

- 用 3 个向量表达仿射矩形框（非退化 AABB）
- 支持 `orthonormalized()` 正交化、`mapped(transform)` 变换、`translated(delta)` 平移
- `localToSceneTransform()` 返回 3×3 仿射矩阵，可将局部单位正方形映射到场景

## CanvasGeometry — 几何计算

`canvas_geometry` namespace 提供纯数学计算（无 QGraphicsView 依赖）：

| 函数 | 说明 |
|------|------|
| `circleRectFromPoints(start, end)` | 拖拽对角点生成圆的正方形外接框 |
| `scaledFrameFromHandle(handle, frame, point, keepAspectRatio)` | 计算缩放后的目标 frame |
| `transformBetweenFrames(source, target)` | 计算两个 frame 之间的增量变换 |

`CanvasHandle` 枚举：`None / TopLeft / TopRight / BottomLeft / BottomRight`。

## AppLanguage — 语言枚举

```cpp
enum class AppLanguage : std::uint8_t {
    English,
    SimplifiedChinese,  // 默认
};
```

- `appLanguageFromSettingsValue("en")` → `English`；其余回退到 `SimplifiedChinese`
- `appLanguageToSettingsValue(...)` → `"en"` / `"zh-CN"`
- 持久化键：`ui/language`

## I18n — 翻译接口

```cpp
namespace i18n {
    QString tr(AppLanguage lang, std::string_view key, const QString& en, const QString& zh);
}
```

- 不依赖 Qt Linguist / .ts 文件
- 翻译表（`shapeDisplayNames` / `penStyleDisplayNames`）集中在 `I18n.h`
- `pickLocalized(lang, key, LocalizedString{en, zh})` 便捷挑选当前语言文本

## CanvasViewConstants — 全局常量

集中管理所有魔术数字（`CanvasViewConstants.h`）：

| 常量 | 值 | 用途 |
|------|----|------|
| `kSelectionDragMinDistance` | 1.0 | 拖动阈值 |
| `kPasteOffsetPx` | 16.0 | 连续粘贴偏移 |
| `kExportMarginPx` | 20.0 | 导出边距 |
| `kExportMinSize` | 400×300 | 导出最小尺寸 |
| `kDefaultCanvasWidth` | 1200.0 | 默认画布宽度 |
| `kDefaultCanvasHeight` | 800.0 | 默认画布高度 |
| `kHitPaddingMinPx` | 8.0 | 命中区最小 padding |
| `kHitPaddingExtraPx` | 6.0 | 命中区额外 padding |
| `kBoundingRectPaddingPx` | 4.0 | boundingRect 留白 |
| `kSelectionDecorationPaddingPx` | 6.0 | 选中虚框留白 |
