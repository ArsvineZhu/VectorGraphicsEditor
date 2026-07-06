---
layout: default
transition: slide-left
---

# 测试与质量保障

<div class="h-[2px] w-10 bg-sky-500 mt-2 mb-5"></div>

<div class="grid grid-cols-4 gap-4 mb-6">
  <div v-click class="rounded-xl border border-slate-200 bg-slate-50 p-4 text-center">
    <div class="text-3xl font-bold text-sky-500">5</div>
    <div class="text-sm text-slate-700 mt-1">测试模块</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 bg-slate-50 p-4 text-center">
    <div class="text-3xl font-bold text-sky-500">40</div>
    <div class="text-sm text-slate-700 mt-1">测试用例</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 bg-slate-50 p-4 text-center">
    <div class="text-3xl font-bold text-sky-500">2</div>
    <div class="text-sm text-slate-700 mt-1">静态检查</div>
    <div class="text-xs text-slate-400">format / lint</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 bg-slate-50 p-4 text-center">
    <div class="text-3xl font-bold text-sky-500">3</div>
    <div class="text-sm text-slate-700 mt-1">调试平台</div>
    <div class="text-xs text-slate-400">Windows / Linux / macOS</div>
  </div>
</div>

<div class="grid grid-cols-2 gap-5 text-sm">
  <div class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-3">测试覆盖点</div>
    <ul class="space-y-2 text-slate-600">
      <li v-click><code>ShapeDataTests</code>：序列化往返、归一化、transform 合成、SelectionFrame 映射。</li>
      <li v-click><code>FileManagerTests</code>：文档读写、版本校验、非法 shape 和重复 ID。</li>
      <li v-click><code>CanvasGeometryTests</code>：等比缩放、正交化、防翻转、frame-to-frame 映射。</li>
      <li v-click><code>ShapeItemTests</code>：路径构建、命中区域、preview 行为。</li>
      <li v-click><code>MainWindowTests</code>：<code>Tools</code> 菜单 1 / 3 / 4 分组回归。</li>
    </ul>
  </div>
  <div class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-3">质量门禁</div>
    <ul class="space-y-2 text-slate-600">
      <li v-click><code>clang-format --dry-run --Werror</code> 保证格式一致。</li>
      <li v-click><code>clang-tidy</code> 检查 bugprone / analyzer / modernize / performance 问题。</li>
      <li v-click><code>ctest --output-on-failure</code> 统一跑自动化回归。</li>
      <li v-click>关键逻辑尽量放在可单测的 core 层，而不是只能手点验证的 UI 层。</li>
    </ul>
  </div>
</div>

<!--
这页强调“可验证性”。课程项目最容易被问到“你怎么证明它稳定”。我的回答不是只说“我自己点过很多次”，而是给出明确的测试模块、用例数量和质量门禁。
-->
