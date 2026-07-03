// =====================================================================
// ShapeItem.h
// ---------------------------------------------------------------------
// @brief   单个图形在 QGraphicsScene 中的渲染节点
// @details `ShapeItem` 是 `ShapeData` 与 Qt Graphics View 框架的桥接类：
//          - 持有 ShapeData 副本（构造时即归一化），所有绘制与命中测试都基于它；
//          - 继承 QGraphicsObject 以支持信号（如 shapeChanged）；
//          - 通过自定义 `Type = UserType + 1` 让 `qgraphicsitem_cast<ShapeItem*>`
//            能从场景 items 中精确识别出本类实例。
// @layer   graphics
// @warning ShapeItem 不再使用 Qt 的 `ItemIsMovable` 临时位移；几何变换统一
//          通过 `ShapeData::transform` 持久化，由 CanvasView 协调拖动 / 缩放。
// =====================================================================

#pragma once

#include <QGraphicsObject>
#include <QPainterPath>
#include <QPen>

#include "core/ShapeData.h"

/// @brief 在 QGraphicsScene 中代表一个 ShapeData 的可渲染、可选中对象。
class ShapeItem : public QGraphicsObject {
    Q_OBJECT

  public:
    /// @brief 自定义 type id，配合 `qgraphicsitem_cast<ShapeItem*>` 使用。
    enum { Type = QGraphicsItem::UserType + 1 };

    /// @brief 构造时即对 data 调用 normalizedShapeData，避免退化几何。
    /// @param data   源 ShapeData
    /// @param parent 父 QGraphicsItem（用于 Qt 对象树）
    explicit ShapeItem(const ShapeData& data, QGraphicsItem* parent = nullptr);

    /// @brief Qt 框架调用，返回 type() 用于 `qgraphicsitem_cast`。
    int type() const override { return Type; }

    /// @brief Qt 绘制/拾取使用；向外扩展 4 像素以容纳描边与选区高亮。
    QRectF boundingRect() const override;

    /// @brief Qt 命中测试用；描边宽度放大到 max(8, stroke+6) 像素以容差。
    QPainterPath shape() const override;

    /// @brief Qt 实际绘制入口；按 type 分支调用 drawXxx。
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    /// @brief 取得当前绑定的 ShapeData（按值返回，独立副本）。
    ShapeData shapeData() const;

    /// @brief 替换内部数据。会自动 prepareGeometryChange 并刷新 z。
    /// @param data 新数据（内部会归一化）
    void setShapeData(const ShapeData& data);

    /// @brief 开启/关闭"预览模式"：用于拖拽创建过程中的实时反馈。
    /// @details 预览模式下：
    ///          - 不可被选中（ItemIsSelectable = false）
    ///          - 不参与选择态交互
    ///          - 描边色变浅 + 改为虚线
    void setPreviewMode(bool enabled);

    /// @brief 控制是否绘制 item 自带的选中虚线框。
    void setSelectionDecorationVisible(bool visible);

    /// @brief 返回未应用 transform 的本地真实几何包围盒，供单选 frame 初始化使用。
    QRectF localGeometryBounds() const;

    /// @brief 返回未应用 transform 的本地交互包围盒，供命中相关逻辑使用。
    QRectF localSelectionBounds() const;

  signals:
    /// @brief 当用户拖动结束后发出（含选中图形的位移提交）。
    /// @param item 发生变化的 ShapeItem 自身
    void shapeChanged(ShapeItem* item);

  private:
    friend class ShapeItemTests;

    /// @brief 根据 m_data 与 m_previewMode 构造用于绘制 / 命中测试的路径。
    QPainterPath buildPath() const;

    /// @brief 构造未应用 transform 的基础几何路径。
    QPainterPath buildBasePath() const;

    /// @brief 基于给定 path 计算命中 / 选中用的交互区域。
    QPainterPath buildInteractionPath(const QPainterPath& path) const;

    /// @brief 根据 m_data.style 构造 QPen；预览模式下颜色变浅、改为虚线。
    QPen buildPen() const;

    /// @brief 绑定的数据（构造时已归一化）
    ShapeData m_data;

    /// @brief 是否处于"拖拽中预览"状态；为 true 时禁用选中 / 移动
    bool m_previewMode = false;

    /// @brief true 时允许 item 自己绘制选中虚线框；Select 工具下由 overlay 接管。
    bool m_selectionDecorationVisible = true;
};
