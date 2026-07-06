---
layout: default
transition: slide-left
---

# 构建与跨平台

<div class="h-[2px] w-10 bg-sky-500 mt-2 mb-5"></div>

<div class="rounded-xl border border-slate-200 bg-slate-50 p-4 mb-5 text-sm">

| 平台 | Configure Preset | Build Preset | Test Preset |
| --- | --- | --- | --- |
| Windows | `windows-ucrt64-debug / release` | `build-ucrt64-*` | `test-ucrt64-debug` |
| Linux | `linux-debug / release` | `build-linux-*` | `test-linux-debug` |
| macOS | `darwin-debug / release` | `build-darwin-*` | `test-darwin-debug` |

</div>

<div class="grid grid-cols-[1.1fr_1fr] gap-6 text-sm">
  <div class="rounded-xl border border-slate-200 p-5">
    <div class="font-semibold text-slate-700 mb-3">统一命令入口</div>

```bash
cmake --preset windows-ucrt64-debug
cmake --build --preset build-ucrt64-debug
ctest --preset test-ucrt64-debug --output-on-failure
```

    <div class="mt-3 text-slate-500">CMake Presets v6 把生成器、Qt 路径、编译器和测试输出都固定下来，减少“我机器上能跑，你机器上不行”的问题。</div>
  </div>
  <div class="space-y-4">
    <div v-click class="rounded-xl border border-slate-200 p-5">
      <div class="font-semibold text-slate-700 mb-2">Windows 选择</div>
      <div class="text-slate-500">当前主验证路径是 MSYS2 UCRT64，而不是不完整的 MSVC 配置。</div>
    </div>
    <div v-click class="rounded-xl border border-slate-200 p-5">
      <div class="font-semibold text-slate-700 mb-2">构建工具链</div>
      <div class="text-slate-500">生成器统一为 Ninja，测试通过 CTest 注册，质量目标直接挂到 CMake target。</div>
    </div>
  </div>
</div>

<!--
这一页主要是工程化补充：我没有把构建说明散落在 README 和口头说明里，而是做成了标准化 preset。答辩时老师如果追问跨平台，这页能快速给出证据。
-->
