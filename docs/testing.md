# 测试体系

测试框架：**Qt Test**（`QTest`） + **CTest** 编排。

## 测试一览

| 二进制 | 源文件 | 项数 | 依赖 | 覆盖范围 |
|--------|--------|------|------|---------|
| `shape_data_tests` | `tests/core/ShapeDataTests.cpp` | 14 | `Qt6::Test` | JSON 往返、归一化、缺字段拒绝、选择框变换和平移 |
| `file_manager_tests` | `tests/core/FileManagerTests.cpp` | 5 | `Qt6::Test` | 读写往返、版本 1 拒绝、重复 id 拒绝、非法变换拒绝（加载+保存） |
| `canvas_geometry_tests` | `tests/core/CanvasGeometryTests.cpp` | 10 | `Qt6::Test` | 圆矩形构造、正交化、缩放变换、等比缩放、防翻转夹紧 |
| `shape_item_tests` | `tests/graphics/ShapeItemTests.cpp` | 9 | `Qt6::Test` + `Qt6::Widgets` | 7 种图形路径构建、交互区域外扩、预览画笔 |
| `main_window_tests` | `tests/ui/MainWindowTests.cpp` | 1 | `Qt6::Test` + `Qt6::Widgets` | Tools 菜单结构验证 |

## 运行

```bash
# 全部
ctest --preset test-darwin-debug --output-on-failure

# 按名称
ctest --preset test-darwin-debug -R shape_data_tests

# 直接运行二进制（调试用）
./out/build/darwin-debug/shape_data_tests
```

## 测试用例详情

### shape_data_tests（14 项）

- 圆/矩形/折线/椭圆 JSON 序列化往返
- 折线平移移动所有顶点
- 变换复合到 ShapeData
- 归一化夹紧负尺寸、强制圆正方形、幂等性
- 缺 transform / strokeEnabled 反序列化失败
- 选择框跟随仿射变换、平移移动所有角

### file_manager_tests（5 项）

- saveAndLoadDocument — 完整读写往返
- rejectOldOrIncompleteDocument — 拒绝 version 1 和缺字段
- rejectDuplicateShapeId — 拒绝重复 id
- rejectUnsupportedTransformOnLoad — 加载时拒绝旋转的 transform
- rejectUnsupportedTransformOnSave — 保存时拒绝旋转的 transform

### canvas_geometry_tests（10 项）

- circleRectFromDiagonalDrag / circleRectFromAntiDiagonalDrag
- orthonormalizedFrameKeepsCenterAndRemovesShear
- scaleTransformDoublesFrameSizeFromTopLeft
- draggedHandleFollowsMouseOnAxisAlignedFrame / OnOrientedFrame
- scaledOrientedFrameKeepsAxesPerpendicular
- scaleClampPreventsFlipAcrossPivot
- keepAspectRatioPreservesPivotAndUniformScale
- transformBetweenFramesMapsCornersExactly

### shape_item_tests（9 项）

- pointBuildPathHasEllipseOfExpectedRadius
- lineBuildPathHasMoveAndLine
- polylineBuildPathHasMultipleSegments
- polygonBuildPathClosedWhenNotPreview / OpenInPreview
- circleAndEllipseBuildPathHasEllipse
- rectangleBuildPathHasRect
- interactionPathExpandsByStrokePlus6WithMin8
- previewModePenIsLighterAndDashed
- emptyPathPassesThroughInteractionPath

### main_window_tests（1 项）

- toolsMenuContainsExpectedGroupedSubmenus — 验证工具菜单含 3 个子菜单，动作数分别为 1/3/4

## 链接策略

- Core 层测试（`shape_data_tests` / `file_manager_tests` / `canvas_geometry_tests`）：仅 `Qt6::Test`
- Graphics 层测试（`shape_item_tests`）：`Qt6::Test` + `Qt6::Widgets`
- UI 层测试（`main_window_tests`）：`Qt6::Test` + `Qt6::Widgets` + 全部 app 源码（除 `main.cpp`）
