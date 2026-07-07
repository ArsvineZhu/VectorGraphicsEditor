// =====================================================================
// CreationStrategy.h
// ---------------------------------------------------------------------
// @brief   图形创建策略抽象接口（策略模式）
// @details 定义统一创建策略接口，支持拖拽创建（DragCreationStrategy）
//          和路径创建（PathCreationStrategy）两种实现。CanvasView 通过
//          多态调用 begin/update/finish/cancel 完成不同的创建流程。
// @layer   canvas
// =====================================================================

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
