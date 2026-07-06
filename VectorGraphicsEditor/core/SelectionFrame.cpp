// =====================================================================
// SelectionFrame.cpp
// ---------------------------------------------------------------------
// @brief   SelectionFrame.h 中声明方法的实现
// @details 这里不依赖 Widgets，仅做几何运算：
//          - `localToSceneTransform()` 把局部单位正方形映射到当前 frame
//          - `mapped()` 用仿射变换整体更新 frame，供缩放/平移会话复用
// @layer   core
// =====================================================================

#include "core/SelectionFrame.h"

#include <algorithm>
#include <cmath>

#include <QtMath>

namespace {

constexpr qreal kMinimumAxisLength = 1.0;

QPointF normalizedVector(const QPointF& vector) {
    const qreal length = std::hypot(vector.x(), vector.y());
    if (qFuzzyIsNull(length)) {
        return QPointF(0.0, -1.0);
    }
    return QPointF(vector.x() / length, vector.y() / length);
}

qreal dotProduct(const QPointF& lhs, const QPointF& rhs) { return lhs.x() * rhs.x() + lhs.y() * rhs.y(); }

qreal crossProduct(const QPointF& lhs, const QPointF& rhs) { return lhs.x() * rhs.y() - lhs.y() * rhs.x(); }

} // namespace

SelectionFrame SelectionFrame::fromRect(const QRectF& rect) {
    const QRectF normalizedRect = rect.normalized();

    SelectionFrame frame;
    frame.topLeft = normalizedRect.topLeft();
    frame.xAxis = QPointF(std::max(normalizedRect.width(), kMinimumAxisLength), 0.0);
    frame.yAxis = QPointF(0.0, std::max(normalizedRect.height(), kMinimumAxisLength));
    return frame;
}

bool SelectionFrame::isValid() const {
    return std::hypot(xAxis.x(), xAxis.y()) > 0.0 && std::hypot(yAxis.x(), yAxis.y()) > 0.0;
}

bool SelectionFrame::isOrthogonal(qreal tolerance) const {
    const qreal xLength = std::hypot(xAxis.x(), xAxis.y());
    const qreal yLength = std::hypot(yAxis.x(), yAxis.y());
    if (qFuzzyIsNull(xLength) || qFuzzyIsNull(yLength)) {
        return false;
    }

    return std::abs(dotProduct(xAxis, yAxis)) <= tolerance * xLength * yLength;
}

SelectionFrame SelectionFrame::orthonormalized() const {
    if (!isValid()) {
        return *this;
    }

    const QPointF frameCenter = center();
    QPointF xDirection = normalizedVector(xAxis);
    const qreal xLength = std::max(std::hypot(xAxis.x(), xAxis.y()), kMinimumAxisLength);
    const qreal yLength = std::max(std::hypot(yAxis.x(), yAxis.y()), kMinimumAxisLength);
    const qreal orientation = crossProduct(xAxis, yAxis) < 0.0 ? -1.0 : 1.0;
    const QPointF yDirection(orientation * -xDirection.y(), orientation * xDirection.x());

    SelectionFrame frame;
    frame.xAxis = QPointF(xDirection.x() * xLength, xDirection.y() * xLength);
    frame.yAxis = QPointF(yDirection.x() * yLength, yDirection.y() * yLength);
    frame.topLeft = frameCenter - 0.5 * (frame.xAxis + frame.yAxis);
    return frame;
}

QPointF SelectionFrame::topRight() const { return topLeft + xAxis; }

QPointF SelectionFrame::bottomLeft() const { return topLeft + yAxis; }

QPointF SelectionFrame::bottomRight() const { return topLeft + xAxis + yAxis; }

QPointF SelectionFrame::center() const { return topLeft + 0.5 * (xAxis + yAxis); }

QPolygonF SelectionFrame::polygon() const { return QPolygonF({topLeft, topRight(), bottomRight(), bottomLeft()}); }

QRectF SelectionFrame::boundingRect() const { return polygon().boundingRect(); }

QTransform SelectionFrame::localToSceneTransform() const {
    return QTransform(xAxis.x(), xAxis.y(), yAxis.x(), yAxis.y(), topLeft.x(), topLeft.y());
}

SelectionFrame SelectionFrame::mapped(const QTransform& transform) const {
    SelectionFrame frame;
    frame.topLeft = transform.map(topLeft);
    frame.xAxis = transform.map(topRight()) - frame.topLeft;
    frame.yAxis = transform.map(bottomLeft()) - frame.topLeft;
    return frame;
}

SelectionFrame SelectionFrame::translated(const QPointF& delta) const {
    SelectionFrame frame = *this;
    frame.topLeft += delta;
    return frame;
}
