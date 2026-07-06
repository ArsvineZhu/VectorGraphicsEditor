---
layout: two-cols
transition: slide-left
---

# 需求与目标

<div class="h-[2px] w-10 bg-sky-500 mt-2 mb-5"></div>

::left::

<div class="rounded-xl border border-slate-200 bg-slate-50 p-5 text-sm">
  <div class="font-semibold text-slate-700 mb-3">题目要求</div>
  <v-clicks>

  - 实现一个可视化矢量图形处理系统，而不是单纯的数据结构练习。
  - 支持基础图形的创建、选择、编辑和保存，能完成一次完整作业流程。
  - 用课程内的 OOP 知识组织代码，而不是把逻辑全部塞进一个窗口类。

  </v-clicks>
</div>

<div v-click class="mt-4 rounded-xl border border-slate-200 p-5 text-sm">
  <div class="font-semibold text-slate-700 mb-3">核心用户操作</div>
  <div class="text-slate-600">选工具 → 在画布创建图形 → 调整样式与几何参数 → 多选 / 变换 → 保存文档或导出 PNG。</div>
</div>

::right::

<div class="space-y-4 text-sm">
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">功能目标</div>
    <div class="text-slate-500">保证 7 种图形、两类创建流程、属性面板、复制粘贴、文件 I/O、主题与语言切换都可用。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">工程目标</div>
    <div class="text-slate-500">分层清晰、数据模型稳定、关键行为可测试、跨平台构建路径统一。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-2">边界选择</div>
    <div class="text-slate-500">暂不做撤销/重做、旋转编辑、标准 SVG 互操作，优先把当前功能做完整、做稳定。</div>
  </div>
</div>

<!--
这页说明我是怎么“收敛范围”的：课程项目最怕什么都想做，最后每个点都只做一半。所以我把目标锁定成“围绕 7 种图形完成一个稳定闭环”，再用分层和测试把它做好。
-->
