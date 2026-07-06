// =====================================================================
// CanvasView.h
// ---------------------------------------------------------------------
// @brief   画布视图：承载绘制、选择、变换、文档导入导出等主交互
// @details CanvasView 是项目里最核心的交互协调者。它持有 scene、工具状态、
//          创建策略、选择框 overlay 和多图形会话快照，负责把 UI 输入转成
//          ShapeData 的稳定更新，但不直接承担菜单、属性面板或磁盘 I/O 的 UI。
// @layer   canvas
// @warning 本类跨越创建、选择、缩放、复制粘贴等多条流程；新增状态前应先确认
//          是否属于现有三类会话（创建 / 选择拖动 / 变换）之一，避免状态机分叉。
// =====================================================================

#pragma once

#include <QGraphicsView>

#include <cstdint>
#include <memory>
#include <optional>

#include "core/AppLanguage.h"
#include "canvas/MultiShapeSession.h"
#include "canvas/SelectionTransformOverlayItem.h"
#include "core/ShapeData.h"
#include "graphics/ShapeItem.h"

class ICreationStrategy;
class QGraphicsScene;
struct DocumentData;

/// @brief 应用主画布视图（QGraphicsView）。
class CanvasView : public QGraphicsView {
    Q_OBJECT

  public:
    /// @brief 当前工具类型。值同时驱动工具栏状态与创建策略分派。
    enum class Tool : std::uint8_t {
        Select,
        Point,
        Line,
        Polyline,
        Circle,
        Ellipse,
        Rectangle,
        Polygon,
    };

    explicit CanvasView(QWidget* parent = nullptr);
    ~CanvasView() override;

    void setLanguage(AppLanguage language);
    void setTool(Tool tool);
    Tool tool() const;

    void updateSelectedShape(const ShapeData& data);
    void deleteSelectedItem();
    void deleteSelectedItems();
    void copySelectedItem();
    void pasteCopiedItem();
    void clearCanvas();
    DocumentData documentData() const;
    void loadDocument(const DocumentData& document);
    bool exportImage(const QString& filePath, QString* errorMessage = nullptr);
    int selectedShapeCount() const;

  signals:
    void selectionStateChanged(ShapeItem* primaryItem, int selectedCount);
    void statusMessageChanged(const QString& message);
    void deleteSelectionRequested();

  protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

  private:
    /// @brief 多选平移会话：保留按下位置，避免拖动第一帧出现跳变。
    struct SelectionDragSession : MultiShapeSession {
        QPointF pressScenePoint;
    };

    /// @brief 缩放会话：记录当前命中的锚点与原始快照。
    struct TransformSession : MultiShapeSession {
        SelectionTransformOverlayItem::Handle handle = SelectionTransformOverlayItem::Handle::None;
    };

    ShapeItem* selectedShapeItem() const;
    QList<ShapeItem*> selectedShapeItems() const;
    void handleSelectionChanged();
    void addShape(const ShapeData& data, bool selectNewItem = true);

    ShapeStyle currentStyleFor(ShapeType type) const;
    QString generateShapeId() const;
    ICreationStrategy* activePathStrategy() const;
    void cancelAllStrategies();
    bool isPreviewItem(const ShapeItem* item) const;

    std::optional<SelectionFrame> buildSelectionFrameFromSelection() const;
    void normalizeSelectedShapeTransforms();
    void invalidateSelectionFrame();
    void refreshSelectionNotification();
    void updateSelectionOverlay();
    void updateSelectionDecorations();

    ShapeItem* shapeItemAtViewPosition(const QPoint& viewPosition) const;
    void armSelectionDrag(const QPoint& viewPosition, const QPointF& scenePoint);
    void beginSelectionDrag();
    void updateSelectionDrag(const QPointF& scenePoint);
    void finishSelectionDrag();
    void cancelSelectionDrag();

    void beginTransformSession(SelectionTransformOverlayItem::Handle handle, const QPointF& scenePoint);
    void updateTransformSession(const QPointF& scenePoint, Qt::KeyboardModifiers modifiers);
    void finishTransformSession();
    void cancelTransformSession();

    qreal nextZValue();

    QGraphicsScene* m_scene = nullptr;
    AppLanguage m_language = kDefaultLanguage;
    Tool m_tool = Tool::Select;
    ShapeStyle m_currentStyle;
    std::optional<ShapeData> m_clipboardShape;
    qreal m_zCounter = 0.0;
    int m_pasteCount = 0;
    SelectionTransformOverlayItem* m_selectionOverlay = nullptr;

    std::unique_ptr<ICreationStrategy> m_dragStrategy;
    std::unique_ptr<ICreationStrategy> m_polylineStrategy;
    std::unique_ptr<ICreationStrategy> m_polygonStrategy;

    std::optional<SelectionFrame> m_selectionFrame;
    TransformSession m_transformSession;
    SelectionDragSession m_selectionDragSession;
    bool m_selectionDragCandidate = false;
    QPoint m_selectionMovePressViewPos;
};
