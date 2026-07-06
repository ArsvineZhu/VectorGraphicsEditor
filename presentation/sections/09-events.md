---
layout: two-cols
transition: slide-left
---

# 多态与策略模式

<div class="h-[2px] w-10 bg-sky-500 mt-2 mb-5"></div>

::left::

```cpp {1|2-8}
class ICreationStrategy {
  public:
    virtual ~ICreationStrategy() = default;
    virtual void begin(const QPointF& scenePoint) = 0;
    virtual void update(const QPointF& scenePoint) = 0;
    virtual void finish(const QPointF& scenePoint) = 0;
    virtual void cancel() = 0;
    virtual bool inProgress() const = 0;
};
```

::right::

<div class="text-sm text-slate-500 mb-4">创建流程被拆成统一接口 `ICreationStrategy`，具体行为在运行时绑定到不同工具。</div>

<v-clicks>

- **两类具体策略**：`DragCreationStrategy` 处理直线 / 圆 / 椭圆 / 矩形，`PathCreationStrategy` 处理折线 / 多边形。
- **CanvasView 的用法**：只持有 `unique_ptr<ICreationStrategy>`，在 `mousePressEvent / mouseMoveEvent / mouseReleaseEvent` 中通过基类指针调用 `begin / update / finish`。
- **收益**：新增一种创建方式时，只需要新增策略并接入路由，而不是把大量 `if / else` 分支继续堆进视图类。

</v-clicks>

<!--
这一页重点回答“多态具体体现在哪里”。四个条件是：基类虚函数、派生类 override、基类指针调用、指向派生类对象。这里四个条件全满足，所以这不是形式上的接口，而是真正的运行时多态。
-->
