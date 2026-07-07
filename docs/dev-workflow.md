# 开发工作流与代码规范

## 质量门禁

```bash
cmake --build --preset build-darwin-debug --target typecheck   # 全量编译
cmake --build --preset build-darwin-debug --target format      # clang-format 原地格式化
cmake --build --preset build-darwin-debug --target format-check # CI 格式检查
cmake --build --preset build-darwin-debug --target lint        # clang-tidy 静态分析
```

## 推荐工作流

1. `typecheck` — 确保无编译错误
2. `ctest` — 确保测试通过
3. 编码 → `typecheck` → 运行相关测试 → `format` → 提交

## 代码风格

### 基础

- LLVM 风格，4 空格缩进，120 列宽
- 头文件保护：`#pragma once`
- Include 顺序手动维护（`SortIncludes: Never`）

### 命名

| 类型 | 约定 | 示例 |
|------|------|------|
| 类/结构体 | PascalCase | `ShapeData`, `CanvasView` |
| 枚举/枚举值 | PascalCase | `ShapeType::Circle`, `Tool::Select` |
| 成员变量 | `m_` + camelCase | `m_scene`, `m_zCounter` |
| 自由函数 | camelCase | `shapeDataToJson`, `normalizedShapeData` |
| 局部变量 | camelCase | `filePath`, `delta` |
| 常量 | `k` + PascalCase | `kHandleSize`, `kDefaultCanvasWidth` |
| 命名空间 | snake_case | `canvas_geometry`, `i18n` |

### Include 顺序

1. 对应头文件（`.cpp` 文件）
2. Qt 头文件（`<Q...>`）
3. C++ 标准库（`<...>`）
4. 项目头文件（`"..."`）

组间空行分隔，各组内字母序。

### 文件结构

```cpp
// =====================================================================
// FileName.h
// ---------------------------------------------------------------------
// @brief   一句话说明
// @details 详细说明
// @layer   core | canvas | graphics | ui | entry
// @warning 注意事项（可选）
// =====================================================================

#pragma once

// includes

class Foo {
  public:
    /// public methods
  signals:  // (if QObject)
    /// signals
  private:
    /// members
};
```

### Doxygen 规范

- 每个 `.h` / `.cpp` 头部必须含 `@brief` / `@details` / `@layer`
- `@layer` 取值：`entry` / `core` / `canvas` / `graphics` / `ui`
- 公开方法使用 `///` 单行注释
- 可选标签：`@warning`、`@note`

### 内存管理

- `QObject` 子类通过 Qt 父子对象树管理
- `ICreationStrategy` 实现类通过 `std::unique_ptr` 管理
- `ShapeItem` 加入 `QGraphicsScene` 后由 Scene 在 `clear()` 时销毁

## 工具配置

| 文件 | 说明 |
|------|------|
| `.clang-format` | LLVM，4 空格，120 列，`SortIncludes: Never` |
| `.clang-tidy` | bugprone/clang-analyzer/modernize/performance/readability |
| `.editorconfig` | UTF-8 / LF / 尾换行 / 去尾空 |
| `CMakePresets.json` | v6，三平台 × 双模式预设 |

## 常用命令速查

```bash
# 全量编译
cmake --build --preset build-darwin-debug

# 运行所有测试
ctest --preset test-darwin-debug --output-on-failure

# 按名称筛选测试
ctest --preset test-darwin-debug -R shape_data_tests

# 格式化
cmake --build --preset build-darwin-debug --target format

# lint
cmake --build --preset build-darwin-debug --target lint

# 直接运行测试二进制
./out/build/darwin-debug/shape_data_tests
./out/build/darwin-debug/file_manager_tests
./out/build/darwin-debug/canvas_geometry_tests
./out/build/darwin-debug/shape_item_tests
./out/build/darwin-debug/main_window_tests
```
