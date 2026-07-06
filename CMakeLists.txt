# =====================================================================
# 根 CMakeLists.txt
# ---------------------------------------------------------------------
# @brief   VectorGraphicsEditor 项目的根 CMake 配置
# @details
#   - 设定项目元信息（C++20、Qt6 AUTOMOC 等）
#   - 把 Qt6_DIR / Qt6_ROOT 环境变量注入到 CMAKE_PREFIX_PATH
#     （兼容 MSYS2 / Homebrew / Qt 官方安装器等多种安装方式）
#   - 调用子目录：VectorGraphicsEditor/（应用 + core 静态库）、tests/（测试，仅在
#     BUILD_TESTING=ON 时）
#   - 暴露给开发者的质量门禁自定义目标：format / format-check / lint / typecheck
# @layer   build
# =====================================================================

# 最低 CMake 版本要求（3.16+ 支持 conan/protoc 较新特性；本项目用 3.16 即可）
cmake_minimum_required(VERSION 3.16)

# 项目语言限定为 C++，避免 CMake 默认混入 C / Fortran 检测
project(VectorGraphicsEditor LANGUAGES CXX)

# 强制使用 C++20；不开启 GNU / MSVC 扩展以保证编译器间一致性
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# 输出 compile_commands.json，方便 IDE / clang-tidy 集成
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt 元对象编译器：仅开 AUTOMOC（Q_OBJECT 宏自动处理），
# AUTOUIC / AUTORCC 关闭（项目未使用 .ui / .qrc 资源）
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# 全局可选项：严格警告 / clang-tidy
option(VECTOR_GRAPHICS_EDITOR_ENABLE_WARNINGS "Enable strict compiler warnings." ON)
option(VECTOR_GRAPHICS_EDITOR_ENABLE_CLANG_TIDY "Enable clang-tidy during builds when available." ON)

# Qt6_DIR / Qt6_ROOT 由 preset 或用户显式注入；这里把它们的值附加到搜索路径
if(DEFINED ENV{Qt6_DIR})
  list(APPEND CMAKE_PREFIX_PATH "$ENV{Qt6_DIR}")
endif()

if(DEFINED ENV{Qt6_ROOT})
  list(APPEND CMAKE_PREFIX_PATH "$ENV{Qt6_ROOT}")
endif()

# 子目录：VectorGraphicsEditor/ 内部定义 vector_graphics_editor_core 静态库 + VectorGraphicsEditor 可执行
add_subdirectory(VectorGraphicsEditor)

# 引入 CTest（自动从各 add_test() 收集测试用例）
include(CTest)

if(BUILD_TESTING)
  # tests/ 子目录注册两个测试可执行：shape_data_tests / file_manager_tests
  add_subdirectory(tests)
endif()

# 工具探测：clang-format / clang-tidy 各自仅在 PATH 中存在时启用对应 target
find_program(CLANG_FORMAT_EXE NAMES clang-format)
find_program(CLANG_TIDY_EXE NAMES clang-tidy)

# VectorGraphicsEditor/CMakeLists.txt 把所有源码文件追加到这个全局 property，
# 根 CMake 用它做全项目级别的 format / lint（不依赖子目录 target）
get_property(VECTOR_GRAPHICS_EDITOR_SOURCE_FILES GLOBAL PROPERTY VECTOR_GRAPHICS_EDITOR_SOURCE_FILES)

if(CLANG_FORMAT_EXE AND VECTOR_GRAPHICS_EDITOR_SOURCE_FILES)
  # format: 直接改写源文件（开发者本地使用）
  add_custom_target(format
    COMMAND ${CLANG_FORMAT_EXE} -i ${VECTOR_GRAPHICS_EDITOR_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Formatting VectorGraphicsEditor sources with clang-format")

  # format-check: 干跑 + Werror（CI 用的门禁）
  add_custom_target(format-check
    COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${VECTOR_GRAPHICS_EDITOR_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Checking VectorGraphicsEditor formatting with clang-format")
else()
  # 工具缺失时回退为 echo：避免 IDE / CI 莫名失败
  add_custom_target(format
    COMMAND ${CMAKE_COMMAND} -E echo "clang-format not found; install LLVM tools to enable formatting.")
  add_custom_target(format-check
    COMMAND ${CMAKE_COMMAND} -E echo "clang-format not found; install LLVM tools to enable format checks.")
endif()

if(CLANG_TIDY_EXE AND VECTOR_GRAPHICS_EDITOR_SOURCE_FILES)
  add_custom_target(lint
    COMMAND ${CLANG_TIDY_EXE}
            -quiet
            -p ${CMAKE_BINARY_DIR}
            --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
            ${VECTOR_GRAPHICS_EDITOR_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy over VectorGraphicsEditor sources")
else()
  add_custom_target(lint
    COMMAND ${CMAKE_COMMAND} -E echo "clang-tidy not found; install LLVM tools to enable lint checks.")
endif()

# typecheck: 仅触发完整构建（依赖 VectorGraphicsEditor 与 vector_graphics_editor_tests），
# 不做额外静态分析 — 编译本身就是类型检查
add_custom_target(typecheck DEPENDS VectorGraphicsEditor)

if(TARGET vector_graphics_editor_tests)
  add_dependencies(typecheck vector_graphics_editor_tests)
endif()
