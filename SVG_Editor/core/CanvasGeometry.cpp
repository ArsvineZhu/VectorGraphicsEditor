// =====================================================================
// CanvasGeometry.cpp
// ---------------------------------------------------------------------
// @brief CanvasGeometry.h 的实现
// @details 这些函数原本在 CanvasView.cpp 的匿名 namespace 中；本文件保持原行为
//          不变，仅提升可见性以便 core 单元测试覆盖。
// @layer   core
// =====================================================================

#include "core/CanvasGeometry.h"

#include <algorithm>
#include <cmath>

namespace {

// 缩放不允许越过对角点翻转；当鼠标跨过 pivot 时，最小正缩放系数用于维持
// “不翻转、但仍继续响应拖拽”的产品语义。
constexpr qreal kMinimumScaleFactor = 0.01;

/// @brief 把角点手柄映射到局部单位矩形的标准坐标。
QPointF localPointForHandleImpl(CanvasHandle handle) {
    switch (handle) {
    case CanvasHandle::TopLeft:
        return QPointF(0.0, 0.0);
    case CanvasHandle::TopRight:
        return QPointF(1.0, 0.0);
    case CanvasHandle::BottomLeft:
        return QPointF(0.0, 1.0);
    case CanvasHandle::BottomRight:
        return QPointF(1.0, 1.0);
    case CanvasHandle::None:
        return QPointF(0.5, 0.5);
    }
    return QPointF(0.5, 0.5);
}

/// @brief 返回给定角点手柄在局部单位矩形中的对角 pivot。
CanvasHandle oppositeHandle(CanvasHandle handle) {
    switch (handle) {
    case CanvasHandle::TopLeft:
        return CanvasHandle::BottomRight;
    case CanvasHandle::TopRight:
        return CanvasHandle::BottomLeft;
    case CanvasHandle::BottomLeft:
        return CanvasHandle::TopRight;
    case CanvasHandle::BottomRight:
        return CanvasHandle::TopLeft;
    case CanvasHandle::None:
        return CanvasHandle::None;
    }
    return CanvasHandle::None;
}

/// @brief 二维点积；用于把任意场景位移投影到 frame 的局部轴。
qreal dotProduct(const QPointF& lhs, const QPointF& rhs) { return lhs.x() * rhs.x() + lhs.y() * rhs.y(); }

/// @brief 向量长度 helper。
qreal vectorLength(const QPointF& vector) { return std::hypot(vector.x(), vector.y()); }

/// @brief 向量归一化；零向量返回空向量，由调用方决定兜底行为。
QPointF normalizedVector(const QPointF& vector) {
    const qreal length = vectorLength(vector);
    if (qFuzzyIsNull(length)) {
        return QPointF();
    }
    return QPointF(vector.x() / length, vector.y() / length);
}

} // namespace

namespace canvas_geometry {

/// @brief 由拖拽对角点生成圆的正方形外接框。
QRectF circleRectFromPoints(const QPointF& start, const QPointF& end) {
    const qreal dx = end.x() - start.x();
    const qreal dy = end.y() - start.y();
    const qreal side = std::max(std::abs(dx), std::abs(dy));
    const qreal x = dx >= 0.0 ? start.x() : start.x() - side;
    const qreal y = dy >= 0.0 ? start.y() : start.y() - side;
    return QRectF(x, y, side, side);
}

/// @brief 从当前鼠标位置直接构造缩放后的目标 SelectionFrame。
/// @details 核心约束：
///          1. 当前被拖拽角点在普通缩放下跟随鼠标；
///          2. 对角 pivot 保持不动；
///          3. 沿局部 x/y 轴分别投影缩放，保留正交 frame；
///          4. 如果鼠标越过 pivot，则被 no-flip 逻辑夹紧为最小正缩放。
SelectionFrame scaledFrameFromHandle(CanvasHandle handle, const SelectionFrame& frame, const QPointF& currentPoint,
                                     bool keepAspectRatio) {
    const SelectionFrame baseFrame = frame.isOrthogonal() ? frame : frame.orthonormalized();
    const qreal xLength = vectorLength(baseFrame.xAxis);
    const qreal yLength = vectorLength(baseFrame.yAxis);
    if (qFuzzyIsNull(xLength) || qFuzzyIsNull(yLength)) {
        return baseFrame;
    }

    const QPointF handleLocal = localPointForHandleImpl(handle);
    const QPointF pivotLocal = localPointForHandleImpl(oppositeHandle(handle));
    const qreal signX = handleLocal.x() - pivotLocal.x();
    const qreal signY = handleLocal.y() - pivotLocal.y();
    const QPointF xDirection = normalizedVector(baseFrame.xAxis);
    const QPointF yDirection = normalizedVector(baseFrame.yAxis);
    const QPointF pivotPoint = baseFrame.topLeft + baseFrame.xAxis * pivotLocal.x() + baseFrame.yAxis * pivotLocal.y();
    const QPointF pivotToCurrent = currentPoint - pivotPoint;
    qreal scaleX = std::max((signX * dotProduct(pivotToCurrent, xDirection)) / xLength, kMinimumScaleFactor);
    qreal scaleY = std::max((signY * dotProduct(pivotToCurrent, yDirection)) / yLength, kMinimumScaleFactor);

    if (keepAspectRatio) {
        // 等比缩放时沿两个局部轴取同一倍率；这会牺牲“角点严格贴住鼠标”，
        // 但保留用户显式请求的 uniform-scale 语义。
        const qreal magnitude = std::max(scaleX, scaleY);
        scaleX = magnitude;
        scaleY = magnitude;
    }

    SelectionFrame scaledFrame;
    scaledFrame.xAxis = xDirection * (xLength * scaleX);
    scaledFrame.yAxis = yDirection * (yLength * scaleY);
    scaledFrame.topLeft = pivotPoint - scaledFrame.xAxis * pivotLocal.x() - scaledFrame.yAxis * pivotLocal.y();
    return scaledFrame;
}

/// @brief 计算 source frame 到 target frame 的单次场景空间仿射变换。
QTransform transformBetweenFrames(const SelectionFrame& source, const SelectionFrame& target) {
    bool invertible = false;
    const QTransform sourceToLocal = source.localToSceneTransform().inverted(&invertible);
    if (!invertible) {
        return QTransform();
    }

    return sourceToLocal * target.localToSceneTransform();
}

/// @brief 直接把缩放结果转换成场景空间仿射变换。
QTransform scaleTransformFromHandle(CanvasHandle handle, const SelectionFrame& frame, const QPointF& currentPoint,
                                    bool keepAspectRatio) {
    return transformBetweenFrames(frame, scaledFrameFromHandle(handle, frame, currentPoint, keepAspectRatio));
}

} // namespace canvas_geometry
