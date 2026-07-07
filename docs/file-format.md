# `.vgjson` 文件格式

自定义 JSON 格式，扩展名 `.vgjson`（也接受 `.json`），当前版本 **2**。

## 根对象

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `version` | integer | 必填 | 固定 `2` |
| `canvas` | object | 必填 | `{width, height}`，默认 1200×800 |
| `shapes` | array | 必填 | 按 zValue 升序排列 |

## 单个图形 `shapes[]`

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `id` | string | 推荐 | UUID v4；非空时全局唯一 |
| `type` | string | 必填 | 7 种类型之一 |
| `geometry` | object | 必填 | 见下方按类型表格 |
| `strokeEnabled` | boolean | 必填 | 是否描边 |
| `strokeColor` | string | 缺省 | `#AARRGGBB`，缺省 `#ff000000` |
| `strokeWidth` | number | 缺省 | ≥ 0.5（加载时夹紧），缺省 2.0 |
| `strokeStyle` | string | 缺省 | `solid` / `dash` / `dot` / `dashdot` |
| `fillColor` | string | 缺省 | `#AARRGGBB`，缺省 `#00000000` |
| `fillEnabled` | boolean | 缺省 | 仅封闭图形有效 |
| `transform` | object | 必填 | `{m11, m12, m21, m22, dx, dy}` |
| `zValue` | number | 缺省 | 绘制顺序 |

## geometry 按类型

| type | geometry | 约束 |
|------|----------|------|
| `point` | `{x, y}` | — |
| `line` | `{x1, y1, x2, y2}` | — |
| `polyline` | `{points: [{x,y},...]}` | ≥ 2 个 |
| `polygon` | `{points: [{x,y},...]}` | ≥ 2 个，自动闭合 |
| `circle` | `{cx, cy, r}` | r ≥ 0 |
| `ellipse` | `{cx, cy, rx, ry}` | rx, ry ≥ 0 |
| `rectangle` | `{x, y, width, height}` | width, height ≥ 0 |

## 加载时安全校验

| 校验项 | 失败行为 |
|--------|---------|
| `version ≠ 2` | 拒绝整个文档 |
| type 缺失/非法 | 拒绝，提示 `"Invalid shape data at index N"` |
| `transform` 缺失 | 拒绝 |
| `strokeEnabled` 缺失 | 拒绝 |
| 旋转/剪切（`|m12| > ε` 或 `|m21| > ε`） | 拒绝，提示 `"Unsupported transform"` |
| 重复 id | 拒绝，提示 `"Duplicate shape id"` |
| 负半径/宽高 | 夹紧到 0 |
| `fillEnabled` 在不支持的 type 上 | 强制关闭 |

## 完整的示例

```json
{
    "version": 2,
    "canvas": {"width": 1200, "height": 800},
    "shapes": [
        {
            "id": "f1c8a3e0-1234-4abc-9def-000000000001",
            "type": "rectangle",
            "geometry": {"x": 100, "y": 100, "width": 200, "height": 120},
            "strokeEnabled": true,
            "strokeColor": "#ff000000",
            "strokeWidth": 2.0,
            "strokeStyle": "solid",
            "fillColor": "#80c8ff",
            "fillEnabled": true,
            "transform": {"m11": 1.0, "m12": 0.0, "m21": 0.0, "m22": 1.0, "dx": 0.0, "dy": 0.0},
            "zValue": 0.0
        }
    ]
}
```

## 关键设计决策

- `points` 和 `rect` 的语义随 `type` 变化：
  - `point` / `line` / `polyline` / `polygon`：仅 `points` 有意义
  - `circle` / `ellipse` / `rectangle`：仅 `rect` 有意义
- 变换模型：`基础几何 + 仿射变换`，左乘到 `transform`
- 序列化前自动 `normalizedShapeData`，确保落盘几何非退化
- Circle 归一化为正方形外接框（取宽高中较大者作为直径）
