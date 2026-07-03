// =====================================================================
// SelectionFrame.h
// ---------------------------------------------------------------------
// @brief   保留仿射轴信息的选择框几何模型
// @details SelectionFrame 用 1 个原点 + 2 个轴向量表达一个仿射矩形框：
//          - `topLeft` 是局部 (0, 0)
//          - `xAxis`   指向局部 (1, 0)
//          - `yAxis`   指向局部 (0, 1)
//          这样单选已变换图形、多选整体变换后，都能继续沿 frame 的局部轴
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
    bool isOrthogonal(qreal tolerance = 0.0001) const;
    SelectionFrame orthonormalized() const;

    QPointF topRight() const;
    QPointF bottomLeft() const;
    QPointF bottomRight() const;
    QPointF center() const;

    QPolygonF polygon() const;
    QRectF boundingRect() const;

    QTransform localToSceneTransform() const;
    SelectionFrame mapped(const QTransform& transform) const;
    SelectionFrame translated(const QPointF& delta) const;
};
