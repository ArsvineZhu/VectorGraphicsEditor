---
layout: two-cols
transition: slide-left
---

# 国际化与主题

<div class="h-[2px] w-10 bg-sky-500 mt-2 mb-5"></div>

::left::

<div class="rounded-xl border border-slate-200 bg-slate-50 p-5 text-sm h-full">
  <div class="font-semibold text-slate-700 mb-3">国际化</div>
  <v-clicks>

  - `AppLanguage` 明确只有 `English / SimplifiedChinese` 两种语言状态。
  - 图形名、线型名、菜单文案、状态栏提示统一走 `I18n` 翻译接口。
  - `TutorialDialog` 内置双语 HTML 手册，帮助系统和主界面一起切换。
  - 语言偏好写入 `QSettings`，下次启动自动恢复。

  </v-clicks>
</div>

::right::

<div class="space-y-4 text-sm">
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">主题模式</div>
    <div class="text-slate-500">`ThemeMode` 提供 `System / Light / Dark` 三档，统一由 `applyApplicationTheme()` 应用。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">实现方式</div>
    <div class="text-slate-500">应用级别切到 `Fusion` 风格，再构建调色板；`System` 模式会跟随系统色彩方案变化。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">价值</div>
    <div class="text-slate-500">这是一个很好的课程扩展点：不仅是算法正确，还考虑了产品层面的可用性和一致性。</div>
  </div>
</div>

<!--
国际化和主题不是“锦上添花”的功能。对课程项目来说，它们说明系统不是一堆功能堆砌，而是从状态持久化、文案组织到视觉一致性都经过了设计。
-->
