// =====================================================================
// CanvasViewInput.cpp
// ---------------------------------------------------------------------
// @brief   CanvasView 的“原始输入分发”实现
// @details 负责接住 Qt 鼠标/键盘事件，并按当前工具与会话状态把输入路由到
//          创建策略、平移会话或缩放会话。这里的职责是分发，不直接做几何计算。
// @layer   canvas
// =====================================================================

#include "canvas/CanvasView.h"

#include <QApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>

#include <limits>

#include "canvas/CreationStrategy.h"

namespace {

/// @brief 判断当前工具是否走“按下-拖拽-抬起”的创建流程。
bool isDragTool(CanvasView::Tool tool) {
    return tool == CanvasView::Tool::Line || tool == CanvasView::Tool::Rectangle || tool == CanvasView::Tool::Circle ||
           tool == CanvasView::Tool::Ellipse;
}

} // namespace

/// @brief 鼠标按下时根据当前工具进入“缩放 / 创建 / 普通选择”三类入口之一。
void CanvasView::mousePressEvent(QMouseEvent* event) {
    const QPointF scenePoint = mapToScene(event->pos());
    m_selectionDragCandidate = false;

    // Select 模式优先把命中到的角点解释为缩放手柄；这样 overlay 永远先于
    // scene 内部的 item selection 处理，避免点击手柄时误触发普通选中。
    if (m_tool == Tool::Select && event->button() == Qt::LeftButton && m_selectionOverlay != nullptr &&
        m_selectionOverlay->isVisible()) {
        const SelectionTransformOverlayItem::Handle handle = m_selectionOverlay->handleAt(scenePoint);
        if (handle != SelectionTransformOverlayItem::Handle::None) {
            beginTransformSession(handle, scenePoint);
            return;
        }
    }

    if (event->button() == Qt::LeftButton) {
        if (m_tool == Tool::Point) {
            ShapeData data;
            data.id = generateShapeId();
            data.type = ShapeType::Point;
            data.points = {scenePoint};
            data.style = currentStyleFor(ShapeType::Point);
            data.zValue = nextZValue();
            addShape(data, true);
            return;
        }

        if (isDragTool(m_tool)) {
            m_dragStrategy->begin(scenePoint);
            return;
        }

        if (ICreationStrategy* strategy = activePathStrategy(); strategy != nullptr) {
            strategy->begin(scenePoint);
            return;
        }
    }

    QGraphicsView::mousePressEvent(event);

    if (m_tool == Tool::Select && event->button() == Qt::LeftButton) {
        // 只有“按下的本来就是已选中图形”时才 arm 平移候选，保留橡皮筋框选与
        // 普通点击切换 selection 的原始行为。
        if (ShapeItem* item = shapeItemAtViewPosition(event->pos()); item != nullptr && item->isSelected()) {
            armSelectionDrag(event->pos(), scenePoint);
        }
    }
}

/// @brief 鼠标移动时优先推进已有会话，其次再把事件交给创建策略或 QGraphicsView。
void CanvasView::mouseMoveEvent(QMouseEvent* event) {
    const QPointF scenePoint = mapToScene(event->pos());

    if (m_transformSession.active) {
        updateTransformSession(scenePoint, event->modifiers());
        return;
    }

    if (m_selectionDragSession.active) {
        updateSelectionDrag(scenePoint);
        return;
    }

    if (m_dragStrategy->inProgress()) {
        m_dragStrategy->update(scenePoint);
        return;
    }

    if (ICreationStrategy* strategy = activePathStrategy(); strategy != nullptr && strategy->inProgress()) {
        strategy->update(scenePoint);
    }

    QGraphicsView::mouseMoveEvent(event);

    if (m_tool != Tool::Select || !event->buttons().testFlag(Qt::LeftButton)) {
        return;
    }

    // 平移候选会先经历一个“按下但尚未进入拖动”的阶段，直到越过系统拖动阈值，
    // 这样点击已选中图形不会立即触发位置抖动。
    if (m_selectionDragCandidate &&
        (event->pos() - m_selectionMovePressViewPos).manhattanLength() >= QApplication::startDragDistance()) {
        beginSelectionDrag();
    }

    if (m_selectionDragSession.active) {
        updateSelectionDrag(scenePoint);
    }
}

/// @brief 鼠标释放时结束当前活跃的创建/平移/缩放会话。
void CanvasView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_transformSession.active) {
        finishTransformSession();
        return;
    }

    if (event->button() == Qt::LeftButton && m_dragStrategy->inProgress()) {
        m_dragStrategy->finish(mapToScene(event->pos()));
        return;
    }

    if (event->button() == Qt::LeftButton && m_selectionDragSession.active) {
        finishSelectionDrag();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);

    if (m_tool == Tool::Select && event->button() == Qt::LeftButton) {
        m_selectionDragCandidate = false;
    }
}

/// @brief 双击是路径类工具的“显式收尾”入口。
void CanvasView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (ICreationStrategy* strategy = activePathStrategy(); strategy != nullptr && strategy->inProgress()) {
            strategy->finish(mapToScene(event->pos()));
            return;
        }
    }

    QGraphicsView::mouseDoubleClickEvent(event);
}

/// @brief 键盘入口：完成路径、取消会话、删除、复制粘贴。
void CanvasView::keyPressEvent(QKeyEvent* event) {
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
        if (ICreationStrategy* strategy = activePathStrategy(); strategy != nullptr && strategy->inProgress()) {
            // PathCreationStrategy 约定：传入 NaN 作为“无额外末点，仅结束当前路径”。
            const qreal nan = std::numeric_limits<qreal>::quiet_NaN();
            strategy->finish(QPointF(nan, nan));
            return;
        }
    }

    if (event->key() == Qt::Key_Escape) {
        if (m_selectionDragSession.active) {
            cancelSelectionDrag();
            return;
        }
        if (m_transformSession.active) {
            cancelTransformSession();
            return;
        }
        cancelAllStrategies();
        return;
    }

    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        emit deleteSelectionRequested();
        return;
    }

    if (event->matches(QKeySequence::Copy)) {
        copySelectedItem();
        return;
    }

    if (event->matches(QKeySequence::Paste)) {
        pasteCopiedItem();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

/// @brief Select 之外只有 Polyline / Polygon 需要长生命周期路径策略。
ICreationStrategy* CanvasView::activePathStrategy() const {
    if (m_tool == Tool::Polyline) {
        return m_polylineStrategy.get();
    }
    if (m_tool == Tool::Polygon) {
        return m_polygonStrategy.get();
    }
    return nullptr;
}

/// @brief 在工具切换或 Esc 时取消所有仍处于进行中的创建策略。
void CanvasView::cancelAllStrategies() {
    if (m_dragStrategy != nullptr) {
        m_dragStrategy->cancel();
    }
    if (m_polylineStrategy != nullptr) {
        m_polylineStrategy->cancel();
    }
    if (m_polygonStrategy != nullptr) {
        m_polygonStrategy->cancel();
    }
}

/// @brief 判断给定 item 是否是任一创建策略暂存的 preview item。
bool CanvasView::isPreviewItem(const ShapeItem* item) const {
    if (item == nullptr) {
        return false;
    }

    return (m_dragStrategy != nullptr && m_dragStrategy->isPreviewItem(item)) ||
           (m_polylineStrategy != nullptr && m_polylineStrategy->isPreviewItem(item)) ||
           (m_polygonStrategy != nullptr && m_polygonStrategy->isPreviewItem(item));
}

/// @brief 从视图坐标命中 ShapeItem；用于“已选中图形是否进入平移候选”的判断。
ShapeItem* CanvasView::shapeItemAtViewPosition(const QPoint& viewPosition) const {
    if (QGraphicsItem* graphicsItem = itemAt(viewPosition)) {
        return qgraphicsitem_cast<ShapeItem*>(graphicsItem);
    }
    return nullptr;
}
