#pragma once

#include <QVector>

#include "canvas/CreationStrategy.h"
#include "canvas/DragCreationStrategy.h"

class PathCreationStrategy : public ICreationStrategy {
  public:
    PathCreationStrategy(ShapeType type, CreationContext context);

    void begin(const QPointF& scenePoint) override;
    void update(const QPointF& scenePoint) override;
    void finish(const QPointF& scenePoint) override;
    void cancel() override;
    bool inProgress() const override;
    bool isPreviewItem(const ShapeItem* item) const override;

  private:
    ShapeData buildPathPreviewData(const QPointF& cursorPoint) const;

    ShapeType m_type;
    CreationContext m_context;
    QVector<QPointF> m_pathPoints;
    ShapeItem* m_previewItem = nullptr;
};
