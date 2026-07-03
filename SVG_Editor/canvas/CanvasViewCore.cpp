// =====================================================================
// CanvasViewCore.cpp
// ---------------------------------------------------------------------
// @brief   CanvasView 的“文档 / 生命周期 / 通用命令”实现
// @details 这里放不直接依赖鼠标事件状态机的逻辑：构造、工具切换、增删、
//          复制粘贴、文档导入导出、样式与 z-value 管理。
// @layer   canvas
// =====================================================================

#include "canvas/CanvasView.h"

#include <QGraphicsScene>
#include <QImage>
#include <QPainter>
#include <QUuid>

#include <algorithm>

#include "core/CanvasViewConstants.h"
#include "canvas/DragCreationStrategy.h"
#include "core/FileManager.h"
#include "core/I18n.h"
#include "canvas/PathCreationStrategy.h"
#include "graphics/ShapeFactory.h"
#include "graphics/ShapeItem.h"

namespace {

/// @brief 把当前工具映射到其对应的 ShapeType；Select 没有可落盘图形类型。
std::optional<ShapeType> toolToShapeType(CanvasView::Tool tool) {
    switch (tool) {
    case CanvasView::Tool::Point:
        return ShapeType::Point;
    case CanvasView::Tool::Line:
        return ShapeType::Line;
    case CanvasView::Tool::Polyline:
        return ShapeType::Polyline;
    case CanvasView::Tool::Circle:
        return ShapeType::Circle;
    case CanvasView::Tool::Ellipse:
        return ShapeType::Ellipse;
    case CanvasView::Tool::Rectangle:
        return ShapeType::Rectangle;
    case CanvasView::Tool::Polygon:
        return ShapeType::Polygon;
    case CanvasView::Tool::Select:
        return std::nullopt;
    }

    return std::nullopt;
}

} // namespace

CanvasView::CanvasView(QWidget* parent) : QGraphicsView(parent), m_scene(new QGraphicsScene(this)) {
    // 默认样式既用于新建图形，也会在单选编辑后回流为“当前样式”。
    m_currentStyle.strokeEnabled = true;
    m_currentStyle.strokeColor = Qt::black;
    m_currentStyle.strokeWidth = 2.0;
    m_currentStyle.strokeStyle = Qt::SolidLine;
    m_currentStyle.fillColor = QColor("#80c8ff");
    m_currentStyle.fillEnabled = true;

    m_scene->setSceneRect(0.0, 0.0, kDefaultCanvasWidth, kDefaultCanvasHeight);
    m_scene->setBackgroundBrush(Qt::white);

    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing, true);
    setDragMode(QGraphicsView::RubberBandDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    connect(m_scene, &QGraphicsScene::selectionChanged, this, &CanvasView::handleSelectionChanged);

    m_selectionOverlay = new SelectionTransformOverlayItem();
    m_scene->addItem(m_selectionOverlay);

    // 三类创建策略共享同一组上下文回调，避免策略层反向依赖 CanvasView 细节。
    CreationContext creationContext;
    creationContext.nextZValue = [this]() { return nextZValue(); };
    creationContext.currentStyleFor = [this](ShapeType type) { return currentStyleFor(type); };
    creationContext.addShape = [this](const ShapeData& data, bool selectNewItem) { addShape(data, selectNewItem); };
    creationContext.generateId = [this]() { return generateShapeId(); };
    creationContext.currentShapeType = [this]() { return toolToShapeType(m_tool); };
    creationContext.scene = m_scene;

    m_dragStrategy = std::make_unique<DragCreationStrategy>(creationContext);
    m_polylineStrategy = std::make_unique<PathCreationStrategy>(ShapeType::Polyline, creationContext);
    m_polygonStrategy = std::make_unique<PathCreationStrategy>(ShapeType::Polygon, creationContext);
}

CanvasView::~CanvasView() = default;

/// @brief 切换当前语言；后续状态栏与错误消息均按该语言生成。
void CanvasView::setLanguage(AppLanguage language) { m_language = language; }

/// @brief 切换当前工具，并清理与旧工具相关的会话状态。
void CanvasView::setTool(Tool tool) {
    if (m_tool == tool) {
        return;
    }

    cancelSelectionDrag();
    cancelTransformSession();
    cancelAllStrategies();

    m_tool = tool;
    setDragMode(tool == Tool::Select ? QGraphicsView::RubberBandDrag : QGraphicsView::NoDrag);
    m_selectionDragCandidate = false;
    invalidateSelectionFrame();
    updateSelectionDecorations();
    updateSelectionOverlay();

    if (tool == Tool::Select) {
        emit statusMessageChanged(i18n::tr(m_language, "toolbar.current_select", "Current tool: Select", "当前工具：选择"));
        return;
    }

    emit statusMessageChanged(i18n::tr(m_language, "toolbar.current_other", "Current tool: %1", "当前工具：%1")
                                  .arg(shapeDisplayName(toolToShapeType(tool).value_or(ShapeType::Rectangle), m_language)));
}

CanvasView::Tool CanvasView::tool() const { return m_tool; }

/// @brief 用外部编辑结果覆盖当前单选图形。
void CanvasView::updateSelectedShape(const ShapeData& data) {
    if (selectedShapeCount() != 1) {
        return;
    }

    ShapeItem* item = selectedShapeItem();
    if (item == nullptr) {
        return;
    }

    item->setShapeData(data);
    m_currentStyle = data.style;
    invalidateSelectionFrame();
    refreshSelectionNotification();
}

/// @brief 删除当前单选图形。
void CanvasView::deleteSelectedItem() {
    ShapeItem* item = selectedShapeItem();
    if (item == nullptr) {
        return;
    }

    cancelSelectionDrag();
    cancelTransformSession();
    m_scene->removeItem(item);
    delete item;
    invalidateSelectionFrame();
    refreshSelectionNotification();
}

/// @brief 删除所有当前选中图形。
void CanvasView::deleteSelectedItems() {
    const QList<ShapeItem*> items = selectedShapeItems();
    if (items.isEmpty()) {
        return;
    }

    cancelSelectionDrag();
    cancelTransformSession();
    for (ShapeItem* item : items) {
        m_scene->removeItem(item);
        delete item;
    }
    invalidateSelectionFrame();
    refreshSelectionNotification();
}

/// @brief 复制当前单选图形快照，并重置粘贴偏移计数。
void CanvasView::copySelectedItem() {
    ShapeItem* item = selectedShapeItem();
    if (item == nullptr) {
        return;
    }

    m_clipboardShape = item->shapeData();
    m_pasteCount = 0;
    emit statusMessageChanged(i18n::tr(m_language, "status.shape_copied", "Shape copied.", "图形已复制。"));
}

/// @brief 粘贴复制快照，并通过递增偏移避免与原图完全重叠。
void CanvasView::pasteCopiedItem() {
    if (!m_clipboardShape.has_value()) {
        return;
    }

    ++m_pasteCount;
    ShapeData copy = ShapeFactory::cloneWithOffset(*m_clipboardShape,
                                                   QPointF(kPasteOffsetPx * m_pasteCount, kPasteOffsetPx * m_pasteCount));
    copy.zValue = nextZValue();
    addShape(copy, true);
    emit statusMessageChanged(i18n::tr(m_language, "status.shape_pasted", "Shape pasted.", "图形已粘贴。"));
}

/// @brief 清空画布并重建 selection overlay。
void CanvasView::clearCanvas() {
    cancelSelectionDrag();
    cancelTransformSession();
    cancelAllStrategies();

    m_selectionOverlay = nullptr;
    m_scene->clear();
    m_zCounter = 0.0;
    m_pasteCount = 0;
    m_selectionFrame.reset();
    m_selectionDragCandidate = false;
    m_selectionDragSession = {};
    m_transformSession = {};

    m_selectionOverlay = new SelectionTransformOverlayItem();
    m_scene->addItem(m_selectionOverlay);
    refreshSelectionNotification();
}

/// @brief 提取当前文档快照，忽略所有创建中的 preview item。
DocumentData CanvasView::documentData() const {
    QList<ShapeData> shapes;
    QList<ShapeItem*> items;

    for (QGraphicsItem* graphicsItem : m_scene->items(Qt::AscendingOrder)) {
        if (auto* shapeItem = qgraphicsitem_cast<ShapeItem*>(graphicsItem)) {
            if (isPreviewItem(shapeItem)) {
                continue;
            }
            items.append(shapeItem);
        }
    }

    // scene 遍历顺序不是稳定的落盘顺序，这里按 zValue 统一重排。
    std::sort(items.begin(), items.end(),
              [](const ShapeItem* lhs, const ShapeItem* rhs) { return lhs->shapeData().zValue < rhs->shapeData().zValue; });

    for (const ShapeItem* item : items) {
        shapes.append(item->shapeData());
    }

    return {m_scene->sceneRect().size(), shapes};
}

/// @brief 用外部文档整体替换当前画布内容。
void CanvasView::loadDocument(const DocumentData& document) {
    clearCanvas();
    m_scene->setSceneRect(QRectF(QPointF(0.0, 0.0), document.canvasSize));
    for (const ShapeData& shape : document.shapes) {
        addShape(shape, false);
        m_zCounter = std::max(m_zCounter, shape.zValue);
    }
    m_pasteCount = 0;
    invalidateSelectionFrame();
    refreshSelectionNotification();
}

/// @brief 把当前画布渲染为位图文件。
bool CanvasView::exportImage(const QString& filePath, QString* errorMessage) {
    QRectF bounds;
    bool first = true;
    for (QGraphicsItem* graphicsItem : m_scene->items(Qt::AscendingOrder)) {
        auto* shapeItem = qgraphicsitem_cast<ShapeItem*>(graphicsItem);
        if (shapeItem == nullptr || isPreviewItem(shapeItem)) {
            continue;
        }

        if (first) {
            bounds = shapeItem->shape().boundingRect();
            first = false;
        } else {
            bounds = bounds.united(shapeItem->shape().boundingRect());
        }
    }

    if (first || bounds.isNull()) {
        bounds = m_scene->sceneRect();
    }

    bounds = bounds.adjusted(-kExportMarginPx, -kExportMarginPx, kExportMarginPx, kExportMarginPx);
    const QSize imageSize = bounds.size().toSize().expandedTo(kExportMinSize);

    QImage image(imageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    // 导出结果不应带交互 overlay；完成渲染后再恢复现场。
    const bool overlayWasVisible = m_selectionOverlay != nullptr && m_selectionOverlay->isVisible();
    if (overlayWasVisible) {
        m_selectionOverlay->hide();
    }

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    m_scene->render(&painter, QRectF(QPointF(0.0, 0.0), imageSize), bounds);
    painter.end();

    if (overlayWasVisible) {
        m_selectionOverlay->show();
    }

    if (!image.save(filePath)) {
        if (errorMessage != nullptr) {
            *errorMessage = i18n::tr(m_language, "error.export_image", "Failed to save image file.", "保存图片文件失败。");
        }
        return false;
    }

    return true;
}

int CanvasView::selectedShapeCount() const { return static_cast<int>(selectedShapeItems().size()); }

/// @brief 往 scene 中加入一个真正落盘的 ShapeItem，并按需选中它。
void CanvasView::addShape(const ShapeData& data, bool selectNewItem) {
    std::unique_ptr<ShapeItem> item = ShapeFactory::createItem(data);
    ShapeItem* rawItem = item.release();
    rawItem->setSelectionDecorationVisible(m_tool != Tool::Select);
    m_scene->addItem(rawItem);
    m_zCounter = std::max(m_zCounter, data.zValue);

    if (selectNewItem) {
        m_scene->clearSelection();
        rawItem->setSelected(true);
    }
}

/// @brief 生成当前工具对应的新图形样式；不支持填充的图形会强制关闭 fill。
ShapeStyle CanvasView::currentStyleFor(ShapeType type) const {
    ShapeStyle style = m_currentStyle;
    if (!shapeSupportsFill(type)) {
        style.fillEnabled = false;
    }
    return style;
}

QString CanvasView::generateShapeId() const { return QUuid::createUuid().toString(QUuid::WithoutBraces); }

/// @brief 分配下一个单调递增的 zValue，保证新增图形默认位于最上层。
qreal CanvasView::nextZValue() {
    m_zCounter += 1.0;
    return m_zCounter;
}
