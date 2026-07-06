---
layout: default
transition: slide-left
---

# 选中、变换与几何约束

<div class="h-[2px] w-10 bg-sky-500 mt-2 mb-5"></div>

<div class="grid grid-cols-2 gap-4 text-sm mb-5">
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">`SelectionFrame`</div>
    <div class="text-slate-500">用 `topLeft + xAxis + yAxis` 表达任意朝向矩形框，而不是只用轴对齐包围盒，方便做一致的几何推导。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">Overlay + Handle</div>
    <div class="text-slate-500">选择框覆盖层单独绘制蓝色虚线框和四个角柄，点击手柄优先进入缩放会话。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">`CanvasGeometry` 纯函数</div>
    <div class="text-slate-500">缩放后的目标 frame、frame-to-frame transform、等比缩放和防翻转钳制都在核心层纯函数里完成。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">交互约束</div>
    <div class="text-slate-500">`Shift` 保持宽高比，`Esc` 取消当前平移或缩放，多选会话通过快照恢复到操作前状态。</div>
  </div>
</div>

<div v-click class="rounded-xl border border-slate-200 bg-slate-50 px-4 py-3 text-sm text-slate-600">
  这部分最容易出“拖拽抖动、手柄翻转、圆形被拉歪”的 bug，所以我把几何关系从 UI 事件里抽了出来，并为其配了独立测试。
</div>

<!--
这一页的关键词是“把复杂几何从事件代码里拿出来”。否则 mouseMoveEvent 会同时承担输入分发、状态维护和矩阵计算，既难读也难测。SelectionFrame 的意义就在于给变换一个稳定的中间表示。
-->
