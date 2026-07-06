---
layout: default
transition: slide-left
---

# 关键设计取舍 / 难点解决

<div class="h-[2px] w-10 bg-sky-500 mt-2 mb-5"></div>

<div class="grid grid-cols-2 gap-4 text-sm">
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">数据与视图分离</div>
    <div class="text-slate-500">`CanvasView` 不直接读写 `ShapeItem` 内部状态，统一通过 `ShapeData` 和工厂 / setter 交互，序列化更稳定。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">防重入编辑面板</div>
    <div class="text-slate-500">`m_updatingWidgets` 阻断 “程序更新控件 → valueChanged → 再次 emit” 的死循环，这是属性面板最关键的稳定器。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">选择变换数学抽象</div>
    <div class="text-slate-500">复杂的缩放 / 保比例 / 防翻转没有硬塞进 `mouseMoveEvent`，而是抽成 `SelectionFrame + CanvasGeometry` 组合。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">文件格式的保守约束</div>
    <div class="text-slate-500">版本固定为 2，transform 明确拒绝旋转，减少了 I/O 语义分歧，也让测试边界更清晰。</div>
  </div>
</div>

<div v-click class="mt-5 rounded-xl border border-slate-200 bg-slate-50 px-4 py-3 text-sm text-slate-600">
  我在这个项目里优先选择“可解释、可测试、可维护”的解，而不是短期内最省代码的解。
</div>

<!--
这页适合回答“你觉得自己项目里最有价值的设计是什么”。可以把它理解成答辩里的方法论总结：哪些地方是主动设计出来的，而不是功能自然堆出来的。
-->
