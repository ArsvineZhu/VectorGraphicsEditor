---
layout: two-cols-header
transition: slide-left
---

# 演示与问答提示

<div class="text-sm text-slate-500 mb-2">最后一页不再讲新实现，而是把现场演示路线和高频提问点提前准备好。</div>

::left::

<div class="rounded-xl border border-slate-200 bg-slate-50 p-5 text-sm h-full">
  <div class="font-semibold text-slate-700 mb-3">建议 demo 路线</div>
  <v-clicks>

  1. 选择 `Rectangle` 工具拖出一个矩形。
  2. 在 `PropertyPanel` 修改描边宽度、线型和填充色。
  3. 框选多个图形，拖四角手柄演示缩放，按住 `Shift` 展示等比约束。
  4. `Ctrl+C / Ctrl+V` 演示深拷贝和 16px 粘贴偏移。
  5. 保存为 `.vgjson`，重新打开验证持久化正确；最后导出 PNG。

  </v-clicks>
</div>

::right::

<div class="space-y-3 text-sm">
  <div v-click class="rounded-xl border border-slate-200 p-4">
    <div class="font-semibold text-slate-700">为什么 `ShapeData` 用 `struct`？</div>
    <div class="text-slate-500">它是纯数据边界对象，字段跨层共享，行为放在自由函数中更清晰。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-4">
    <div class="font-semibold text-slate-700">为什么策略接口析构要 `virtual`？</div>
    <div class="text-slate-500">策略通过基类指针管理，不加虚析构就无法安全销毁派生对象。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-4">
    <div class="font-semibold text-slate-700">为什么不直接继承 `QGraphicsItem`？</div>
    <div class="text-slate-500">`QGraphicsObject` 同时提供图形项能力和 `QObject` 生态，更适合需要元对象支持的场景。</div>
  </div>
  <div v-click class="rounded-xl border border-slate-200 p-4">
    <div class="font-semibold text-slate-700">属性面板为什么不会死循环？</div>
    <div class="text-slate-500">因为 `setShapeData()` 期间开启 `m_updatingWidgets`，程序更新控件不会再次触发 emit。</div>
  </div>
</div>

<!--
如果现场时间紧，这页可以只讲右边四个高频问题。它们基本覆盖了老师最常追问的继承、多态、数据建模和事件反馈环问题。还可以补充两个备用问题：为什么 JSON 版本固定为 2、为什么 transform 不支持旋转。
-->
