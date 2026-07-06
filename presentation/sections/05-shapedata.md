---
layout: two-cols-header
transition: slide-left
---

# 功能总览

::left::

<div class="rounded-xl border border-slate-200 bg-slate-50 p-5 text-sm h-full">
  <div class="font-semibold text-slate-700 mb-3">创建链路</div>
  <v-clicks>

  - `Point`：单击即生成。
  - `Line / Circle / Ellipse / Rectangle`：按下-拖拽-释放完成。
  - `Polyline / Polygon`：逐点累加，`Enter` 或双击结束。
  - 绘制中存在 preview item，用户先看到结果再确认落盘。

  </v-clicks>
</div>

::right::

<div class="space-y-4 text-sm">
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">编辑能力</div>
    <div class="text-slate-500">单选、橡皮筋框选、四角手柄缩放、整体平移、实时属性编辑、复制粘贴删除。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">文档能力</div>
    <div class="text-slate-500">`.vgjson` 打开 / 保存 / 另存为，文档版本固定为 2，画布可导出 PNG。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">产品细节</div>
    <div class="text-slate-500">运行时切换语言和主题，偏好通过 `QSettings` 持久化，内置双语操作手册。</div>
  </div>
</div>

<div v-click class="mt-5 rounded-xl border border-sky-100 bg-sky-50 px-4 py-3 text-sm text-sky-700">
  现场 demo 建议顺序：创建矩形 → 改描边 / 填充 → 框选缩放 → 复制粘贴 → 保存 / 重新打开。
</div>

<!--
这一页从用户视角讲功能，不急着进代码。答辩时可以把“创建链路”和“编辑链路”对应到后面策略模式、事件分发和几何变换三页，让老师先知道系统究竟能做什么。
-->
