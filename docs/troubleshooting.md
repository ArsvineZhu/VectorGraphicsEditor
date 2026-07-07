# 常见问题与排错

## 编译错误

| 症状 | 原因 | 解决 |
|------|------|------|
| `Qt6::Core not found` | CMake 找不到 Qt 6 | 设置 `CMAKE_PREFIX_PATH` 指向 Qt 安装路径 |
| `Failed to find Qt component "Test"` | 未装 Qt Test | 安装 `qt6-base-dev`（Linux）或 `qt`（macOS） |
| `'QString' has not been declared` | 缺少 `#include <QString>` | 检查遗漏的 Qt 头文件 |
| `warning: enumeration value ... not handled in switch` | 新增 `ShapeType` 未添加 case | 在相关 switch 中补充分支 |
| `ninja: error: loading 'build.ninja'` | 未运行 configure | 先执行 `cmake --preset <name>` |

## 运行时问题

| 现象 | 原因 | 解决 |
|------|------|------|
| 启动即闪退 | Qt 平台插件缺失 | 检查 `QT_QPA_PLATFORM_PLUGIN_PATH` 环境变量 |
| 导出 PNG 空白 | 画布空（preview 不计入） | 先创建至少一个图形再导出 |
| 打开 `.vgjson` 失败 | 格式/版本/变换不合法 | 查看错误消息确认原因 |
| 属性面板灰掉 | 图形已变换（transform 非恒等） | 已变换图形仅支持样式编辑 |
| 点/线无填充选项 | 这些类型不支持填充 | 改用 Circle / Ellipse / Rectangle / Polygon |
| 选中图形无法移动 | 未切换到 Select 工具 | 切换回 Select 工具 |
| 粘贴位置不对 | offset 基于 `m_pasteCount` 累积 | 重新复制后 `m_pasteCount` 归零 |
| Ctrl+C/V 无反应 | 焦点不在画布上 | 先点击画布区域 |

## macOS 特有问题

| 现象 | 解决 |
|------|------|
| `open xxx.app` 被 Gatekeeper 拦截 | 用二进制直接启动：`./xxx.app/Contents/MacOS/VectorGraphicsEditor` |
| Homebrew 未找到 Qt | `brew install qt` 或手动设置 `CMAKE_PREFIX_PATH` |

## 文件加载错误消息解读

| 错误消息 | 含义 |
|---------|------|
| `"Unsupported document version. Expected version 2."` | `.vgjson` 中 version 不是 2 |
| `"Invalid shape data at index N."` | 第 N 个图形的 type 无效或缺失 |
| `"Duplicate shape id \"X\" at index N."` | id X 在之前已出现过 |
| `"Unsupported transform at shape index N. Rotation is no longer supported."` | 图形含旋转/剪切变换 |

## 开发调试技巧

1. 直接运行测试二进制而非通过 ctest，可用调试器附加：
   ```bash
   lldb ./out/build/darwin-debug/shape_data_tests
   ```
2. `qDebug()` 打印 `ShapeData` 到 stderr
3. `QJsonDocument::toJson(QJsonDocument::Indented)` 可在断点查看完整文件内容
4. 临时在 `CanvasView::mousePressEvent` 添加 `qDebug() << "tool:" << (int)m_tool` 可追踪工具状态
