// =====================================================================
// SelectionTransformOverlayItem.cpp
// ---------------------------------------------------------------------
// @brief SelectionTransformOverlayItem.h 中声明方法的实现
// @details
//   - 绘制顺序：蓝色虚线选框 → 选区上沿到旋转手柄的连接线 → 4 个角点方形
//     手柄（白底蓝框）→ 旋转手柄（蓝底圆）。
//   - hit-test 使用 5 个手柄的矩形区域（包含旋转手柄的包围盒，足够覆盖
//     视觉上的圆形）。
//   - 选区为空时 (`m_bounds` 为 null / 空)，所有绘制与命中测试都直接 early-return。
// =====================================================================

#include "SelectionTransformOverlayItem.h"

#include <QPainter>
#include <QPainterPath>

namespace {

// 4 个角点方形手柄的边长（场景坐标）
constexpr qreal kHandleSize = 10.0;
// 旋转手柄的半径（场景坐标）
constexpr qreal kRotateHandleRadius = 6.0;
// 旋转手柄中心到选区上沿的垂直距离（场景坐标）
constexpr qreal kRotateHandleOffset = 26.0;

/// @brief 给定中心点与边长，返回以该点为中心的正方形。
QRectF centeredRect(const QPointF& center, qreal size) {
    const qreal half = size / 2.0;
    return QRectF(center.x() - half, center.y() - half, size, size);
}

} // namespace

SelectionTransformOverlayItem::SelectionTransformOverlayItem(QGraphicsItem* parent) : QGraphicsObject(parent) {
    // 不主动消费鼠标事件：让 CanvasView 在 QGraphicsView 层拿到所有事件
    setAcceptedMouseButtons(Qt::NoButton);
    // 浮到所有 ShapeItem 之上（z 值足够大即可，不影响 hit-test 因无 hit 区）
    setZValue(1000000.0);
    // 默认无选区
    hide();
}

QRectF SelectionTransformOverlayItem::boundingRect() const {
    if (!hasFrame()) {
        return QRectF();
    }

    QRectF bounds = m_frame.boundingRect();
    if (!m_handlesVisible) {
        return bounds.adjusted(-4.0, -4.0, 4.0, 4.0);
    }

    bounds = bounds.united(handleRect(Handle::Rotate));
    bounds = bounds.united(handleRect(Handle::TopLeft));
    bounds = bounds.united(handleRect(Handle::TopRight));
    bounds = bounds.united(handleRect(Handle::BottomLeft));
    bounds = bounds.united(handleRect(Handle::BottomRight));
    return bounds.adjusted(-8.0, -8.0, 8.0, 8.0);
}

QPainterPath SelectionTransformOverlayItem::shape() const {
    QPainterPath path;
    if (!hasFrame() || !m_handlesVisible) {
        return path;
    }

    // 命中区 = 5 个手柄矩形（含旋转手柄的包围盒，覆盖视觉上的小圆已足够）
    for (Handle handle : {Handle::TopLeft, Handle::TopRight, Handle::BottomLeft, Handle::BottomRight, Handle::Rotate}) {
        path.addRect(handleRect(handle));
    }
    return path;
}

void SelectionTransformOverlayItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    if (!hasFrame()) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // 1) 选区虚框
    QPen framePen(QColor("#2d7ff9"));
    framePen.setWidthF(1.0);
    framePen.setStyle(Qt::DashLine);
    // cosmetic: 1px 不会随场景缩放变化（避免在高 zoom 下变成粗线）
    framePen.setCosmetic(true);
    painter->setPen(framePen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPolygon(m_frame.polygon());

    if (!m_handlesVisible) {
        painter->restore();
        return;
    }

    // 2) 选区上沿中点 → 旋转手柄圆心的连接线
    painter->drawLine(m_frame.topCenter(), rotateHandleCenter());

    // 3) 4 个角点方形手柄（白底蓝框）
    painter->setPen(QPen(QColor("#2d7ff9")));
    painter->setBrush(QColor("#ffffff"));
    for (Handle handle : {Handle::TopLeft, Handle::TopRight, Handle::BottomLeft, Handle::BottomRight}) {
        painter->drawRect(handleRect(handle));
    }

    // 4) 旋转手柄（蓝底圆）
    painter->setBrush(QColor("#2d7ff9"));
    painter->drawEllipse(handleRect(Handle::Rotate));
    painter->restore();
}

void SelectionTransformOverlayItem::setSelectionFrame(const SelectionFrame& frame) {
    prepareGeometryChange();
    m_frame = frame;
    m_hasFrame = frame.isValid();
    setVisible(m_hasFrame);
    update();
}

void SelectionTransformOverlayItem::clearSelectionFrame() {
    prepareGeometryChange();
    m_frame = SelectionFrame{};
    m_hasFrame = false;
    hide();
}

SelectionFrame SelectionTransformOverlayItem::selectionFrame() const { return m_frame; }

void SelectionTransformOverlayItem::setHandlesVisible(bool visible) {
    if (m_handlesVisible == visible) {
        return;
    }

    prepareGeometryChange();
    m_handlesVisible = visible;
    update();
}

bool SelectionTransformOverlayItem::handlesVisible() const { return m_handlesVisible; }

SelectionTransformOverlayItem::Handle SelectionTransformOverlayItem::handleAt(const QPointF& scenePoint) const {
    if (!hasFrame() || !m_handlesVisible) {
        return Handle::None;
    }

    // 顺序：4 个角点 → 旋转。命中即返回，第一个赢家即返回值。
    for (Handle handle : {Handle::TopLeft, Handle::TopRight, Handle::BottomLeft, Handle::BottomRight, Handle::Rotate}) {
        if (handleRect(handle).contains(scenePoint)) {
            return handle;
        }
    }

    return Handle::None;
}

QRectF SelectionTransformOverlayItem::handleRect(Handle handle) const {
    switch (handle) {
    case Handle::TopLeft:
        return centeredRect(m_frame.topLeft, kHandleSize);
    case Handle::TopRight:
        return centeredRect(m_frame.topRight(), kHandleSize);
    case Handle::BottomLeft:
        return centeredRect(m_frame.bottomLeft(), kHandleSize);
    case Handle::BottomRight:
        return centeredRect(m_frame.bottomRight(), kHandleSize);
    case Handle::Rotate:
        // 旋转手柄的 hit/test 矩形 = 2 倍半径的包围盒（paint 时画圆）
        return centeredRect(rotateHandleCenter(), kRotateHandleRadius * 2.0);
    case Handle::None:
        return QRectF();
    }

    return QRectF();
}

QPointF SelectionTransformOverlayItem::rotateHandleCenter() const {
    // 旋转后的 frame 也必须沿当前顶边外法线放置旋转手柄，才能保持交互语义一致。
    return m_frame.topCenter() + m_frame.topNormal() * kRotateHandleOffset;
}

bool SelectionTransformOverlayItem::hasFrame() const { return m_hasFrame; }
