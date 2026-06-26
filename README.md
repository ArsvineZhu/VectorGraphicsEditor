# SVG Editor

轻量级桌面矢量图形编辑器。C++20 + Qt 6 Widgets，支持点、线、折线、多边形、圆、椭圆、矩形等基本图形的绘制、编辑、保存与导出。

本项目是 C++ 课程的实践项目：所有源码以分层架构组织，编译用 CMake Presets，测试用 Qt Test，持续集成使用 `clang-format` / `clang-tidy` / `ctest` 把关。

---

## 目录

- [特性一览](#特性一览)
- [快速开始](#快速开始)
- [架构总览](#架构总览)
- [源码结构](#源码结构)
- [`.vgjson` 文件格式](#vgjson-文件格式)
- [构建与运行](#构建与运行)
  - [Windows (MSYS2 UCRT64)](#windows-msys2-ucrt64)
  - [Linux](#linux)
  - [macOS](#macos)
- [开发工作流](#开发工作流)
  - [运行单个测试](#运行单个测试)
  - [格式化与静态检查](#格式化与静态检查)
- [代码风格](#代码风格)
- [国际化](#国际化)
- [已知约束与设计取舍](#已知约束与设计取舍)

---

## 特性一览

- **图形种类**：点、线段、折线、多边形（自动闭合）、圆、椭圆、矩形。
- **交互方式**：
  - 一次性拖拽：Line / Circle / Ellipse / Rectangle — 按下并拖动，松开落格。
  - 多点绘制：Polyline / Polygon — 单击累加顶点，Enter / 双击结束，Esc 取消。
  - 选择工具：单选 / 框选 / 拖动 / 缩放。
- **属性面板**：选中后实时编辑描边色、线宽、线型、填充色、几何参数。
- **复制 / 粘贴 / 删除**：内置 16 像素步进的多级粘贴。
- **撤销友好结构**：`ShapeData` 携带 `id`（`QUuid`），便于后续接入撤销栈。
- **持久化**：自定义 JSON 格式 `.vgjson`，跨平台可读。
- **导出**：将当前画布渲染为 PNG。
- **国际化**：运行时切换英文 / 简体中文，偏好通过 `QSettings` 持久化。
- **内置教程**：双语操作手册对话框（`Tutorial → Operation Manual`）。

---

## 快速开始

```bash
# 1. 克隆 / 进入源码目录
cd SVG_Editor

# 2. 配置（以 macOS Debug 为例）
cmake --preset darwin-debug

# 3. 编译
cmake --build --preset build-darwin-debug

# 4. 跑测试
ctest --preset test-darwin-debug

# 5. 启动应用
./out/build/darwin-debug/SVG_Editor.app/Contents/MacOS/SVG_Editor
```

Windows / Linux 的具体命令见 [构建与运行](#构建与运行)。

---

## 架构总览

项目按"依赖方向"分四层；下层不引用上层。`core` 静态库供所有上层共用，`ui` 可执行文件把各模块串成完整应用。

```
entry      main.cpp                       — QApplication 入口
  │
  ▼
ui         MainWindow                     — 菜单 / 工具栏 / 信号桥接
  │         CanvasView (QGraphicsView)    — 工具状态机 + 鼠标事件
  │         PropertyPanel                 — 选中图形的属性编辑
  │         TutorialDialog                — 内嵌双语 HTML 手册
  │         ShapeItem (QGraphicsObject)   — 单个图形的绘制 + 命中测试
  │
  ▼
core       ShapeData                      — 数据模型 + JSON 序列化
  │         FileManager                   — 文档级 JSON I/O
  │         AppLanguage                   — i18n 枚举
  │         ShapeFactory                  — 构造 / 克隆辅助
```

### 关键设计决策

- **数据 / 视图分离**：`ShapeData`（`core`）只承担纯数据与序列化；`ShapeItem`（`ui`）只承担绘制与命中测试。`CanvasView` 不直接读 `ShapeItem` 内部状态，而是通过 `ShapeItem::shapeData()` 拉回数据再写回。
- **拖拽工作流与路径工作流共享 `m_previewItem`**：两个状态机不能并存；切换工具时一律 `cancelDrawing()`。
- **JSON 字段名是稳定契约**：`ShapeData.h` / `FileManager.h` 注释中已注明此点，修改字段名需同步更新两个文件。
- **z 值 = `ShapeData::zValue` + `ShapeItem` 自身微小偏移**：同层图形的命中测试靠这层偏移区分。

---

## 源码结构

```
SVG_Editor/
├── CMakeLists.txt              # 根 CMake：项目设置、format/lint/typecheck 目标
├── CMakePresets.json           # 三平台 × 两 build type 预设
├── .clang-format               # LLVM 基础风格（4 空格 / 120 列）
├── .clang-tidy                 # bugprone / modernize / performance / readability
├── .editorconfig               # 强制 UTF-8 + LF + 4 空格
├── README.md                   # 本文件
├── SVG_Editor/                 # 全部源码（10 模块，20 个 .h/.cpp）
│   ├── main.cpp
│   ├── AppLanguage.h           # i18n 枚举
│   ├── ShapeData.{h,cpp}       # 数据模型 + 序列化
│   ├── FileManager.{h,cpp}     # 文档级 JSON I/O
│   ├── ShapeFactory.{h,cpp}    # ShapeItem 构造 / 克隆
│   ├── ShapeItem.{h,cpp}       # QGraphicsObject 渲染层
│   ├── CanvasView.{h,cpp}      # 主画布 + 工具状态机
│   ├── PropertyPanel.{h,cpp}   # 选中图形的属性编辑
│   ├── TutorialDialog.{h,cpp}  # 内嵌双语 HTML 手册
│   └── MainWindow.{h,cpp}      # 主窗口 + 菜单 / 工具栏
└── tests/
    ├── CMakeLists.txt
    ├── ShapeDataTests.cpp      # ShapeData / 序列化 / 归一化测试
    └── FileManagerTests.cpp    # FileManager 往返 / 缺省字段 / 错误文件测试
```

---

## `.vgjson` 文件格式

本项目**不**使用标准 SVG，而是自定义 JSON 格式，扩展名为 `.vgjson`（也接受 `.json`）。

### 根对象

| 字段      | 类型      | 是否必填 | 说明                                       |
| --------- | --------- | -------- | ------------------------------------------ |
| `version` | integer   | 必填     | 当前固定为 `1`                             |
| `canvas`  | object    | 必填     | 画布尺寸；缺省字段回退到 1200×800          |
| `shapes`  | array     | 必填     | 全部图形（按 z 升序）                       |

### `canvas` 对象

| 字段    | 类型    | 说明                       |
| ------- | ------- | -------------------------- |
| `width` | number  | 画布逻辑宽度（场景坐标系） |
| `height`| number  | 画布逻辑高度               |

### `shapes[]` 单个图形

| 字段          | 类型     | 说明                                                       |
| ------------- | -------- | ---------------------------------------------------------- |
| `id`          | string   | UUID；粘贴 / 撤销栈会用到                                  |
| `type`        | string   | `point` / `line` / `polyline` / `polygon` / `circle` / `ellipse` / `rectangle` |
| `points`      | array    | `{x, y}` 数组；按 `type` 决定长度（详见下表）              |
| `rect`        | object   | `{x, y, width, height}`；按 `type` 决定是否使用             |
| `strokeColor` | string   | `#AARRGGBB`                                               |
| `strokeWidth` | number   | ≥ 0.5                                                     |
| `strokeStyle` | string   | `solid` / `dash` / `dot` / `dashdot`                      |
| `fillColor`   | string   | `#AARRGGBB`                                               |
| `fillEnabled` | boolean  | 仅在 `circle` / `ellipse` / `rectangle` / `polygon` 有效  |
| `zValue`      | number   | 绘制顺序                                                   |

### `points` 与 `rect` 字段的语义

| `type`      | `points` 含义         | `rect` 含义             |
| ----------- | --------------------- | ----------------------- |
| `point`     | 1 个点（坐标）        | 不使用                  |
| `line`      | 2 个点（起、终）      | 不使用                  |
| `polyline`  | ≥ 2 个顶点            | 不使用                  |
| `polygon`   | ≥ 2 个顶点（自动闭）  | 不使用                  |
| `circle`    | 不使用                | 正方形外接框            |
| `ellipse`   | 不使用                | 外接矩形                |
| `rectangle` | 不使用                | 矩形（可任意方向）      |

### 示例

```json
{
  "version": 1,
  "canvas": { "width": 1200, "height": 800 },
  "shapes": [
    {
      "id": "f1c8a3e0-1234-4abc-9def-000000000001",
      "type": "rectangle",
      "points": [],
      "rect": { "x": 100, "y": 100, "width": 200, "height": 120 },
      "strokeColor": "#ff000000",
      "strokeWidth": 2.0,
      "strokeStyle": "solid",
      "fillColor": "#ff80c8ff",
      "fillEnabled": true,
      "zValue": 0
    }
  ]
}
```

---

## 构建与运行

### 通用前置条件

- **CMake 3.24+**
- **Qt 6**（`Core` / `Gui` / `Widgets`，测试时还要 `Test`）
- **Ninja**
- **C++20 编译器**
  - Windows: MSVC 或 **MSYS2 UCRT64 内的 g++**（推荐）
  - Linux: GCC 或 Clang
  - macOS: Apple Clang（Xcode Command Line Tools）

### Windows (MSYS2 UCRT64)

#### 安装（一次性）

```bash
# 在 MSYS2 UCRT64 终端中
pacman -Syu
pacman -S --noconfirm mingw-w64-ucrt-x86_64-gcc \
                     mingw-w64-ucrt-x86_64-ninja \
                     mingw-w64-ucrt-x86_64-cmake \
                     mingw-w64-ucrt-x86_64-qt6-base \
                     mingw-w64-ucrt-x86_64-qt6-declarative
```

#### 构建

```powershell
# PowerShell（推荐使用 Windows Terminal 的 MSYS2 UCRT64 profile）
cmake --preset windows-ucrt64-debug
cmake --build --preset build-ucrt64-debug
ctest --preset test-ucrt64-debug
```

> **PATH 提示**：preset 已把 `C:/msys64/ucrt64/bin` 注入到 `PATH`，无需额外设置即可找到 Qt DLL。如在普通 PowerShell 中手动构建，先执行
> ```powershell
> $env:Path = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:Path
> ```

#### 启动应用

```powershell
.\out\build\windows-ucrt64-debug\SVG_Editor.exe
```

### Linux

```bash
# 依赖（Debian/Ubuntu 为例）
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-declarative-dev

# 构建
cmake --preset linux-debug
cmake --build --preset build-linux-debug
ctest --preset test-linux-debug

# 启动
./out/build/linux-debug/SVG_Editor
```

### macOS

#### 安装（一次性）

```bash
# 1) 安装 Xcode Command Line Tools（如未安装）
xcode-select --install

# 2) 安装 Homebrew Qt
brew install qt cmake ninja

# 备用：使用 Qt 官方安装器
# 下载 https://www.qt.io/download-qt-installer 并把 Qt 6 装到 ~/Qt/6.x.x/macos
# 之后把下面加到 ~/.zshrc：
#   export CMAKE_PREFIX_PATH="$HOME/Qt/6.x.x/macos:$CMAKE_PREFIX_PATH"
```

#### 构建

```bash
# 默认从 Homebrew 查找 Qt（/opt/homebrew/opt/qt, /usr/local/opt/qt）
cmake --preset darwin-debug
cmake --build --preset build-darwin-debug
ctest --preset test-darwin-debug

# Release 变体
cmake --preset darwin-release
cmake --build --preset build-darwin-release
```

如果你的 Qt 装在非标准路径，可在调用 preset 前覆盖 `CMAKE_PREFIX_PATH`：

```bash
export CMAKE_PREFIX_PATH="$HOME/Qt/6.7.0/macos:$CMAKE_PREFIX_PATH"
cmake --preset darwin-debug
cmake --build --preset build-darwin-debug
```

#### 启动应用

```bash
open ./out/build/darwin-debug/SVG_Editor.app
# 或
./out/build/darwin-debug/SVG_Editor.app/Contents/MacOS/SVG_Editor
```

> **代码签名提示**：未签名的 `.app` 在 Apple Silicon 上首次启动可能被 Gatekeeper 拦截。
> 命令行启动（第二条命令）可以绕过该限制。

---

## 开发工作流

### 运行单个测试

```bash
# Windows
ctest --preset test-ucrt64-debug -R shape_data_tests --output-on-failure
ctest --preset test-ucrt64-debug -R file_manager_tests --output-on-failure

# Linux
ctest --preset test-linux-debug -R shape_data_tests --output-on-failure

# macOS
ctest --preset test-darwin-debug -R shape_data_tests --output-on-failure
```

也可直接跑测试可执行文件（更快，适合调试）：

```bash
# macOS 示例
./out/build/darwin-debug/shape_data_tests
./out/build/darwin-debug/file_manager_tests
```

### 格式化与静态检查

```bash
# 全部格式化（覆盖 SVG_Editor/ + tests/ 全部源文件）
cmake --build --preset build-darwin-debug --target format

# 仅校验不修改（CI 用的门禁）
cmake --build --preset build-darwin-debug --target format-check

# clang-tidy 静态分析
cmake --build --preset build-darwin-debug --target lint

# 编译式类型检查（= 完整构建）
cmake --build --preset build-darwin-debug --target typecheck
```

> `format` / `format-check` / `lint` 需要 `clang-format` / `clang-tidy` 在 `PATH` 中。
> macOS 自带 `clang-format`（来自 Xcode CLT），`clang-tidy` 用 `brew install llvm` 安装后位于 `/opt/homebrew/opt/llvm/bin`。

### 清理

```bash
rm -rf out/build/darwin-debug
```

---

## 代码风格

- **基础风格**：LLVM 风格（`.clang-format`）。
- **缩进**：4 空格（`.editorconfig`）。
- **列宽**：120。
- **指针对齐**：左对齐（`int* p`）。
- **头文件顺序**：`#pragma once` 之后按 `core` → `graphics` → `ui` 顺序 include；不强制 IWYU。
- **Doxygen 注释**：所有公开类型、方法、成员变量均用 `///` 块注释；中文为主、英文保留技术术语。
- **可疑代码**：用 `// FIXME:` 标记已确认有问题但不立即修复的位置（详见 `ShapeData.cpp` / `FileManager.cpp`）；不要把这些标记用作 TODO 列表。
- **行内注释**：仅在以下场景使用——魔法数字、Qt 平台差异、绕过的编译器行为。

---

## 国际化

- 语言标识：`enum class AppLanguage : std::uint8_t { English, SimplifiedChinese }`（见 [SVG_Editor/AppLanguage.h](SVG_Editor/AppLanguage.h)）。
- 翻译方式：每个 UI 模块自行实现 `textForLanguage(language, "en", "zh")`，**不**依赖 Qt Linguist / `.ts` 文件。
- 持久化：用户上次选择写入 `QSettings`，key 为 `ui/language`，值 `"en"` 或 `"zh-CN"`（非 `"en"` 一律回退为简体中文）。
- 切换路径：`Tutorial → Language → English / 简体中文`。

---

## 已知约束与设计取舍

- **JSON 字段顺序敏感**：`shapeDataToJson` 的字段顺序与 `QJsonObject` 构造顺序一致，便于人读；不要为了"对齐"而调整顺序（diff 会很乱）。
- **`ShapeType` 7 个枚举值**：`shapeTypeToString` 用 `default:` 兜底返回 `"unknown"`；新增枚举值时**必须**同步此函数（GCC `-Wswitch` 会告警）。
- **拖拽工作流与路径工作流互斥**：共享 `m_previewItem`，切换工具时必须 `cancelDrawing()`。
- **`m_updatingWidgets` 防重入**：`PropertyPanel` 在响应用户输入时使用该标志阻止自身信号重入。
- **PNG 导出走 `QGraphicsView::grab()`**：画布尺寸以场景坐标系为准，与视图缩放无关。
- **`QSettings` 路径**：Windows 下写到注册表 `HKEY_CURRENT_USER\Software\CourseProject\SVG Editor`，macOS 下写到 `~/Library/Preferences/com.CourseProject.SVG-Editor.plist`，Linux 下写到 `~/.config/CourseProject/SVG Editor.conf`。

---

## 许可

课程项目，仅供学习与参考。
