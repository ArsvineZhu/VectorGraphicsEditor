// =====================================================================
// SelectionTransformOverlayItem.h
// ---------------------------------------------------------------------
// @brief   选中图形的「缩放」操控手柄（QGraphicsObject 子类）
// @details 当画布上有图形被选中时，CanvasView 会在场景中放置一个
//          SelectionTransformOverlayItem 实例并实时跟随选中项的 bbox。
//          它本身 **不** 修改任何 ShapeData：
//            - paint()      绘制蓝色虚线选框 + 4 个角点方形手柄
//            - shape()      用作 hit-test，仅 4 个手柄区域可被命中
//            - handleAt()   告诉调用方鼠标当前落在哪个手柄上
//          上层（CanvasView）拿到 handle 后再驱动真正的 transform 逻辑。
// @layer   ui
// @warning 该 item 显式 `setZValue(1e6)`，确保始终盖在所有 ShapeItem 之上；
//          接受鼠标事件被设为 `Qt::NoButton`，所以事件不会被它自身消费，
//          CanvasView 仍能在最外层 QGraphicsView 截获所有鼠标事件。
// =====================================================================

#pragma once

#include <QGraphicsObject>
#include <QRectF>

#include <cstdint>

#include "core/CanvasGeometry.h"
#include "core/SelectionFrame.h"

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

/// @brief 选中图形的可交互覆盖层（操控手柄）。
/// @details
/// - 形状：选区 4 角各一个 10×10 方形手柄。
/// - 视觉：1px 蓝色虚线选框（`#2d7ff9`），手柄白底蓝框。
/// - 命中区：仅 4 个手柄矩形；选框线条本身不可点。
class SelectionTransformOverlayItem : public QGraphicsObject {
    Q_OBJECT

  public:
    /// @brief 手柄位置枚举（与 CanvasHandle 一一对应；ui 层用此名以与图形化语义对齐）
    using Handle = CanvasHandle;

    /// @brief 构造。手柄默认隐藏，z 值固定为 1e6。
    explicit SelectionTransformOverlayItem(QGraphicsItem* parent = nullptr);

    /// @brief Qt Graphics View 框架要求：返回 item 的外接矩形。
    QRectF boundingRect() const override;

    /// @brief Qt Graphics View 框架要求：返回命中测试路径（4 个手柄矩形）。
    QPainterPath shape() const override;

    /// @brief 绘制选框 + 4 个手柄。
    /// @param painter  Qt 传入的画笔
    /// @param option   Qt 传入的样式选项（未使用）
    /// @param widget   Qt 传入的目标 widget（未使用）
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    /// @brief 设置当前选区 frame。会先 `prepareGeometryChange()`，再根据有效性决定显示 / 隐藏。
    void setSelectionFrame(const SelectionFrame& frame);

    /// @brief 清除选区：复位 frame 并隐藏 item。
    void clearSelectionFrame();

    /// @brief 当前选区 frame。
    SelectionFrame selectionFrame() const;

    /// @brief 控制是否显示缩放锚点。
    void setHandlesVisible(bool visible);

    /// @brief 当前是否显示缩放锚点。
    bool handlesVisible() const;

    /// @brief 命中测试：返回给定场景坐标点上的手柄。
    /// @param scenePoint  场景坐标
    /// @return            命中的手柄；若未命中任何手柄或选区为空，返回 `Handle::None`
    Handle handleAt(const QPointF& scenePoint) const;

  private:
    /// @brief 给定手柄位置，返回其在场景坐标系中的矩形（用作绘制与命中测试）。
    QRectF handleRect(Handle handle) const;

    /// @brief 选区是否有效（两条轴长度都非零）。
    bool hasFrame() const;

    /// @brief 当前选区 frame（场景坐标系）。
    SelectionFrame m_frame;

    /// @brief 当前是否持有可绘制 frame。
    bool m_hasFrame = false;

    /// @brief true 时绘制四角锚点；false 时仅绘制虚线矩形。
    bool m_handlesVisible = true;
};
