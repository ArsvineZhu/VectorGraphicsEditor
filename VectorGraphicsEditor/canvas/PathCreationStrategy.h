// =====================================================================
// PathCreationStrategy.h
// ---------------------------------------------------------------------
// @brief   路径创建策略实现（多点式图形创建）
// @details 适用于 Polyline / Polygon 两种图形的创建。用户通过连续单击
//          累加顶点，支持 Enter / 双击结束、Esc 取消。内部维护顶点列表
//          和预览 ShapeItem。
// @layer   canvas
// =====================================================================

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
