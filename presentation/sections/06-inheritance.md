---
layout: default
transition: slide-left
---

# 四层架构

<div class="h-[2px] w-10 bg-sky-500 mt-2 mb-5"></div>

<div class="grid grid-cols-[1.6fr_1fr] gap-6 items-start">
  <div class="rounded-xl border border-slate-200 bg-slate-50 p-4">

```mermaid
flowchart TB
  UI["ui<br/>MainWindow / PropertyPanel / TutorialDialog"]
  Canvas["canvas<br/>CanvasView / CreationStrategies / Overlay"]
  Graphics["graphics<br/>ShapeItem / ShapeFactory"]
  Core["core<br/>ShapeData / FileManager / Geometry / I18n"]

  UI --> Canvas
  Canvas --> Graphics
  Canvas --> Core
  Graphics --> Core
```

  </div>
  <div class="space-y-4 text-sm">
    <div v-click class="rounded-xl border border-slate-200 p-4">
      <div class="font-semibold text-slate-700 mb-2">单向依赖</div>
      <div class="text-slate-500">下层不引用上层，`core` 层完全不依赖 Qt Widgets，可被图形、I/O、测试共同复用。</div>
    </div>
    <div v-click class="rounded-xl border border-slate-200 p-4">
      <div class="font-semibold text-slate-700 mb-2">职责分离</div>
      <div class="text-slate-500">视图分发输入，图形层负责绘制，核心层负责数据和几何，不把业务逻辑堆在主窗口。</div>
    </div>
    <div v-click class="rounded-xl border border-slate-200 p-4">
      <div class="font-semibold text-slate-700 mb-2">测试友好</div>
      <div class="text-slate-500">`vector_graphics_editor_core` 作为静态库可以单独链接测试，几何与文件格式都能脱离 GUI 验证。</div>
    </div>
  </div>
</div>

<!--
架构页要强调：这不是“为了分层而分层”，而是为了让 ShapeData、FileManager、CanvasGeometry 这些核心逻辑能脱离界面独立验证。如果把它们都塞进 MainWindow，后面的测试和扩展都会变得很痛苦。
-->
