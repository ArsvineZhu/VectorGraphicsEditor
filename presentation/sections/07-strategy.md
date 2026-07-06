---
layout: two-cols
transition: slide-left
---

# 核心数据模型 `ShapeData`

<div class="h-[2px] w-10 bg-sky-500 mt-2 mb-5"></div>

::left::

```cpp {1|2-7|8-9}
struct ShapeData {
    QString id;
    ShapeType type = ShapeType::Rectangle;
    QVector<QPointF> points;
    QRectF rect;
    ShapeStyle style;
    QTransform transform;
    qreal zValue = 0.0;
};
```

<div class="mt-4 text-xs text-slate-400">真实定义位于 `core/ShapeData.h`，它是跨 `core / graphics / canvas / ui` 的唯一公共数据结构。</div>

::right::

<div class="space-y-4 text-sm">
  <div v-click class="rounded-xl border border-slate-200 p-4">
    <div class="font-semibold text-slate-700 mb-2">单一边界模型</div>
    <div class="text-slate-500">界面、图形、文件 I/O 都通过 `ShapeData` 交换信息，避免把 `QGraphicsItem` 之类的视图对象泄漏到核心层。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-4">
    <div class="font-semibold text-slate-700 mb-2">几何语义按类型切换</div>
    <div class="text-slate-500">`Point / Line / Polyline / Polygon` 用 `points`，`Circle / Ellipse / Rectangle` 用 `rect`，同一结构统一承载七种图形。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-4">
    <div class="font-semibold text-slate-700 mb-2">配套规则</div>
    <div class="text-slate-500">`normalizedShapeData()` 负责负宽高归一化、圆形外接框正方形化；JSON 字段名是稳定持久化契约。</div>
  </div>
</div>

<!--
如果老师问为什么这里用 struct：因为它是纯数据容器，字段需要在多个层之间直接读写，没有额外封装不变式。真正的行为，比如归一化、平移、序列化，都放在自由函数里。
-->
