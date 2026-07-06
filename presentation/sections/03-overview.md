---
layout: default
transition: slide-left
---

# 项目概述

<div class="h-[2px] w-10 bg-sky-500 mt-2 mb-6"></div>

<div class="grid grid-cols-4 gap-4 mb-7">
  <div v-click class="rounded-xl border border-slate-200 bg-slate-50 p-4 text-center">
    <div class="text-3xl font-bold text-sky-500">44</div>
    <div class="mt-1 text-sm text-slate-700">源码文件</div>
    <div class="text-xs text-slate-400">`VectorGraphicsEditor/*.cpp + *.h`</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 bg-slate-50 p-4 text-center">
    <div class="text-3xl font-bold text-sky-500">4430</div>
    <div class="mt-1 text-sm text-slate-700">源码行数</div>
    <div class="text-xs text-slate-400">不含 tests 目录</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 bg-slate-50 p-4 text-center">
    <div class="text-3xl font-bold text-sky-500">5 / 40</div>
    <div class="mt-1 text-sm text-slate-700">测试模块 / 用例</div>
    <div class="text-xs text-slate-400">Qt Test + CTest</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 bg-slate-50 p-4 text-center">
    <div class="text-3xl font-bold text-sky-500">7</div>
    <div class="mt-1 text-sm text-slate-700">基础图形</div>
    <div class="text-xs text-slate-400">点、线、面、圆形族</div>
  </div>
</div>

<v-clicks>

<div class="flex border-b border-slate-100 py-3 text-sm">
  <div class="w-28 shrink-0 text-slate-400 font-medium">项目定位</div>
  <div class="text-slate-700">轻量级桌面矢量图形编辑器，覆盖绘制、选择、编辑、存储、导出完整闭环。</div>
</div>

<div class="flex border-b border-slate-100 py-3 text-sm">
  <div class="w-28 shrink-0 text-slate-400 font-medium">技术栈</div>
  <div class="text-slate-700">C++20、Qt 6 Widgets / Graphics View、CMake Presets、Ninja。</div>
</div>

<div class="flex border-b border-slate-100 py-3 text-sm">
  <div class="w-28 shrink-0 text-slate-400 font-medium">输出能力</div>
  <div class="text-slate-700">自定义 `.vgjson` 文件格式、PNG 导出、运行时中英文切换、系统 / 明亮 / 深色主题。</div>
</div>

<div class="flex py-3 text-sm">
  <div class="w-28 shrink-0 text-slate-400 font-medium">课程价值</div>
  <div class="text-slate-700">把继承、多态、抽象类、静态成员、文件 I/O、友元测试等知识点落到了一个真实可运行系统里。</div>
</div>

</v-clicks>

<!--
这一页先建立量级感：项目不是 demo 级别，而是有明确模块边界、文件格式和测试体系的课程实践。这里的数字都来自当前仓库状态，后面再展开它们分别意味着什么。
-->
