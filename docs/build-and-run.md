# 构建与运行

## 通用前置条件

- CMake 3.16+
- Qt 6（Core / Gui / Widgets，测试额外需要 Test）
- Ninja
- C++20 编译器（GCC 11+ / Clang 14+ / MSVC 2022 17+）

## 构建预设

| 平台 | 配置预设 | 构建预设 | 测试预设 |
|------|---------|---------|---------|
| macOS | `darwin-debug` / `darwin-release` | `build-darwin-debug` / `build-darwin-release` | `test-darwin-debug` |
| Linux | `linux-debug` / `linux-release` | `build-linux-debug` / `build-linux-release` | `test-linux-debug` |
| Windows | `windows-ucrt64-debug` / `windows-ucrt64-release` | `build-ucrt64-debug` / `build-ucrt64-release` | `test-ucrt64-debug` |

## macOS

```bash
brew install qt cmake ninja

cmake --preset darwin-debug
cmake --build --preset build-darwin-debug
ctest --preset test-darwin-debug --output-on-failure

open ./out/build/darwin-debug/VectorGraphicsEditor.app

# Gatekeeper 拦截时直接运行二进制：
./out/build/darwin-debug/VectorGraphicsEditor.app/Contents/MacOS/VectorGraphicsEditor
```

### macOS Qt 安装提示

- Homebrew：preset 自动查找 `/opt/homebrew/opt/qt`
- Qt Installer：安装到 `~/Qt/6.x.x/macos`，运行前置 `export CMAKE_PREFIX_PATH="$HOME/Qt/6.x.x/macos:$CMAKE_PREFIX_PATH"`

## Linux

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev

cmake --preset linux-debug
cmake --build --preset build-linux-debug
ctest --preset test-linux-debug --output-on-failure

./out/build/linux-debug/VectorGraphicsEditor
```

## Windows (MSYS2 UCRT64)

```bash
pacman -Syu
pacman -S --noconfirm mingw-w64-ucrt-x86_64-gcc \
                     mingw-w64-ucrt-x86_64-ninja \
                     mingw-w64-ucrt-x86_64-cmake \
                     mingw-w64-ucrt-x86_64-qt6-base

cmake --preset windows-ucrt64-debug
cmake --build --preset build-ucrt64-debug
ctest --preset test-ucrt64-debug --output-on-failure

.\out\build\windows-ucrt64-debug\VectorGraphicsEditor.exe
```

## CMake 结构

### 根 CMakeLists.txt

- C++20，关闭编译器扩展
- `CMAKE_AUTOMOC = ON`
- 可选：`VECTOR_GRAPHICS_EDITOR_ENABLE_WARNINGS`（严格警告）、`VECTOR_GRAPHICS_EDITOR_ENABLE_CLANG_TIDY`
- 子目录：`VectorGraphicsEditor/`、`tests/`（仅 `BUILD_TESTING=ON`）

### VectorGraphicsEditor/CMakeLists.txt

- 查找 `Qt6::Core`、`Qt6::Gui`、`Qt6::Widgets`
- `vector_graphics_editor_core`：静态库（仅 core/ 文件），链接 `Qt6::Core` + `Qt6::Gui`
- `VectorGraphicsEditor`：可执行文件（WIN32），链接 `vector_graphics_editor_core` + `Qt6::Widgets`
- `vector_graphics_editor_warnings`：INTERFACE 库，`-Wall -Wextra -Wpedantic`

### tests/CMakeLists.txt

- 5 个独立测试可执行文件，全部链接 `vector_graphics_editor_core` + `Qt6::Test`
- `shape_item_tests` 和 `main_window_tests` 额外链接 `Qt6::Widgets`
