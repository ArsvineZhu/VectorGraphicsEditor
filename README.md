# VectorGraphicsEditor

轻量级桌面矢量图形编辑器。C++20 + Qt 6.12 Widgets（Graphics View 框架），支持点、线、折线、多边形、圆、椭圆、矩形 7 种图形的绘制、编辑、保存与导出。

本项目是 C++ 课程实践项目：所有源码按四层架构组织，编译用 CMake Presets，测试用 Qt Test + CTest，代码质量用 `clang-format` / `clang-tidy` 把关。

---

## 目录

- [特性一览](#特性一览)
- [快速开始](#快速开始)
- [架构总览](#架构总览)
- [源码结构](#源码结构)
- [`.vgjson` 文件格式](#vgjson-文件格式)
- [构建与运行](#构建与运行)
  - [macOS](#macos)
  - [Linux](#linux)
  - [Windows (MSYS2 UCRT64)](#windows-msys2-ucrt64)
- [开发工作流](#开发工作流)
- [国际化](#国际化)
- [代码风格](#代码风格)
- [课程要求对照](#课程要求对照)

---

## 特性一览

- **7 种图形**：Point / Line / Polyline / Polygon / Circle / Ellipse / Rectangle
- **两种创建方式**：
  - 拖拽创建（DragCreationStrategy）— Line / Circle / Ellipse / Rectangle，按下拖拽落格
  - 多点创建（PathCreationStrategy）— Polyline / Polygon，点击累加顶点，Enter / 双击结束，Esc 取消
  - 即时创建 — Point 工具单次点击
- **选择系统**：单击单选 + RubberBand 框选
- **变换系统**：四角手柄缩放 + 整体平移，Shift 键锁定宽高比 / 角度吸附，支持 Esc 取消
- **属性编辑**：PropertyPanel 实时编辑描边颜色 / 线宽 / 线型 / 填充颜色 / 几何参数，无需 Apply 按钮
- **复制 / 粘贴 / 删除**：Ctrl+C 深拷贝，Ctrl+V 粘贴带 16px 偏移 + 新 UUID
- **文件 I/O**：自定义 JSON 格式 `.vgjson`（version 2），支持打开 / 保存 / 另存为
- **导出**：画布渲染为 PNG
- **主题**：System / Light / Dark，Fusion 风格，System 模式跟随系统色觉设置
- **国际化**：运行时切换 English / 简体中文，偏好通过 QSettings 持久化
- **内置教程**：双语 HTML 操作手册对话框（Tutorial → Operation Manual）

---

## 快速开始

```bash
cd VectorGraphicsEditor

# 配置（以 macOS Debug 为例）
cmake --preset darwin-debug

# 编译
cmake --build --preset build-darwin-debug

# 测试
ctest --preset test-darwin-debug --output-on-failure

# 启动
open ./out/build/darwin-debug/VectorGraphicsEditor.app
```

其他平台的具体命令见 [构建与运行](#构建与运行)。

---

## 架构总览

项目按依赖方向严格分四层，下层不引用上层：

```
entry       app/main.cpp                          — QApplication 入口
  │
  ▼
ui          MainWindow                            — 菜单 / 工具栏 / 信号桥接
            MainWindowActions                     — 声明式菜单/工具栏组装 DSL
            PropertyPanel + GeometryFields        — 图形属性实时编辑器
            TutorialDialog + ManualContent        — 双语 HTML 手册
            ThemeMode / ThemeUtils                — 主题系统

canvas      CanvasView (QGraphicsView)            — 工具状态机 + 鼠标事件分发
            DragCreationStrategy                  — 拖拽创建策略
            PathCreationStrategy                  — 多点创建策略
            MultiShapeSession                     — 多选快照 / 变换撤销
            SelectionTransformOverlayItem         — 选择框覆盖层（手柄+虚线框）

graphics    ShapeItem (QGraphicsObject)           — 单图形绘制 + 命中测试
            ShapeFactory                          — 构造 / 克隆辅助

core        ShapeData                             — 数据模型 + JSON 序列化
            FileManager                           — 文档级 JSON I/O
            CanvasGeometry                        — 几何计算（帧变换、缩放）
            SelectionFrame                        — 有向包围盒 OBB
            AppLanguage / I18n                    — 国际化枚举 + 翻译表
            CanvasViewConstants                   — 全局常量
```

### 设计模式

| 模式 | 应用位置 | 说明 |
|------|---------|------|
| 策略模式 | CreationStrategy 体系 | 统一接口，两种创建策略，依赖注入解耦 |
| 工厂模式 | ShapeFactory | 集中创建 + 强制归一化 |
| 快照模式 | MultiShapeSession | 变换前拍摄完整数据快照，Esc 恢复 |
| 观察者模式 | Qt 信号槽 | selectionStateChanged / shapeEdited 等 4 条信号链 |
| 声明式 DSL | MainWindowActions | ActionDescriptor / MenuDescriptor 数据驱动 UI 组装 |
| 防重入守卫 | PropertyPanel::m_updatingWidgets | 打破 setShapeData → emit → setShapeData 循环 |

### 关键数据流

```
PropertyPanel 控件值改变
  → emit shapeEdited(data)
    → CanvasView::updateSelectedShape(data)
      → item->setShapeData(data) → 重绘
      → emit selectionStateChanged(item, 1)
    → PropertyPanel::setShapeData(data)
      → [m_updatingWidgets=true] 填充控件，不重复发射
```

---

## 源码结构

```
VectorGraphicsEditor/
├── CMakeLists.txt                     # 根 CMake：C++20、Qt6 AUTOMOC、format/lint/typecheck 目标
├── CMakePresets.json                  # 三平台 × 双模式构建/测试预设
├── .clang-format                      # LLVM 风格（4 空格、120 列）
├── .clang-tidy                        # bugprone/modernize/performance/readability
├── .editorconfig                      # UTF-8 / LF / 4 空格
├── README.md                          # 本文件
├── description.md                     # 详细项目分析（供答辩 PPT 生成用）
├── VectorGraphicsEditor/
│   ├── CMakeLists.txt                 # vector_graphics_editor_core（静态库）+ VectorGraphicsEditor（可执行文件）
│   ├── app/
│   │   └── main.cpp                   # [entry] 应用入口
│   ├── core/                          # [core] 零 Widgets 依赖，可被任意层引用
│   │   ├── ShapeData.h/.cpp           #   图形数据模型 + JSON 序列化/归一化
│   │   ├── AppLanguage.h/.cpp         #   语言枚举 + QSettings 持久化
│   │   ├── I18n.h/.cpp                #   统一翻译接口 + 图形/线型翻译表
│   │   ├── CanvasGeometry.h/.cpp      #   几何计算：帧缩放、变换矩阵、角度吸附
│   │   ├── CanvasViewConstants.h      #   全局魔术数字常量
│   │   ├── SelectionFrame.h/.cpp      #   有向包围盒 OBB（支持旋转变换）
│   │   └── FileManager.h/.cpp         #   .vgjson 文件读写 + 安全校验
│   ├── canvas/                        # [canvas] 画布交互与控制
│   │   ├── CanvasView.h               #   QGraphicsView 子类，8 种工具状态机
│   │   ├── CanvasViewCore.cpp         #   生命周期、addShape、文档 I/O、导出
│   │   ├── CanvasViewInput.cpp        #   鼠标/键盘事件分发（4 种工具 × 5 种事件）
│   │   ├── CanvasViewSelection.cpp    #   选择管理、覆盖层更新、变换/平移会话
│   │   ├── CreationStrategy.h         #   ICreationStrategy 抽象接口
│   │   ├── DragCreationStrategy.h/.cpp    # 拖拽创建策略（Line/Rect/Circle/Ellipse）
│   │   ├── PathCreationStrategy.h/.cpp    # 路径创建策略（Polyline/Polygon）
│   │   ├── MultiShapeSession.h/.cpp       # 多选快照 + cancel/restore
│   │   └── SelectionTransformOverlayItem.h/.cpp  # 选择框覆盖层（4 手柄 + 虚线框）
│   ├── graphics/                      # [graphics] QGraphicsItem 渲染
│   │   ├── ShapeItem.h/.cpp           #   QGraphicsObject：路径构建 + 绘制 + 碰撞检测
│   │   └── ShapeFactory.h/.cpp        #   工厂：createItem() + cloneWithOffset()
│   └── ui/                            # [ui] Qt Widgets 界面
│       ├── MainWindow.h/.cpp          #   主窗口：菜单/工具栏/信号桥接/文件操作
│       ├── MainWindowActions.h/.cpp   #   声明式 ActionDescriptor / populateMenu / populateToolbar
│       ├── PropertyPanel.h/.cpp       #   属性面板：几何/描边/填充实时编辑
│       ├── PropertyPanelGeometryFields.h  # 几何字段按类型映射表
│       ├── TutorialDialog.h/.cpp      #   模态教程对话框（QTextBrowser）
│       ├── TutorialManualContent.h    #   静态双语 HTML 教程内容
│       ├── ThemeMode.h                #   System/Light/Dark 枚举 + QSettings 序列化
│       └── ThemeUtils.h/.cpp          #   buildThemePalette + applyApplicationTheme
└── tests/
    ├── CMakeLists.txt
    ├── core/
    │   ├── ShapeDataTests.cpp         #   14 项：JSON 往返、归一化、缺字段、选择框变换
    │   ├── FileManagerTests.cpp       #   5 项：读写往返、版本/变换/ID 校验
    │   └── CanvasGeometryTests.cpp    #   10 项：帧缩放、正交化、等比缩放、钳制防翻转
    ├── graphics/
    │   └── ShapeItemTests.cpp         #   9 项：各类型路径构建、交互区域、预览画笔
    └── ui/
        └── MainWindowTests.cpp        #   1 项：工具菜单结构验证（3 子菜单 × 1/3/4 动作）
```

---

## `.vgjson` 文件格式

本项目使用自定义 JSON 格式，扩展名 `.vgjson`（也接受 `.json`），当前版本为 **2**。

### 根对象

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `version` | integer | 必填 | 当前固定为 `2` |
| `canvas` | object | 必填 | `{width, height}`，默认 1200×800 |
| `shapes` | array | 必填 | 图形列表，按 zValue 升序排列 |

### 单个图形 `shapes[]`

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | string | UUID |
| `type` | string | `point` / `line` / `polyline` / `polygon` / `circle` / `ellipse` / `rectangle` |
| `geometry` | object | 按类型不同（见下表） |
| `strokeEnabled` | boolean | 是否描边 |
| `strokeColor` | string | `#AARRGGBB` |
| `strokeWidth` | number | ≥ 0.5 |
| `strokeStyle` | string | `solid` / `dash` / `dot` / `dashdot` |
| `fillColor` | string | `#AARRGGBB` |
| `fillEnabled` | boolean | 仅封闭图形有效 |
| `transform` | object | `{m11, m12, m21, m22, dx, dy}`，仅支持平移+缩放 |
| `zValue` | number | 绘制顺序 |

### `geometry` 字段按类型

| type | geometry | 说明 |
|------|----------|------|
| `point` | `{x, y}` | 单点坐标 |
| `line` | `{x1, y1, x2, y2}` | 线段起终点 |
| `polyline` | `{points: [{x,y},...]}` | ≥2 个顶点 |
| `polygon` | `{points: [{x,y},...]}` | ≥2 个顶点，自动闭合 |
| `circle` | `{cx, cy, r}` | 圆心 + 半径 |
| `ellipse` | `{cx, cy, rx, ry}` | 圆心 + 半轴 |
| `rectangle` | `{x, y, width, height}` | 左上角 + 宽高 |

### 示例

```json
{
    "version": 2,
    "canvas": {"width": 1200, "height": 800},
    "shapes": [
        {
            "id": "f1c8a3e0-1234-4abc-9def-000000000001",
            "type": "rectangle",
            "geometry": {"x": 100, "y": 100, "width": 200, "height": 120},
            "strokeEnabled": true,
            "strokeColor": "#ff000000",
            "strokeWidth": 2.0,
            "strokeStyle": "solid",
            "fillColor": "#80c8ff",
            "fillEnabled": true,
            "transform": {"m11": 1.0, "m12": 0.0, "m21": 0.0, "m22": 1.0, "dx": 0.0, "dy": 0.0},
            "zValue": 0.0
        }
    ]
}
```

---

## 构建与运行

### 通用前置条件

- CMake 3.16+
- Qt 6（Core / Gui / Widgets，测试额外需要 Test）
- Ninja
- C++20 编译器

### macOS

```bash
# 安装依赖
brew install qt cmake ninja

# 构建
cmake --preset darwin-debug
cmake --build --preset build-darwin-debug
ctest --preset test-darwin-debug --output-on-failure

# 启动
open ./out/build/darwin-debug/VectorGraphicsEditor.app
```

### Linux

```bash
# Debian/Ubuntu 依赖
sudo apt install build-essential cmake ninja-build qt6-base-dev

# 构建
cmake --preset linux-debug
cmake --build --preset build-linux-debug
ctest --preset test-linux-debug --output-on-failure

# 启动
./out/build/linux-debug/VectorGraphicsEditor
```

### Windows (MSYS2 UCRT64)

```bash
# 在 MSYS2 UCRT64 终端中安装
pacman -Syu
pacman -S --noconfirm mingw-w64-ucrt-x86_64-gcc \
                     mingw-w64-ucrt-x86_64-ninja \
                     mingw-w64-ucrt-x86_64-cmake \
                     mingw-w64-ucrt-x86_64-qt6-base

# 构建
cmake --preset windows-ucrt64-debug
cmake --build --preset build-ucrt64-debug
ctest --preset test-ucrt64-debug --output-on-failure

# 启动
.\out\build\windows-ucrt64-debug\VectorGraphicsEditor.exe
```

---

## 开发工作流

```bash
# 格式化源码
cmake --build --preset build-darwin-debug --target format

# CI 格式检查（不修改文件）
cmake --build --preset build-darwin-debug --target format-check

# clang-tidy 静态分析
cmake --build --preset build-darwin-debug --target lint

# 全量编译类型检查
cmake --build --preset build-darwin-debug --target typecheck

# 运行单个测试
ctest --preset test-darwin-debug -R shape_data_tests --output-on-failure

# 直接运行测试二进制（更快）
./out/build/darwin-debug/shape_data_tests
./out/build/darwin-debug/file_manager_tests
./out/build/darwin-debug/canvas_geometry_tests
./out/build/darwin-debug/shape_item_tests
./out/build/darwin-debug/main_window_tests
```

---

## 国际化

- 语言枚举：`AppLanguage { English, SimplifiedChinese }`（`core/AppLanguage.h`）
- 翻译方案：自定义 `i18n::tr(language, key, en, zh)` 接口，**不依赖** Qt Linguist
- 翻译表：`core/I18n.h` 中集中维护图形类型名称 + 线型名称
- 持久化：`QSettings` 键 `ui/language`，值 `"en"` / `"zh-CN"`
- 切换路径：`Tutorial → Language → English / 简体中文`

---

## 代码风格

- 基础风格：LLVM（`.clang-format`），4 空格缩进，120 列宽
- Doxygen 注释：每个文件头部 `@brief` / `@details` / `@layer`，公开方法 `///` 注释
- 头文件保护：`#pragma once`
- 常量管理：全部集中在 `CanvasViewConstants.h`
- 内存管理：Qt 父子对象树自动管理

---

## 课程要求对照

| # | 要求 | 实现情况 |
|---|------|---------|
| 1 | 绘制点/直线/折线/圆/椭圆/矩形/多边形 | ✅ 7 种图形，可视化鼠标交互（拖拽+多点两种策略模式） |
| 2 | 选择、编辑、删除 | ✅ 单选+框选、PropertyPanel 实时编辑、Delete 键删除 |
| 3 | 图形文件保存 | ✅ 自定义 .vgjson 格式（JSON），version 2 |
| 4 | 图形文件打开 | ✅ 含版本校验、变换合法性、重复 ID 检测 |
| 5 | 添加图形 | ✅ 工具栏/菜单 8 种工具即时切换 |
| 6 | 属性信息（颜色/线宽/线型）+ 修改 | ✅ ShapeStyle + PropertyPanel 全字段可编辑 |
| 7 | 封闭图形颜色填充 | ✅ Circle/Ellipse/Rectangle/Polygon 支持 fillEnabled + fillColor |
| 8 | 移动 | ✅ Select 工具拖拽平移，快照式撤销 |
| 8 | 缩放 | ✅ 四角手柄拖拽，Shift 等比缩放 |
| 8 | 复制 | ✅ Ctrl+C 深拷贝至剪贴板 |
| 8 | 粘贴（拷贝） | ✅ Ctrl+V 带 16px 偏移 + 新 UUID |

---

## 许可

课程项目，仅供学习与参考。
