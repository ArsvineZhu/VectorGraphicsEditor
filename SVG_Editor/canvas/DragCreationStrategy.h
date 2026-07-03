#pragma once

#include <functional>
#include <optional>

#include "canvas/CreationStrategy.h"
#include "core/ShapeData.h"

class QGraphicsScene;

struct CreationContext {
    std::function<qreal()> nextZValue;
    std::function<ShapeStyle(ShapeType)> currentStyleFor;
    std::function<void(const ShapeData&, bool)> addShape;
    std::function<QString()> generateId;
    std::function<std::optional<ShapeType>()> currentShapeType;
    QGraphicsScene* scene = nullptr;
};

class DragCreationStrategy : public ICreationStrategy {
  public:
    explicit DragCreationStrategy(CreationContext context);

    void begin(const QPointF& scenePoint) override;
    void update(const QPointF& scenePoint) override;
    void finish(const QPointF& scenePoint) override;
    void cancel() override;
    bool inProgress() const override;
    bool isPreviewItem(const ShapeItem* item) const override;

  private:
    ShapeData buildDragShapeData(const QPointF& endPoint) const;

    CreationContext m_context;
    QPointF m_dragStartPoint;
    bool m_dragDrawing = false;
    ShapeItem* m_previewItem = nullptr;
};
