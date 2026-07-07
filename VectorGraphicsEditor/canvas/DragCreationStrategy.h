// =====================================================================
// DragCreationStrategy.h
// ---------------------------------------------------------------------
// @brief   拖拽创建策略实现（拖拽式图形创建）
// @details 适用于 Line / Rectangle / Circle / Ellipse 四种图形的创建。
//          用户在画布上按下为起点，拖拽预览，松开确认图形。内部维护一个
//          preview ShapeItem 用于实时反馈。
// @layer   canvas
// @warning 本策略与 PathCreationStrategy 共同继承 ICreationStrategy；
//          创建完成后预览图元会被清理，最终图形通过 context.addShape 提交。
// =====================================================================

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
