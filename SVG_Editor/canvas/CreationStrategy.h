#pragma once

#include <QPointF>

class ShapeItem;

class ICreationStrategy {
  public:
    virtual ~ICreationStrategy() = default;

    virtual void begin(const QPointF& scenePoint) = 0;
    virtual void update(const QPointF& scenePoint) = 0;
    virtual void finish(const QPointF& scenePoint) = 0;
    virtual void cancel() = 0;
    virtual bool inProgress() const = 0;
    virtual bool isPreviewItem(const ShapeItem* item) const = 0;
};
