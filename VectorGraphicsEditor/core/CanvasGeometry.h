// =====================================================================
// CanvasGeometry.h
// ---------------------------------------------------------------------
// @brief   CanvasView 内部几何 helper 的 core 层版本（无 QGraphicsView 依赖）
// @details CanvasView 的匿名 namespace 里有一批"纯函数"helper（不依赖
//          QGraphicsItem / QGraphicsScene 状态），本文件把它们提升到 core 静态库，
//          便于编写单元测试与在其它视图（如缩略图、工具栏图标）复用。
// @layer   core
// @warning 内部签名 / 行为变更须同步更新 CanvasView 的调用点。
// =====================================================================

#pragma once

#include <QPointF>
#include <QRectF>
#include <QTransform>

#include <cstdint>

#include "core/SelectionFrame.h"

/// @brief 选中手柄位置（core 层定义；QGraphicsView 层通过 SelectionTransformOverlayItem 暴露同名列）
enum class CanvasHandle : std::uint8_t {
    None,        ///< 无命中
    TopLeft,     ///< 左上角缩放手柄
    TopRight,    ///< 右上角缩放手柄
    BottomLeft,  ///< 左下角缩放手柄
    BottomRight, ///< 右下角缩放手柄
};

namespace canvas_geometry {

/// @brief 从拖拽的起、终点构造圆的外接正方形。
QRectF circleRectFromPoints(const QPointF& start, const QPointF& end);

/// @brief 构造"沿 frame 局部轴缩放"的场景空间变换。
/// @details 普通拖拽时，当前手柄角点会跟随 `currentPoint`；对角 pivot 保持不动。
QTransform scaleTransformFromHandle(CanvasHandle handle, const SelectionFrame& frame, const QPointF& currentPoint,
                                    bool keepAspectRatio);

/// @brief 根据当前拖拽位置直接计算目标正交 frame。
/// @details 普通拖拽时，被拖拽角点严格跟随 `currentPoint`，对角 pivot 保持不动。
SelectionFrame scaledFrameFromHandle(CanvasHandle handle, const SelectionFrame& frame, const QPointF& currentPoint,
                                     bool keepAspectRatio);

/// @brief 计算两个 frame 之间的场景空间增量变换。
QTransform transformBetweenFrames(const SelectionFrame& source, const SelectionFrame& target);

} // namespace canvas_geometry
