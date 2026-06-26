// =====================================================================
// SelectionFrame.h
// ---------------------------------------------------------------------
// @brief   可旋转的选择框几何模型
// @details SelectionFrame 用 1 个原点 + 2 个轴向量表达一个仿射矩形框：
//          - `topLeft` 是局部 (0, 0)
//          - `xAxis`   指向局部 (1, 0)
//          - `yAxis`   指向局部 (0, 1)
//          这样单选旋转图形、多选整体旋转后，都能继续沿 frame 的局部轴
//          做缩放和手柄布局，而不会退化回轴对齐正矩形。
// @layer   core
// =====================================================================

#pragma once

#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QTransform>

struct SelectionFrame {
    QPointF topLeft;
    QPointF xAxis;
    QPointF yAxis;

    static SelectionFrame fromRect(const QRectF& rect);

    bool isValid() const;

    QPointF topRight() const;
    QPointF bottomLeft() const;
    QPointF bottomRight() const;
    QPointF center() const;
    QPointF topCenter() const;
    QPointF topNormal() const;

    QPolygonF polygon() const;
    QRectF boundingRect() const;

    QTransform localToSceneTransform() const;
    SelectionFrame mapped(const QTransform& transform) const;
    SelectionFrame translated(const QPointF& delta) const;
};
