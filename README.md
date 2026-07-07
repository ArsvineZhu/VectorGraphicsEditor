# VectorGraphicsEditor

轻量级桌面矢量图形编辑器。C++20 + Qt 6.12 Widgets（Graphics View 框架），支持点、线、折线、多边形、圆、椭圆、矩形 7 种图形的绘制、编辑、保存与导出。

本项目是 C++ 课程实践项目：所有源码按四层架构组织，编译用 CMake Presets，测试用 Qt Test + CTest，代码质量用 `clang-format` / `clang-tidy` 把关。

---

## 快速开始

```bash
cmake --preset darwin-debug
cmake --build --preset build-darwin-debug
ctest --preset test-darwin-debug --output-on-failure
open ./out/build/darwin-debug/VectorGraphicsEditor.app
```

详见 [docs/build-and-run.md](docs/build-and-run.md)。

---

## 特性一览

- **7 种图形**：Point / Line / Polyline / Polygon / Circle / Ellipse / Rectangle
- **三种创建方式**：拖拽创建 / 多点创建 / 即时创建（Point 单次点击）
- **选择系统**：单选 + RubberBand 框选，多选统一变换/删除
- **变换系统**：四角手柄缩放 + 整体平移，Shift 等比缩放，Esc 回退
- **属性编辑**：实时编辑描边/线宽/线型/填充/几何参数，无需 Apply
- **文件 I/O**：`.vgjson` (version 2)，打开/保存/另存为/导出 PNG
- **主题**：System / Light / Dark，Fusion 风格
- **国际化**：English / 简体中文，运行时切换

---

## 平台支持

| 平台 | 架构 | 打包格式 |
|------|------|---------|
| Windows | AMD64 (x86_64) | 便携版 ZIP / NSIS 安装包 (`.exe`) |
| Windows | ARM64 | 便携版 ZIP (`.zip`) |
| macOS | Apple Silicon (ARM64) | DMG (`.dmg`) |
| Linux | AMD64 (x86_64) | Debian (`.deb`) / Red Hat (`.rpm`) / Arch (`.pkg.tar.zst`) |

Release 构建矩阵参见 [`.github/workflows/release.yml`](.github/workflows/release.yml)。

---

## 架构

项目按**四层严格分层**：`entry → ui → canvas → graphics → core`。详见 [docs/architecture.md](docs/architecture.md)。

```
entry    app/main.cpp                    — QApplication 入口
ui       MainWindow / PropertyPanel      — 菜单、工具栏、属性编辑、主题
canvas   CanvasView / Strategies         — 工具状态机、创建策略、选择变换
graphics ShapeItem / ShapeFactory        — 渲染、碰撞检测、构造/克隆
core     ShapeData / FileManager         — 数据模型、JSON 序列化、几何计算
```

---

## 项目文档

| 文档 | 内容 |
|------|------|
| [架构总览](docs/architecture.md) | 四层架构、设计模式、交互会话、数据流 |
| [构建与运行](docs/build-and-run.md) | macOS / Linux / Windows 构建指南、CMake 结构 |
| [文件格式](docs/file-format.md) | `.vgjson` v2 规范、字段表、安全校验规则 |
| [core 层](docs/core-layer.md) | ShapeData、FileManager、SelectionFrame、几何计算、i18n |
| [canvas 层](docs/canvas-layer.md) | 工具状态机、事件路由、创建策略、选择/变换系统 |
| [graphics 层](docs/graphics-layer.md) | ShapeItem 渲染、命中测试、ShapeFactory 工厂 |
| [ui 层](docs/ui-layer.md) | MainWindow DSL、PropertyPanel、TutorialDialog |
| [测试体系](docs/testing.md) | 5 个测试二进制、用例详情、运行方式 |
| [国际化与主题](docs/i18n-and-theme.md) | 翻译接口、语言切换、亮/暗调色板、系统跟随 |
| [开发工作流](docs/dev-workflow.md) | 质量门禁、代码规范、命名约定、Doxygen 规范 |
| [如何扩展](docs/extending.md) | 新增图形类型/策略/动作/语言/格式版本的步骤清单 |
| [常见问题](docs/troubleshooting.md) | 编译/运行时问题排错、macOS 特有问题 |

---

## 开发工作流

```bash
cmake --build --preset build-darwin-debug --target typecheck   # 类型检查
ctest --preset test-darwin-debug --output-on-failure            # 运行测试
cmake --build --preset build-darwin-debug --target format      # 格式化
cmake --build --preset build-darwin-debug --target lint        # 静态分析
```

详见 [docs/dev-workflow.md](docs/dev-workflow.md)。

---

## 课程要求对照

| # | 要求 | 实现 |
|---|------|------|
| 1 | 7 种图形绘制 | ✅ 拖拽+多点两种策略模式 |
| 2 | 选择、编辑、删除 | ✅ 单选+框选、实时编辑、Delete |
| 3 | 文件保存 | ✅ `.vgjson` v2 (JSON) |
| 4 | 文件打开 | ✅ 版本/变换/id 校验 |
| 5 | 添加图形 | ✅ 8 种工具切换 |
| 6 | 属性修改 | ✅ ShapeStyle 全字段可编辑 |
| 7 | 封闭填充 | ✅ Circle/Ellipse/Rectangle/Polygon |
| 8 | 移动/缩放/复制/粘贴 | ✅ 快照撤销、等比缩放、16px 偏移粘贴 |

## 当前边界

不支持：贝塞尔曲线、布尔运算、图层系统、SVG 导入、逐顶点编辑、渐变填充、Undo/Redo。

---

## 许可

课程项目，仅供学习与参考。
