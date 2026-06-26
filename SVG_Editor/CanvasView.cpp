// =====================================================================
// CanvasView.cpp
// ---------------------------------------------------------------------
// @brief   CanvasView.h 中声明方法的实现
// @details 本文件内容大致分四块：
//          1) 匿名命名空间：i18n 文本挑选、Tool ↔ ShapeType 转换、
//             缩放/旋转几何辅助、变换矩阵构造；
//          2) 公开槽函数：language / tool / 选中图形的 CRUD（copy / paste /
//             delete / 序列化 / 导出）；
//          3) 鼠标 / 键盘事件：把用户输入分发到「拖拽创建」「路径创建」
//             「选择变换」三条工作流；
//          4) 私有助手：addShape、buildXxx、transformSession 三件套。
// @layer   ui
// =====================================================================

#include "CanvasView.h"

#include <QFile>
#include <QApplication>
#include <QGraphicsScene>
#include <QImage>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QTransform>
#include <QtMath>
#include <QUuid>

#include <algorithm>
#include <cmath>

#include "ShapeFactory.h"

namespace {

/// @brief i18n 文本挑选：根据 `m_language` 返回对应文案（不识别 Qt translator 体系）。
QString textForLanguage(AppLanguage language, const QString& english, const QString& chinese) {
    return language == AppLanguage::SimplifiedChinese ? chinese : english;
}

/// @brief 当前工具是否属于「路径工具」（多次点击累加顶点）。
bool isPathTool(CanvasView::Tool tool) {
    return tool == CanvasView::Tool::Polyline || tool == CanvasView::Tool::Polygon;
}

/// @brief 当前工具是否属于「拖拽工具」（按下并拖动，松开落格）。
bool isDragTool(CanvasView::Tool tool) {
    return tool == CanvasView::Tool::Line || tool == CanvasView::Tool::Rectangle || tool == CanvasView::Tool::Circle ||
           tool == CanvasView::Tool::Ellipse;
}

/// @brief 把工具枚举映射到 ShapeType。Select 工具的返回值是「哨兵」，仅占位。
ShapeType toolToShapeType(CanvasView::Tool tool) {
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
        return ShapeType::Rectangle;
    }

    return ShapeType::Rectangle;
}

/// @brief 从拖拽的起、终点构造圆的外接正方形。
/// @details 圆必须以「正方形」bbox 表达：边长 = max(|dx|, |dy|)；
///          起点始终是 bbox 的左上角（用 dx/dy 的符号决定是否需要左 / 上偏移）。
QRectF circleRectFromPoints(const QPointF& start, const QPointF& end) {
    const qreal dx = end.x() - start.x();
    const qreal dy = end.y() - start.y();
    const qreal side = std::max(std::abs(dx), std::abs(dy));
    const qreal x = dx >= 0.0 ? start.x() : start.x() - side;
    const qreal y = dy >= 0.0 ? start.y() : start.y() - side;
    return QRectF(x, y, side, side);
}

/// @brief 多个 ShapeItem 的合并外接矩形。空列表返回空矩形。
QRectF sceneBoundsForItems(const QList<ShapeItem*>& items) {
    QRectF bounds;
    bool first = true;
    for (ShapeItem* item : items) {
        if (item == nullptr) {
            continue;
        }

        // 多选初始 frame 仍从场景空间轴对齐并集框起步；后续旋转/缩放再改由 frame 自己跟踪。
        const QRectF itemBounds = item->sceneTransform().map(item->shape()).boundingRect();

        if (first) {
            bounds = itemBounds;
            first = false;
            continue;
        }

        bounds = bounds.united(itemBounds);
    }

    return bounds.normalized();
}

/// @brief 给定手柄位置，返回 frame 局部坐标中的锚点位置。
QPointF localPointForHandle(SelectionTransformOverlayItem::Handle handle) {
    switch (handle) {
    case SelectionTransformOverlayItem::Handle::TopLeft:
        return QPointF(0.0, 0.0);
    case SelectionTransformOverlayItem::Handle::TopRight:
        return QPointF(1.0, 0.0);
    case SelectionTransformOverlayItem::Handle::BottomLeft:
        return QPointF(0.0, 1.0);
    case SelectionTransformOverlayItem::Handle::BottomRight:
        return QPointF(1.0, 1.0);
    case SelectionTransformOverlayItem::Handle::Rotate:
    case SelectionTransformOverlayItem::Handle::None:
        return QPointF(0.5, 0.5);
    }

    return QPointF(0.5, 0.5);
}

SelectionTransformOverlayItem::Handle oppositeHandle(SelectionTransformOverlayItem::Handle handle) {
    switch (handle) {
    case SelectionTransformOverlayItem::Handle::TopLeft:
        return SelectionTransformOverlayItem::Handle::BottomRight;
    case SelectionTransformOverlayItem::Handle::TopRight:
        return SelectionTransformOverlayItem::Handle::BottomLeft;
    case SelectionTransformOverlayItem::Handle::BottomLeft:
        return SelectionTransformOverlayItem::Handle::TopRight;
    case SelectionTransformOverlayItem::Handle::BottomRight:
        return SelectionTransformOverlayItem::Handle::TopLeft;
    case SelectionTransformOverlayItem::Handle::Rotate:
    case SelectionTransformOverlayItem::Handle::None:
        return SelectionTransformOverlayItem::Handle::None;
    }

    return SelectionTransformOverlayItem::Handle::None;
}

/// @brief 给定中心与点，返回该点相对中心的极角（弧度，范围 (-π, π]）。
qreal angleFromCenter(const QPointF& center, const QPointF& point) {
    return std::atan2(point.y() - center.y(), point.x() - center.x());
}

/// @brief 把角度吸附到 15° 的整数倍。
qreal snapAngleDegrees(qreal degrees) { return std::round(degrees / 15.0) * 15.0; }

/// @brief 构造一个「沿当前 frame 局部轴缩放」的场景空间变换。
/// @details 先把鼠标点逆变换到 frame 的局部单位矩形，再在局部坐标里计算缩放，
///          最后通过 `M * S * M^-1` 映射回场景空间。
QTransform scaleTransformFromHandle(SelectionTransformOverlayItem::Handle handle, const SelectionFrame& frame,
                                    const QPointF& currentPoint, const QPointF& handlePressOffsetLocal,
                                    bool keepAspectRatio) {
    const QTransform localToScene = frame.localToSceneTransform();
    bool invertible = false;
    const QTransform sceneToLocal = localToScene.inverted(&invertible);
    if (!invertible) {
        return QTransform();
    }

    const QPointF handleLocal = localPointForHandle(handle);
    const QPointF pivotLocal = localPointForHandle(oppositeHandle(handle));
    const QPointF currentLocal = sceneToLocal.map(currentPoint) - handlePressOffsetLocal;
    const qreal startDx = handleLocal.x() - pivotLocal.x();
    const qreal startDy = handleLocal.y() - pivotLocal.y();
    qreal scaleX = qFuzzyIsNull(startDx) ? 1.0 : (currentLocal.x() - pivotLocal.x()) / startDx;
    qreal scaleY = qFuzzyIsNull(startDy) ? 1.0 : (currentLocal.y() - pivotLocal.y()) / startDy;

    if (keepAspectRatio) {
        const qreal magnitude = std::max(std::abs(scaleX), std::abs(scaleY));
        scaleX = (scaleX < 0.0 ? -1.0 : 1.0) * magnitude;
        scaleY = (scaleY < 0.0 ? -1.0 : 1.0) * magnitude;
    }

    QTransform localScale;
    localScale.translate(pivotLocal.x(), pivotLocal.y());
    localScale.scale(scaleX, scaleY);
    localScale.translate(-pivotLocal.x(), -pivotLocal.y());
    return localToScene * localScale * sceneToLocal;
}

/// @brief 构造一个「以 center 为旋转中心、累计旋转量为 start→current 极角差」的 QTransform。
/// @param snapToFifteenDegrees  Shift 按下时为 true：把累计角度吸附到 15° 整数倍
QTransform rotationTransformFromPoints(const SelectionFrame& frame, const QPointF& startPoint,
                                       const QPointF& currentPoint, bool snapToFifteenDegrees) {
    const QPointF center = frame.center();
    qreal rotationDegrees =
        qRadiansToDegrees(angleFromCenter(center, currentPoint) - angleFromCenter(center, startPoint));
    if (snapToFifteenDegrees) {
        rotationDegrees = snapAngleDegrees(rotationDegrees);
    }

    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.rotate(rotationDegrees);
    transform.translate(-center.x(), -center.y());
    return transform;
}

} // namespace

CanvasView::CanvasView(QWidget* parent) : QGraphicsView(parent), m_scene(new QGraphicsScene(this)) {
    // 默认样式：黑色实线 2px、淡蓝填充（开启）；切换工具时这些值会被 currentStyleFor 复制
    m_currentStyle.strokeEnabled = true;
    m_currentStyle.strokeColor = Qt::black;
    m_currentStyle.strokeWidth = 2.0;
    m_currentStyle.strokeStyle = Qt::SolidLine;
    m_currentStyle.fillColor = QColor("#80c8ff");
    m_currentStyle.fillEnabled = true;

    // 默认画布 1200×800：与 FileManager 缺省值、MainWindow 默认值保持一致
    m_scene->setSceneRect(0.0, 0.0, 1200.0, 800.0);
    m_scene->setBackgroundBrush(Qt::white);

    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing, true);
    // Select 工具用框选；其他工具关掉 QGraphicsView 自带拖拽，避免和我们的拖拽冲突
    setDragMode(QGraphicsView::RubberBandDrag);
    // 完整重绘：避免 Antialiasing + 高速拖拽时出现残影
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    connect(m_scene, &QGraphicsScene::selectionChanged, this, &CanvasView::handleSelectionChanged);

    // 选择变换覆盖层：永远在场景里活着；空选区时 hide
    m_selectionOverlay = new SelectionTransformOverlayItem();
    m_scene->addItem(m_selectionOverlay);
}

void CanvasView::setLanguage(AppLanguage language) { m_language = language; }

void CanvasView::setTool(Tool tool) {
    if (m_tool == tool) {
        return;
    }

    // 切工具必须把任何进行中的会话 / 预览清掉，否则状态机之间会互相污染
    cancelSelectionDrag();
    cancelTransformSession();
    cancelDrawing();
    m_tool = tool;
    // 工具切换影响覆盖层可见性：非 Select 时不能显示手柄
    setDragMode(tool == Tool::Select ? QGraphicsView::RubberBandDrag : QGraphicsView::NoDrag);
    m_selectionDragCandidate = false;
    invalidateSelectionFrame();
    updateSelectionDecorations();
    updateSelectionOverlay();

    emit statusMessageChanged(tool == Tool::Select
                                  ? textForLanguage(m_language, "Current tool: Select", "当前工具：选择")
                                  : textForLanguage(m_language, "Current tool: %1", "当前工具：%1")
                                        .arg(shapeDisplayName(toolToShapeType(tool), m_language)));
}

CanvasView::Tool CanvasView::tool() const { return m_tool; }

void CanvasView::updateSelectedShape(const ShapeData& data) {
    // PropertyPanel 写入回画布：仅在单选时生效（多选时面板本身不应被启用）
    if (selectedShapeCount() != 1) {
        return;
    }

    ShapeItem* item = selectedShapeItem();
    if (item == nullptr) {
        return;
    }

    item->setShapeData(data);
    // 把用户改后的样式作为「当前样式」保留，下次新画的图形继承
    m_currentStyle = data.style;
    invalidateSelectionFrame();
    refreshSelectionNotification();
}

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

void CanvasView::deleteSelectedItems() {
    const QList<ShapeItem*> items = selectedShapeItems();
    if (items.isEmpty()) {
        return;
    }

    // 多选删除：先取消缩放 / 旋转会话，避免覆盖层与正在被删的 item 互相干扰
    cancelSelectionDrag();
    cancelTransformSession();
    for (ShapeItem* item : items) {
        m_scene->removeItem(item);
        delete item;
    }
    invalidateSelectionFrame();
    refreshSelectionNotification();
}

void CanvasView::copySelectedItem() {
    ShapeItem* item = selectedShapeItem();
    if (item == nullptr) {
        return;
    }

    m_clipboardShape = item->shapeData();
    // 复制时重置粘贴计数：保证粘贴偏移从 16px 起算
    m_pasteCount = 0;
    emit statusMessageChanged(textForLanguage(m_language, "Shape copied.", "图形已复制。"));
}

void CanvasView::pasteCopiedItem() {
    if (!m_clipboardShape.has_value()) {
        return;
    }

    ++m_pasteCount;
    // 16 像素步进：每次粘贴递增偏移，避免完全重叠
    ShapeData copy =
        ShapeFactory::cloneWithOffset(*m_clipboardShape, QPointF(16.0 * m_pasteCount, 16.0 * m_pasteCount));
    // 粘贴的图形始终置顶
    copy.zValue = nextZValue();
    addShape(copy, true);
    emit statusMessageChanged(textForLanguage(m_language, "Shape pasted.", "图形已粘贴。"));
}

void CanvasView::clearCanvas() {
    cancelSelectionDrag();
    cancelTransformSession();
    cancelDrawing();
    // QGraphicsScene::clear 会顺带删除 addItem 的所有 child（包括 m_selectionOverlay！）
    m_scene->clear();
    m_previewItem = nullptr;
    m_zCounter = 0.0;
    m_pasteCount = 0;
    m_selectionFrame.reset();
    m_selectionDragCandidate = false;
    m_selectionDragSession = {};
    // 重建覆盖层（被上面 clear 释放了）
    m_selectionOverlay = new SelectionTransformOverlayItem();
    m_scene->addItem(m_selectionOverlay);
    refreshSelectionNotification();
}

DocumentData CanvasView::documentData() const {
    QList<ShapeData> shapes;
    QList<ShapeItem*> items;

    // 升序遍历：低 z 在前；预览节点不导出
    for (QGraphicsItem* graphicsItem : m_scene->items(Qt::AscendingOrder)) {
        if (auto* shapeItem = qgraphicsitem_cast<ShapeItem*>(graphicsItem)) {
            if (shapeItem == m_previewItem) {
                continue;
            }
            items.append(shapeItem);
        }
    }

    // 按 z 升序稳定排序后输出，确保文件读回后的 z 与显示一致
    std::sort(items.begin(), items.end(), [](const ShapeItem* lhs, const ShapeItem* rhs) {
        return lhs->shapeData().zValue < rhs->shapeData().zValue;
    });

    for (const ShapeItem* item : items) {
        shapes.append(item->shapeData());
    }

    return {m_scene->sceneRect().size(), shapes};
}

void CanvasView::loadDocument(const DocumentData& document) {
    clearCanvas();
    m_scene->setSceneRect(QRectF(QPointF(0.0, 0.0), document.canvasSize));
    for (const ShapeData& shape : document.shapes) {
        addShape(shape, false);
        // 加载时同步把 z 计数器抬到当前最高 z，避免新画的图形 z 倒挂
        m_zCounter = std::max(m_zCounter, shape.zValue);
    }
    invalidateSelectionFrame();
    refreshSelectionNotification();
}

bool CanvasView::exportImage(const QString& filePath, QString* errorMessage) const {
    QRectF bounds;
    bool first = true;
    // 拼出所有图形的合并 bbox（不含预览）
    for (QGraphicsItem* graphicsItem : m_scene->items(Qt::AscendingOrder)) {
        auto* shapeItem = qgraphicsitem_cast<ShapeItem*>(graphicsItem);
        if (shapeItem == nullptr || shapeItem == m_previewItem) {
            continue;
        }

        if (first) {
            bounds = shapeItem->shape().boundingRect();
            first = false;
            continue;
        }

        bounds = bounds.united(shapeItem->shape().boundingRect());
    }

    // 没有任何图形：回退到整张场景画布
    if (first || bounds.isNull()) {
        bounds = m_scene->sceneRect();
    }

    // 20 像素外边距，避免最外层图形被裁掉
    bounds = bounds.adjusted(-20.0, -20.0, 20.0, 20.0);
    // 最小导出尺寸 400×300，保证空画布也输出非空图片
    const QSize imageSize = bounds.size().toSize().expandedTo(QSize(400, 300));

    QImage image(imageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    // 导出时不要把选区手柄画进去：先记录可见性，导出后恢复
    const bool overlayWasVisible = m_selectionOverlay != nullptr && m_selectionOverlay->isVisible();
    if (overlayWasVisible) {
        // const 方法里修改 m_selectionOverlay 用 const_cast 绕过——Qt 的 API 不友好
        const_cast<SelectionTransformOverlayItem*>(m_selectionOverlay)->hide();
    }

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    m_scene->render(&painter, QRectF(QPointF(0.0, 0.0), imageSize), bounds);
    painter.end();

    if (overlayWasVisible) {
        const_cast<SelectionTransformOverlayItem*>(m_selectionOverlay)->show();
    }

    if (!image.save(filePath)) {
        if (errorMessage != nullptr) {
            *errorMessage = textForLanguage(m_language, "Failed to save image file.", "保存图片文件失败。");
        }
        return false;
    }

    return true;
}

int CanvasView::selectedShapeCount() const { return static_cast<int>(selectedShapeItems().size()); }

void CanvasView::mousePressEvent(QMouseEvent* event) {
    const QPointF scenePoint = mapToScene(event->pos());
    m_selectionDragCandidate = false;

    // 优先：Select 工具下点中手柄 → 进入缩放 / 旋转会话
    if (m_tool == Tool::Select && event->button() == Qt::LeftButton && m_selectionOverlay != nullptr &&
        m_selectionOverlay->isVisible()) {
        const SelectionTransformOverlayItem::Handle handle = m_selectionOverlay->handleAt(scenePoint);
        if (handle != SelectionTransformOverlayItem::Handle::None) {
            beginTransformSession(handle, scenePoint);
            return;
        }
    }

    if (event->button() == Qt::LeftButton) {
        // Point 工具：单击直接落点
        if (m_tool == Tool::Point) {
            ShapeData data;
            data.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            data.type = ShapeType::Point;
            data.points = {scenePoint};
            data.style = currentStyleFor(ShapeType::Point);
            data.zValue = nextZValue();
            addShape(data, true);
            return;
        }

        // 拖拽工具：单次按下并拖动
        if (isDragTool(m_tool)) {
            beginDragShape(scenePoint);
            return;
        }

        // 路径工具：每次点击累加一个顶点
        if (isPathTool(m_tool)) {
            beginPathPoint(scenePoint);
            return;
        }
    }

    QGraphicsView::mousePressEvent(event);

    if (m_tool == Tool::Select && event->button() == Qt::LeftButton) {
        if (ShapeItem* item = shapeItemAtViewPosition(event->pos()); item != nullptr && item->isSelected()) {
            armSelectionDrag(event->pos(), scenePoint);
        }
    }
}

void CanvasView::mouseMoveEvent(QMouseEvent* event) {
    const QPointF scenePoint = mapToScene(event->pos());

    // 缩放 / 旋转会话最高优先级
    if (m_transformSession.active) {
        updateTransformSession(scenePoint, event->modifiers());
        return;
    }

    if (m_selectionDragSession.active) {
        updateSelectionDrag(scenePoint);
        return;
    }

    // 拖拽创建中：实时更新预览
    if (m_dragDrawing) {
        updateDragPreview(scenePoint);
        return;
    }

    // 路径创建中：延展最后一个"幽灵"顶点跟随光标
    if (!m_pathPoints.isEmpty()) {
        updatePathPreview(scenePoint);
    }

    QGraphicsView::mouseMoveEvent(event);

    if (m_tool != Tool::Select || !event->buttons().testFlag(Qt::LeftButton)) {
        return;
    }

    if (m_selectionDragCandidate &&
        (event->pos() - m_selectionMovePressViewPos).manhattanLength() >= QApplication::startDragDistance()) {
        beginSelectionDrag();
    }

    if (m_selectionDragSession.active) {
        updateSelectionDrag(scenePoint);
    }
}

void CanvasView::mouseReleaseEvent(QMouseEvent* event) {
    // 缩放 / 旋转会话：左键释放即结束会话，保留当前结果
    if (event->button() == Qt::LeftButton && m_transformSession.active) {
        finishTransformSession();
        return;
    }

    // 拖拽工作流：左键释放落格
    if (event->button() == Qt::LeftButton && m_dragDrawing) {
        finishDragShape(mapToScene(event->pos()));
        return;
    }

    if (event->button() == Qt::LeftButton && m_selectionDragSession.active) {
        finishSelectionDrag();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);

    if (m_tool == Tool::Select && event->button() == Qt::LeftButton) {
        m_selectionDragCandidate = false;
    }
}

void CanvasView::mouseDoubleClickEvent(QMouseEvent* event) {
    // 路径工具中双击结束绘制（与 Enter 等价）；如果双击点与上一个顶点不同则先追加
    if (event->button() == Qt::LeftButton && isPathTool(m_tool) && !m_pathPoints.isEmpty()) {
        const QPointF scenePoint = mapToScene(event->pos());
        if ((m_pathPoints.last() - scenePoint).manhattanLength() > 1.0) {
            m_pathPoints.append(scenePoint);
        }
        finishPathShape(m_tool == Tool::Polygon);
        return;
    }

    QGraphicsView::mouseDoubleClickEvent(event);
}

void CanvasView::keyPressEvent(QKeyEvent* event) {
    // 路径工具中按 Enter / Return 结束绘制
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && isPathTool(m_tool) &&
        !m_pathPoints.isEmpty()) {
        finishPathShape(m_tool == Tool::Polygon);
        return;
    }

    // Esc：优先取消缩放 / 旋转会话；否则取消当前正在绘制的图形
    if (event->key() == Qt::Key_Escape) {
        if (m_selectionDragSession.active) {
            cancelSelectionDrag();
            return;
        }
        if (m_transformSession.active) {
            cancelTransformSession();
            return;
        }
        cancelDrawing();
        return;
    }

    // Delete / Backspace：把删除请求 emit 给 MainWindow，由它决定是否二次确认
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        emit deleteSelectionRequested();
        return;
    }

    if (event->matches(QKeySequence::Copy)) {
        copySelectedItem();
        return;
    }

    if (event->matches(QKeySequence::Paste)) {
        pasteCopiedItem();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

ShapeItem* CanvasView::selectedShapeItem() const {
    const QList<ShapeItem*> items = selectedShapeItems();
    return items.size() == 1 ? items.first() : nullptr;
}

QList<ShapeItem*> CanvasView::selectedShapeItems() const {
    const QList<QGraphicsItem*> items = m_scene->selectedItems();
    QList<ShapeItem*> selectedItems;
    for (QGraphicsItem* item : items) {
        if (auto* shapeItem = qgraphicsitem_cast<ShapeItem*>(item)) {
            selectedItems.append(shapeItem);
        }
    }
    return selectedItems;
}

void CanvasView::handleSelectionChanged() {
    m_selectionDragCandidate = false;
    m_selectionDragSession = {};
    invalidateSelectionFrame();
    refreshSelectionNotification();
}

void CanvasView::addShape(const ShapeData& data, bool selectNewItem) {
    std::unique_ptr<ShapeItem> item = ShapeFactory::createItem(data);
    // 桥接 ShapeItem 自身的 shapeChanged 信号：用户通过 PropertyPanel 改样式时同步到 m_currentStyle
    connect(item.get(), &ShapeItem::shapeChanged, this, [this](ShapeItem* changedItem) {
        m_currentStyle = changedItem->shapeData().style;
        invalidateSelectionFrame();
        refreshSelectionNotification();
    });

    // unique_ptr 释放所有权给 Qt parent 机制（scene 接管）
    ShapeItem* rawItem = item.release();
    rawItem->setSelectionDecorationVisible(m_tool != Tool::Select);
    m_scene->addItem(rawItem);
    // 同步 z 计数器为新图形的 z（≥），确保 nextZValue 不会回退
    m_zCounter = std::max(m_zCounter, data.zValue);

    if (selectNewItem) {
        // 清空旧选中再选新图形：避免同时多个图形高亮
        m_scene->clearSelection();
        rawItem->setSelected(true);
    }
}

void CanvasView::beginDragShape(const QPointF& scenePoint) {
    // 切换到新拖拽时先清掉任何残留状态（其他工作流的 preview / path）
    cancelDrawing();
    m_dragDrawing = true;
    m_dragStartPoint = scenePoint;

    ShapeData preview = buildDragShapeData(scenePoint);
    preview.zValue = nextZValue();
    std::unique_ptr<ShapeItem> item = ShapeFactory::createItem(preview);
    item->setPreviewMode(true);

    m_previewItem = item.release();
    m_scene->addItem(m_previewItem);
}

void CanvasView::updateDragPreview(const QPointF& scenePoint) {
    if (m_previewItem == nullptr) {
        return;
    }

    m_previewItem->setShapeData(buildDragShapeData(scenePoint));
}

void CanvasView::finishDragShape(const QPointF& scenePoint) {
    const ShapeData data = buildDragShapeData(scenePoint);

    m_dragDrawing = false;
    if (m_previewItem != nullptr) {
        m_scene->removeItem(m_previewItem);
        delete m_previewItem;
        m_previewItem = nullptr;
    }

    // 最小尺寸过滤：1.0 像素长度（线段 / 矩形宽高）以下视为误触，丢弃
    bool validShape = false;
    switch (data.type) {
    case ShapeType::Line:
        validShape = data.points.size() >= 2 && QLineF(data.points.at(0), data.points.at(1)).length() > 1.0;
        break;
    case ShapeType::Circle:
    case ShapeType::Ellipse:
    case ShapeType::Rectangle:
        validShape = data.rect.width() > 1.0 && data.rect.height() > 1.0;
        break;
    default:
        break;
    }

    if (validShape) {
        addShape(data, true);
    }
}

void CanvasView::beginPathPoint(const QPointF& scenePoint) {
    // 首次落点：构造预览图形（多边形 / 折线都先建一个空 ShapeItem）
    if (m_pathPoints.isEmpty()) {
        ShapeData preview;
        preview.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        preview.type = toolToShapeType(m_tool);
        preview.style = currentStyleFor(preview.type);
        preview.zValue = nextZValue();

        std::unique_ptr<ShapeItem> item = ShapeFactory::createItem(preview);
        item->setPreviewMode(true);
        m_previewItem = item.release();
        m_scene->addItem(m_previewItem);
    }

    m_pathPoints.append(scenePoint);
    updatePathPreview(scenePoint);
}

void CanvasView::updatePathPreview(const QPointF& scenePoint) {
    if (m_previewItem == nullptr || m_pathPoints.isEmpty()) {
        return;
    }

    m_previewItem->setShapeData(buildPathPreviewData(scenePoint));
}

void CanvasView::finishPathShape(bool closed) {
    if (m_pathPoints.isEmpty()) {
        cancelDrawing();
        return;
    }

    ShapeData data = buildPathPreviewData(m_pathPoints.last());
    data.points = m_pathPoints;
    data.type = closed ? ShapeType::Polygon : ShapeType::Polyline;

    if (m_previewItem != nullptr) {
        m_scene->removeItem(m_previewItem);
        delete m_previewItem;
        m_previewItem = nullptr;
    }

    // 多边形至少 3 顶点、折线至少 2 顶点
    const int minimumPoints = closed ? 3 : 2;
    if (data.points.size() >= minimumPoints) {
        addShape(data, true);
    }

    m_pathPoints.clear();
}

void CanvasView::cancelDrawing() {
    // 完整复位所有"正在绘制"状态：拖拽标志、路径顶点、预览节点
    m_dragDrawing = false;
    m_pathPoints.clear();
    if (m_previewItem != nullptr) {
        m_scene->removeItem(m_previewItem);
        delete m_previewItem;
        m_previewItem = nullptr;
    }
}

ShapeData CanvasView::buildDragShapeData(const QPointF& endPoint) const {
    ShapeData data;
    data.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    data.type = toolToShapeType(m_tool);
    data.style = currentStyleFor(data.type);
    // zValue = 当前计数 + 1：让预览图形始终在最上层
    data.zValue = m_zCounter + 1.0;

    switch (data.type) {
    case ShapeType::Line:
        // 线段用 2 个端点表达
        data.points = {m_dragStartPoint, endPoint};
        break;
    case ShapeType::Rectangle:
    case ShapeType::Ellipse:
        // 矩形 / 椭圆用起点 + 终点构造并归一化（处理反向拖动）
        data.rect = QRectF(m_dragStartPoint, endPoint).normalized();
        break;
    case ShapeType::Circle:
        // 圆需要先算出「正方形外接框」（circleRectFromPoints）
        data.rect = circleRectFromPoints(m_dragStartPoint, endPoint);
        break;
    default:
        break;
    }

    return normalizedShapeData(data);
}

ShapeData CanvasView::buildPathPreviewData(const QPointF& cursorPoint) const {
    ShapeData data;
    data.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    data.type = toolToShapeType(m_tool);
    data.style = currentStyleFor(data.type);
    data.zValue = m_zCounter + 1.0;
    // 路径预览 = 已确认顶点 + 当前光标（最后一个点用光标位置以实时跟随）
    data.points = m_pathPoints;
    data.points.append(cursorPoint);
    return normalizedShapeData(data);
}

ShapeStyle CanvasView::currentStyleFor(ShapeType type) const {
    ShapeStyle style = m_currentStyle;
    // 不支持填充的图形（线段、点、折线）强制关闭 fillEnabled，
    // 避免用户切到带填充的样式后给 Line / Point 误加填充
    if (!shapeSupportsFill(type)) {
        style.fillEnabled = false;
    }
    return style;
}

std::optional<SelectionFrame> CanvasView::buildSelectionFrameFromSelection() const {
    const QList<ShapeItem*> items = selectedShapeItems();
    if (items.isEmpty()) {
        return std::nullopt;
    }

    if (items.size() == 1) {
        const ShapeItem* item = items.first();
        // 单选 frame 贴真实几何，不贴命中扩张区域；否则旋转后再缩放会把误差一并放大。
        SelectionFrame frame = SelectionFrame::fromRect(item->localGeometryBounds());
        const QTransform sceneTransform = item->shapeData().transform;
        return frame.mapped(sceneTransform);
    }

    const QRectF bounds = sceneBoundsForItems(items);
    if (bounds.isNull() || bounds.isEmpty()) {
        return std::nullopt;
    }

    return SelectionFrame::fromRect(bounds);
}

void CanvasView::invalidateSelectionFrame() { m_selectionFrame.reset(); }

void CanvasView::refreshSelectionNotification() {
    const QList<ShapeItem*> items = selectedShapeItems();
    updateSelectionDecorations();
    updateSelectionOverlay();
    // 单选时 primaryItem 非空；多选 / 无选时 primaryItem = nullptr，count 表达具体数量
    emit selectionStateChanged(items.size() == 1 ? items.first() : nullptr, static_cast<int>(items.size()));
}

void CanvasView::updateSelectionOverlay() {
    if (m_selectionOverlay == nullptr) {
        return;
    }

    if (m_tool != Tool::Select) {
        m_selectionOverlay->clearSelectionFrame();
        return;
    }

    // 单选时每次都从图形当前几何重建 frame，避免“旋转后再缩放”时缓存 frame 漂出本体。
    if (selectedShapeCount() == 1 && !m_selectionDragSession.active && !m_transformSession.active) {
        m_selectionFrame = buildSelectionFrameFromSelection();
    }

    if (!m_selectionFrame.has_value()) {
        m_selectionFrame = buildSelectionFrameFromSelection();
    }

    if (!m_selectionFrame.has_value()) {
        m_selectionOverlay->clearSelectionFrame();
        return;
    }

    m_selectionOverlay->setHandlesVisible(!m_selectionDragSession.active && !m_transformSession.active);
    m_selectionOverlay->setSelectionFrame(*m_selectionFrame);
}

void CanvasView::updateSelectionDecorations() {
    const bool showItemSelectionDecoration = m_tool != Tool::Select;
    for (QGraphicsItem* graphicsItem : m_scene->items()) {
        if (auto* shapeItem = qgraphicsitem_cast<ShapeItem*>(graphicsItem)) {
            shapeItem->setSelectionDecorationVisible(showItemSelectionDecoration);
        }
    }
}

ShapeItem* CanvasView::shapeItemAtViewPosition(const QPoint& viewPosition) const {
    if (QGraphicsItem* graphicsItem = itemAt(viewPosition)) {
        return qgraphicsitem_cast<ShapeItem*>(graphicsItem);
    }
    return nullptr;
}

void CanvasView::armSelectionDrag(const QPoint& viewPosition, const QPointF& scenePoint) {
    m_selectionDragCandidate = true;
    m_selectionMovePressViewPos = viewPosition;
    m_selectionDragPressScenePoint = scenePoint;
}

void CanvasView::beginSelectionDrag() {
    if (!m_selectionDragCandidate) {
        return;
    }

    if (!m_selectionFrame.has_value()) {
        m_selectionFrame = buildSelectionFrameFromSelection();
    }
    if (!m_selectionFrame.has_value()) {
        return;
    }

    const QList<ShapeItem*> items = selectedShapeItems();
    if (items.isEmpty()) {
        return;
    }

    m_selectionDragSession = {};
    m_selectionDragSession.active = true;
    m_selectionDragSession.pressScenePoint = m_selectionDragPressScenePoint;
    m_selectionDragSession.originalFrame = *m_selectionFrame;
    for (ShapeItem* item : items) {
        m_selectionDragSession.entries.append({item, item->shapeData()});
    }

    m_selectionDragCandidate = false;
    updateSelectionOverlay();
}

void CanvasView::updateSelectionDrag(const QPointF& scenePoint) {
    if (!m_selectionDragSession.active) {
        return;
    }

    const QPointF delta = scenePoint - m_selectionDragSession.pressScenePoint;
    QTransform translation;
    translation.translate(delta.x(), delta.y());

    for (const TransformSelectionEntry& entry : m_selectionDragSession.entries) {
        if (entry.item == nullptr) {
            continue;
        }

        ShapeData data = entry.originalData;
        applyTransformToShapeData(data, translation);
        entry.item->setShapeData(data);
    }

    // 选择态拖动必须严格跟鼠标；因此 frame 也以同一份 delta 从按下快照平移。
    if (m_selectionDragSession.entries.size() == 1) {
        m_selectionFrame = buildSelectionFrameFromSelection();
    } else {
        m_selectionFrame = m_selectionDragSession.originalFrame.translated(delta);
    }
    updateSelectionOverlay();
}

void CanvasView::finishSelectionDrag() {
    if (!m_selectionDragSession.active) {
        return;
    }

    m_selectionDragCandidate = false;
    m_selectionDragSession = {};
    refreshSelectionNotification();
}

void CanvasView::cancelSelectionDrag() {
    if (!m_selectionDragSession.active) {
        m_selectionDragCandidate = false;
        return;
    }

    for (const TransformSelectionEntry& entry : m_selectionDragSession.entries) {
        if (entry.item != nullptr) {
            entry.item->setShapeData(entry.originalData);
        }
    }

    m_selectionFrame = m_selectionDragSession.originalFrame;
    m_selectionDragCandidate = false;
    m_selectionDragSession = {};
    refreshSelectionNotification();
}

void CanvasView::beginTransformSession(SelectionTransformOverlayItem::Handle handle, const QPointF& scenePoint) {
    const QList<ShapeItem*> items = selectedShapeItems();
    if (items.isEmpty()) {
        return;
    }

    if (!m_selectionFrame.has_value()) {
        m_selectionFrame = buildSelectionFrameFromSelection();
    }
    if (!m_selectionFrame.has_value()) {
        return;
    }

    m_transformSession = {};
    m_transformSession.active = true;
    m_transformSession.handle = handle;
    // 变换期间所有预览都从同一份初始 frame 快照推导，避免逐帧累积误差。
    m_transformSession.originalFrame = *m_selectionFrame;
    m_transformSession.startPoint = scenePoint;
    m_transformSession.handlePressOffsetLocal = QPointF();

    if (handle != SelectionTransformOverlayItem::Handle::Rotate) {
        bool invertible = false;
        const QTransform sceneToLocal = m_transformSession.originalFrame.localToSceneTransform().inverted(&invertible);
        if (invertible) {
            m_transformSession.handlePressOffsetLocal = sceneToLocal.map(scenePoint) - localPointForHandle(handle);
        }
    }

    // 为每个被变换的图形拍快照，cancel 时用于回滚
    for (ShapeItem* item : items) {
        m_transformSession.entries.append({item, item->shapeData()});
    }
    updateSelectionOverlay();
}

void CanvasView::updateTransformSession(const QPointF& scenePoint, Qt::KeyboardModifiers modifiers) {
    if (!m_transformSession.active) {
        return;
    }

    // Shift 按住：缩放等比 / 旋转 15° 吸附
    const bool shiftPressed = modifiers.testFlag(Qt::ShiftModifier);
    const bool rotate = m_transformSession.handle == SelectionTransformOverlayItem::Handle::Rotate;
    const SelectionFrame& originalFrame = m_transformSession.originalFrame;
    const QPointF& startPoint = m_transformSession.startPoint;
    QTransform deltaTransform;
    if (rotate) {
        deltaTransform = rotationTransformFromPoints(originalFrame, startPoint, scenePoint, shiftPressed);
    } else {
        deltaTransform = scaleTransformFromHandle(m_transformSession.handle, originalFrame, scenePoint,
                                                  m_transformSession.handlePressOffsetLocal, shiftPressed);
    }

    // 增量变换应用：每帧都用「originalData × deltaTransform」覆盖到 item
    // （不是累加，避免鼠标抖动时漂移）
    for (const TransformSelectionEntry& entry : m_transformSession.entries) {
        if (entry.item == nullptr) {
            continue;
        }

        ShapeData data = entry.originalData;
        applyTransformToShapeData(data, deltaTransform);
        entry.item->setShapeData(data);
    }

    if (m_transformSession.entries.size() == 1) {
        m_selectionFrame = buildSelectionFrameFromSelection();
    } else {
        m_selectionFrame = m_transformSession.originalFrame.mapped(deltaTransform);
    }
    updateSelectionOverlay();
}

void CanvasView::finishTransformSession() {
    if (!m_transformSession.active) {
        return;
    }

    m_transformSession = {};
    if (selectedShapeCount() == 1) {
        invalidateSelectionFrame();
    }
    refreshSelectionNotification();
}

void CanvasView::cancelTransformSession() {
    if (!m_transformSession.active) {
        return;
    }

    // 回滚每个被变换的图形到会话开始时的快照
    for (const TransformSelectionEntry& entry : m_transformSession.entries) {
        if (entry.item != nullptr) {
            entry.item->setShapeData(entry.originalData);
        }
    }

    m_selectionFrame = m_transformSession.originalFrame;
    m_transformSession = {};
    refreshSelectionNotification();
}

qreal CanvasView::nextZValue() {
    m_zCounter += 1.0;
    return m_zCounter;
}
