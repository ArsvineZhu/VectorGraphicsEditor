---
layout: two-cols
transition: slide-left
---

# 依赖注入与创建上下文

<div class="h-[2px] w-10 bg-sky-500 mt-2 mb-5"></div>

::left::

```cpp {1|2-7}
struct CreationContext {
    std::function<qreal()> nextZValue;
    std::function<ShapeStyle(ShapeType)> currentStyleFor;
    std::function<void(const ShapeData&, bool)> addShape;
    std::function<QString()> generateId;
    std::function<std::optional<ShapeType>()> currentShapeType;
    QGraphicsScene* scene = nullptr;
};
```

<div class="mt-4 text-xs text-slate-400">`CanvasView` 在构造函数里一次性把回调注入三个创建策略。</div>

::right::

<div class="space-y-4 text-sm">
  <div v-click class="rounded-xl border border-slate-200 p-4">
    <div class="font-semibold text-slate-700 mb-2">为什么不用“策略直接依赖 CanvasView”</div>
    <div class="text-slate-500">那样策略会反向耦合整个视图类，难以测试，也让职责边界变得模糊。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-4">
    <div class="font-semibold text-slate-700 mb-2">注入的内容</div>
    <div class="text-slate-500">策略只拿到它真正需要的能力：生成 ID、查询当前样式、分配 z 值、把图形加入场景。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-4">
    <div class="font-semibold text-slate-700 mb-2">直接效果</div>
    <div class="text-slate-500">`canvas` 层可以把算法和界面容器解耦，同一套策略既能驱动 preview item，也能最终落盘为真实图形。</div>
  </div>
</div>

<!--
如果老师问“依赖注入体现在哪”，这里就是最直接的例子。CreationContext 不是为了炫技巧，而是为了让策略层只依赖一小组稳定能力，不需要知道 CanvasView 的全部内部状态。
-->
