#include "canvas/PathCreationStrategy.h"

#include <cmath>
#include <QGraphicsScene>

#include "core/CanvasViewConstants.h"
#include "graphics/ShapeFactory.h"
#include "graphics/ShapeItem.h"

PathCreationStrategy::PathCreationStrategy(ShapeType type, CreationContext context)
    : m_type(type), m_context(std::move(context)) {}

void PathCreationStrategy::begin(const QPointF& scenePoint) {
    if (m_pathPoints.isEmpty()) {
        ShapeData preview;
        preview.id = m_context.generateId();
        preview.type = m_type;
        preview.style = m_context.currentStyleFor(m_type);
        preview.zValue = m_context.nextZValue();

        std::unique_ptr<ShapeItem> item = ShapeFactory::createItem(preview);
        item->setPreviewMode(true);
        m_previewItem = item.release();
        m_context.scene->addItem(m_previewItem);
    }

    m_pathPoints.append(scenePoint);
    update(scenePoint);
}

void PathCreationStrategy::update(const QPointF& scenePoint) {
    if (m_previewItem == nullptr || m_pathPoints.isEmpty()) {
        return;
    }

    m_previewItem->setShapeData(buildPathPreviewData(scenePoint));
}

void PathCreationStrategy::finish(const QPointF& scenePoint) {
    if (m_pathPoints.isEmpty()) {
        cancel();
        return;
    }

    if (std::isfinite(scenePoint.x()) && std::isfinite(scenePoint.y()) &&
        (m_pathPoints.last() - scenePoint).manhattanLength() > kSelectionDragMinDistance) {
        m_pathPoints.append(scenePoint);
    }

    ShapeData data = buildPathPreviewData(m_pathPoints.last());
    data.points = m_pathPoints;
    data.type = m_type;

    const int minimumPoints = m_type == ShapeType::Polygon ? 3 : 2;
    cancel();

    if (data.points.size() >= minimumPoints) {
        m_context.addShape(data, true);
    }
}

void PathCreationStrategy::cancel() {
    m_pathPoints.clear();
    if (m_previewItem != nullptr) {
        m_context.scene->removeItem(m_previewItem);
        delete m_previewItem;
        m_previewItem = nullptr;
    }
}

bool PathCreationStrategy::inProgress() const { return !m_pathPoints.isEmpty(); }

bool PathCreationStrategy::isPreviewItem(const ShapeItem* item) const { return item != nullptr && item == m_previewItem; }

ShapeData PathCreationStrategy::buildPathPreviewData(const QPointF& cursorPoint) const {
    ShapeData data;
    data.id = m_context.generateId();
    data.type = m_type;
    data.style = m_context.currentStyleFor(data.type);
    data.points = m_pathPoints;
    data.points.append(cursorPoint);
    return normalizedShapeData(data);
}
