// =====================================================================
// CanvasViewSelection.cpp
// ---------------------------------------------------------------------
// @brief   CanvasView 的“选择 / overlay / 变换会话”实现
// @details 负责从当前选中项构造 SelectionFrame，维护多图形快照，并把缩放 /
//          平移更新稳定地映射回 ShapeData::transform。
// @layer   canvas
// =====================================================================

#include "canvas/CanvasView.h"

#include <QGraphicsScene>

#include "core/CanvasGeometry.h"

namespace {

/// @brief 计算一组图形在场景空间中的联合包围盒。
QRectF sceneBoundsForItems(const QList<ShapeItem*>& items) {
    QRectF bounds;
    bool first = true;
    for (ShapeItem* item : items) {
        if (item == nullptr) {
            continue;
        }

        const QRectF itemBounds = item->sceneTransform().map(item->shape()).boundingRect();
        if (first) {
            bounds = itemBounds;
            first = false;
        } else {
            bounds = bounds.united(itemBounds);
        }
    }

    return bounds.normalized();
}

} // namespace

/// @brief 单选时返回唯一 ShapeItem，多选或空选区返回 nullptr。
ShapeItem* CanvasView::selectedShapeItem() const {
    const QList<ShapeItem*> items = selectedShapeItems();
    return items.size() == 1 ? items.first() : nullptr;
}

/// @brief 从 scene 当前选集中过滤出 ShapeItem 列表。
QList<ShapeItem*> CanvasView::selectedShapeItems() const {
    QList<ShapeItem*> items;
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto* shapeItem = qgraphicsitem_cast<ShapeItem*>(item)) {
            items.append(shapeItem);
        }
    }
    return items;
}

/// @brief scene selection 变化时刷新 overlay，并清理旧的拖动候选状态。
void CanvasView::handleSelectionChanged() {
    m_selectionDragCandidate = false;
    m_selectionDragSession = {};
    normalizeSelectedShapeTransforms();
    invalidateSelectionFrame();
    refreshSelectionNotification();
}

/// @brief 从当前选集构造交互用 SelectionFrame。
/// @details 单选时保留图形自身变换后的局部轴；多选时退回联合 AABB，因为多图形
///          共享旋转/剪切在当前产品语义里并不是受支持编辑面。
std::optional<SelectionFrame> CanvasView::buildSelectionFrameFromSelection() const {
    const QList<ShapeItem*> items = selectedShapeItems();
    if (items.isEmpty()) {
        return std::nullopt;
    }

    if (items.size() == 1) {
        const ShapeItem* item = items.first();
        const SelectionFrame frame = SelectionFrame::fromRect(item->localGeometryBounds());
        return frame.mapped(item->shapeData().transform).orthonormalized();
    }

    const QRectF bounds = sceneBoundsForItems(items);
    if (bounds.isNull() || bounds.isEmpty()) {
        return std::nullopt;
    }

    return SelectionFrame::fromRect(bounds);
}

/// @brief 选中图形若带旧的 shear，会先归一化到正交 frame 再进入交互链路。
void CanvasView::normalizeSelectedShapeTransforms() {
    const QList<ShapeItem*> items = selectedShapeItems();
    for (ShapeItem* item : items) {
        if (item == nullptr) {
            continue;
        }

        const SelectionFrame localFrame = SelectionFrame::fromRect(item->localGeometryBounds());
        const SelectionFrame mappedFrame = localFrame.mapped(item->shapeData().transform);
        if (mappedFrame.isOrthogonal()) {
            continue;
        }

        ShapeData data = item->shapeData();
        data.transform = canvas_geometry::transformBetweenFrames(localFrame, mappedFrame.orthonormalized());
        item->setShapeData(data);
    }
}

void CanvasView::invalidateSelectionFrame() { m_selectionFrame.reset(); }

/// @brief 统一刷新“item 装饰 + overlay + 外部 selection 信号”。
void CanvasView::refreshSelectionNotification() {
    const QList<ShapeItem*> items = selectedShapeItems();
    updateSelectionDecorations();
    updateSelectionOverlay();
    emit selectionStateChanged(items.size() == 1 ? items.first() : nullptr, static_cast<int>(items.size()));
}

/// @brief 根据当前工具和会话状态显示/隐藏 selection overlay。
void CanvasView::updateSelectionOverlay() {
    if (m_selectionOverlay == nullptr) {
        return;
    }

    if (m_tool != Tool::Select) {
        m_selectionOverlay->clearSelectionFrame();
        return;
    }

    if (selectedShapeCount() == 1 && !m_selectionDragSession.active && !m_transformSession.active) {
        m_selectionFrame = buildSelectionFrameFromSelection();
    }

    if (!m_selectionFrame.has_value()) {
        m_selectionFrame = buildSelectionFrameFromSelection();
    }

    if (!m_selectionFrame.has_value()) {
        m_selectionOverlay->clearSelectionFrame();
        return;
    }

    m_selectionOverlay->setHandlesVisible(!m_selectionDragSession.active && !m_transformSession.active);
    m_selectionOverlay->setSelectionFrame(*m_selectionFrame);
}

/// @brief Select 工具下由 overlay 接管选框，其它工具则回退到 item 自绘装饰。
void CanvasView::updateSelectionDecorations() {
    const bool showItemSelectionDecoration = m_tool != Tool::Select;
    for (QGraphicsItem* graphicsItem : m_scene->items()) {
        if (auto* shapeItem = qgraphicsitem_cast<ShapeItem*>(graphicsItem)) {
            shapeItem->setSelectionDecorationVisible(showItemSelectionDecoration);
        }
    }
}

/// @brief 进入平移候选态，但还不真正开始改写图形位置。
void CanvasView::armSelectionDrag(const QPoint& viewPosition, const QPointF& scenePoint) {
    m_selectionDragCandidate = true;
    m_selectionMovePressViewPos = viewPosition;
    m_selectionDragSession = {};
    m_selectionDragSession.pressScenePoint = scenePoint;
}

/// @brief 正式开始平移会话，并快照当前选中项。
void CanvasView::beginSelectionDrag() {
    if (!m_selectionDragCandidate) {
        return;
    }

    if (!m_selectionFrame.has_value()) {
        m_selectionFrame = buildSelectionFrameFromSelection();
    }
    if (!m_selectionFrame.has_value()) {
        return;
    }

    const QList<ShapeItem*> items = selectedShapeItems();
    if (items.isEmpty()) {
        return;
    }

    // 拖动基准必须保留“按下时”的场景坐标；否则一旦会话 begin() 清空字段，
    // 第一帧 delta 会错误地从 (0, 0) 起算，图形就会瞬间跳走。
    const QPointF pressScenePoint = m_selectionDragSession.pressScenePoint;
    m_selectionDragSession = {};
    m_selectionDragSession.begin(items, *m_selectionFrame);
    m_selectionDragSession.pressScenePoint = pressScenePoint;
    m_selectionDragCandidate = false;
    updateSelectionOverlay();
}

/// @brief 用场景空间平移量实时更新所有选中图形。
void CanvasView::updateSelectionDrag(const QPointF& scenePoint) {
    if (!m_selectionDragSession.active) {
        return;
    }

    const QPointF delta = scenePoint - m_selectionDragSession.pressScenePoint;
    QTransform translation;
    translation.translate(delta.x(), delta.y());

    for (const MultiShapeEntry& entry : m_selectionDragSession.entries) {
        if (entry.item == nullptr) {
            continue;
        }

        ShapeData data = entry.snapshot;
        data.transform = entry.snapshot.transform * translation;
        entry.item->setShapeData(data);
    }

    if (m_selectionDragSession.entries.size() == 1) {
        m_selectionFrame = buildSelectionFrameFromSelection();
    } else {
        m_selectionFrame = m_selectionDragSession.originalFrame.translated(delta);
    }
    updateSelectionOverlay();
}

/// @brief 提交平移会话：当前位置已经写入 ShapeData，只需丢弃快照并刷新通知。
void CanvasView::finishSelectionDrag() {
    if (!m_selectionDragSession.active) {
        return;
    }

    m_selectionDragCandidate = false;
    m_selectionDragSession = {};
    refreshSelectionNotification();
}

/// @brief 撤销平移会话：恢复 MultiShapeSession 初始快照。
void CanvasView::cancelSelectionDrag() {
    if (!m_selectionDragSession.active) {
        m_selectionDragCandidate = false;
        return;
    }

    m_selectionDragSession.cancel();
    m_selectionFrame = m_selectionDragSession.originalFrame;
    m_selectionDragCandidate = false;
    m_selectionDragSession = {};
    refreshSelectionNotification();
}

/// @brief 开始缩放会话：记录命中的锚点与完整选中快照。
void CanvasView::beginTransformSession(SelectionTransformOverlayItem::Handle handle, const QPointF&) {
    const QList<ShapeItem*> items = selectedShapeItems();
    if (items.isEmpty()) {
        return;
    }

    if (!m_selectionFrame.has_value()) {
        m_selectionFrame = buildSelectionFrameFromSelection();
    }
    if (!m_selectionFrame.has_value()) {
        return;
    }

    m_transformSession = {};
    m_transformSession.begin(items, *m_selectionFrame);
    m_transformSession.handle = handle;

    updateSelectionOverlay();
}

/// @brief 用“原始 frame -> 目标 frame -> delta transform”的模式推进缩放。
void CanvasView::updateTransformSession(const QPointF& scenePoint, Qt::KeyboardModifiers modifiers) {
    if (!m_transformSession.active) {
        return;
    }

    const bool keepAspectRatio = modifiers.testFlag(Qt::ShiftModifier);
    const SelectionFrame targetFrame =
        canvas_geometry::scaledFrameFromHandle(m_transformSession.handle, m_transformSession.originalFrame, scenePoint,
                                               keepAspectRatio);
    const QTransform deltaTransform = canvas_geometry::transformBetweenFrames(m_transformSession.originalFrame, targetFrame);

    if (m_transformSession.entries.size() == 1) {
        const MultiShapeEntry& entry = m_transformSession.entries.first();
        if (entry.item != nullptr) {
            ShapeData data = entry.snapshot;
            const SelectionFrame localFrame = SelectionFrame::fromRect(entry.item->localGeometryBounds());
            // 单选时直接把“本地图形 frame -> 目标 frame”映射成新的 transform，
            // 这样几何角点能严格对齐当前缩放 overlay。
            data.transform = canvas_geometry::transformBetweenFrames(localFrame, targetFrame);
            entry.item->setShapeData(data);
        }
        m_selectionFrame = targetFrame;
    } else {
        for (const MultiShapeEntry& entry : m_transformSession.entries) {
            if (entry.item == nullptr) {
                continue;
            }

            ShapeData data = entry.snapshot;
            data.transform = entry.snapshot.transform * deltaTransform;
            entry.item->setShapeData(data);
        }
        m_selectionFrame = targetFrame;
    }
    updateSelectionOverlay();
}

/// @brief 提交缩放会话；单选时额外丢弃缓存 frame，强制下次从图形真实状态重建。
void CanvasView::finishTransformSession() {
    if (!m_transformSession.active) {
        return;
    }

    m_transformSession = {};
    if (selectedShapeCount() == 1) {
        invalidateSelectionFrame();
    }
    refreshSelectionNotification();
}

/// @brief 撤销缩放会话：恢复快照并把 overlay 放回原 frame。
void CanvasView::cancelTransformSession() {
    if (!m_transformSession.active) {
        return;
    }

    m_transformSession.cancel();
    m_selectionFrame = m_transformSession.originalFrame;
    m_transformSession = {};
    refreshSelectionNotification();
}
