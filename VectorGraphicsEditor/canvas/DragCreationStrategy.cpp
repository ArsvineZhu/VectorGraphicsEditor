#include "canvas/DragCreationStrategy.h"

#include <QGraphicsScene>
#include <QLineF>

#include "core/CanvasGeometry.h"
#include "core/CanvasViewConstants.h"
#include "graphics/ShapeFactory.h"
#include "graphics/ShapeItem.h"

DragCreationStrategy::DragCreationStrategy(CreationContext context) : m_context(std::move(context)) {}

void DragCreationStrategy::begin(const QPointF& scenePoint) {
    cancel();
    m_dragDrawing = true;
    m_dragStartPoint = scenePoint;

    ShapeData preview = buildDragShapeData(scenePoint);
    preview.zValue = m_context.nextZValue();
    std::unique_ptr<ShapeItem> item = ShapeFactory::createItem(preview);
    item->setPreviewMode(true);
    m_previewItem = item.release();
    m_context.scene->addItem(m_previewItem);
}

void DragCreationStrategy::update(const QPointF& scenePoint) {
    if (m_previewItem == nullptr) {
        return;
    }

    m_previewItem->setShapeData(buildDragShapeData(scenePoint));
}

void DragCreationStrategy::finish(const QPointF& scenePoint) {
    if (!m_dragDrawing) {
        return;
    }

    const ShapeData data = buildDragShapeData(scenePoint);
    cancel();

    bool validShape = false;
    switch (data.type) {
    case ShapeType::Line:
        validShape = data.points.size() >= 2 && QLineF(data.points.at(0), data.points.at(1)).length() > kSelectionDragMinDistance;
        break;
    case ShapeType::Circle:
    case ShapeType::Ellipse:
    case ShapeType::Rectangle:
        validShape = data.rect.width() > kSelectionDragMinDistance && data.rect.height() > kSelectionDragMinDistance;
        break;
    default:
        break;
    }

    if (validShape) {
        m_context.addShape(data, true);
    }
}

void DragCreationStrategy::cancel() {
    m_dragDrawing = false;
    if (m_previewItem != nullptr) {
        m_context.scene->removeItem(m_previewItem);
        delete m_previewItem;
        m_previewItem = nullptr;
    }
}

bool DragCreationStrategy::inProgress() const { return m_dragDrawing; }

bool DragCreationStrategy::isPreviewItem(const ShapeItem* item) const { return item != nullptr && item == m_previewItem; }

ShapeData DragCreationStrategy::buildDragShapeData(const QPointF& endPoint) const {
    ShapeData data;
    data.id = m_context.generateId();
    data.type = m_context.currentShapeType().value_or(ShapeType::Rectangle);
    data.style = m_context.currentStyleFor(data.type);
    data.zValue = 0.0;

    switch (data.type) {
    case ShapeType::Line:
        data.points = {m_dragStartPoint, endPoint};
        break;
    case ShapeType::Rectangle:
    case ShapeType::Ellipse:
        data.rect = QRectF(m_dragStartPoint, endPoint).normalized();
        break;
    case ShapeType::Circle:
        data.rect = canvas_geometry::circleRectFromPoints(m_dragStartPoint, endPoint);
        break;
    default:
        break;
    }

    return normalizedShapeData(data);
}
