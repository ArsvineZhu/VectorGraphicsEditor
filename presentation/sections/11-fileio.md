---
layout: default
transition: slide-left
---

# 事件分发流程

<div class="h-[2px] w-10 bg-sky-500 mt-2 mb-5"></div>

<div class="grid grid-cols-[1.5fr_1fr] gap-6 items-start">
  <div class="rounded-xl border border-slate-200 bg-slate-50 p-4">

```mermaid
flowchart TD
  A["mousePressEvent"] --> B{"Select + 命中手柄?"}
  B -- yes --> C["beginTransformSession"]
  B -- no --> D{"当前工具"}
  D -- Point --> E["直接 addShape"]
  D -- DragTool --> F["m_dragStrategy->begin"]
  D -- PathTool --> G["activePathStrategy()->begin"]

  H["mouseMoveEvent"] --> I{"已有会话?"}
  I -- 变换 --> J["updateTransformSession"]
  I -- 平移 --> K["updateSelectionDrag"]
  I -- 创建 --> L["strategy->update"]

  M["mouseReleaseEvent"] --> N["finishTransform / finishDrag / finishSelectionDrag"]
  O["keyPressEvent"] --> P["Enter 完成路径 / Esc 取消 / Delete 删除 / Ctrl+C/V 复制粘贴"]
```

  </div>
  <div class="space-y-4 text-sm">
    <div v-click class="rounded-xl border border-slate-200 p-4">
      <div class="font-semibold text-slate-700 mb-2">先分发，再计算</div>
      <div class="text-slate-500">`CanvasViewInput.cpp` 的职责是识别当前模式并路由事件，几何计算被下放给策略和 `CanvasGeometry`。</div>
    </div>
    <div v-click class="rounded-xl border border-slate-200 p-4">
      <div class="font-semibold text-slate-700 mb-2">交互优先级明确</div>
      <div class="text-slate-500">选择模式下，命中手柄优先于普通选中，避免用户拖缩放柄时误触发 item selection。</div>
    </div>
  </div>
</div>

<!--
这一页讲“输入状态机”。最重要的点是：不同会话互斥，事件只会流向一个明确入口。比如变换会话开始后，mouseMove 不再进入普通拖拽创建逻辑，这样状态才稳定。
-->
