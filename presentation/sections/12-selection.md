---
layout: two-cols
transition: slide-left
---

# 图形绘制与命中测试

<div class="h-[2px] w-10 bg-sky-500 mt-2 mb-5"></div>

::left::

<div class="space-y-4 text-sm">
  <div v-click class="rounded-xl border border-slate-200 p-4">
    <div class="font-semibold text-slate-700 mb-2">`ShapeItem::paint`</div>
    <div class="text-slate-500">根据 `ShapeType` 选择路径绘制分支，应用描边、填充和 preview 样式。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-4">
    <div class="font-semibold text-slate-700 mb-2">`buildPath` / `buildBasePath`</div>
    <div class="text-slate-500">把统一的 `ShapeData` 几何信息翻译成 `QPainterPath`，支撑矩形、圆、折线、多边形等不同图形。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-4">
    <div class="font-semibold text-slate-700 mb-2">`ShapeFactory`</div>
    <div class="text-slate-500">集中负责 `createItem()` 和 `cloneWithOffset()`，把创建、归一化、复制偏移这些重复动作收拢到一个位置。</div>
  </div>
</div>

::right::

<div class="rounded-xl border border-slate-200 bg-slate-50 p-5 text-sm">
  <div class="font-semibold text-slate-700 mb-3">为什么区分 `boundingRect()` 和 `shape()`</div>
  <v-clicks>

  - `boundingRect()`：给 Qt 做重绘裁剪和粗粒度包围盒判断。
  - `shape()`：给鼠标命中测试、选择和碰撞判断提供更精确的路径区域。
  - 对线段、点这类细图形，需要在命中区域上做适当扩展，否则“看得见但点不中”。
  - 这也是 `ShapeItemTests` 重点覆盖的内容之一。

  </v-clicks>
</div>

<!--
这里可以顺着老师的问题讲：为什么不是只实现一个 paint 就结束。因为图形编辑器不仅要“画出来”，还要“选得到、点得准、复制得对”。所以绘制路径和命中路径是两个不同层面的需求。
-->
