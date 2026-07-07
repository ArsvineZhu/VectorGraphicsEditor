// =====================================================================
// MultiShapeSession.cpp
// ---------------------------------------------------------------------
// @brief MultiShapeSession 的实现
// @details begin() 时拍摄所有选中图形的完整数据快照；
//          cancel() 时遍历 entries 将每个图形的数据恢复为 snapshot。
// @layer   canvas
// =====================================================================

#include "canvas/MultiShapeSession.h"

#include "graphics/ShapeItem.h"

void MultiShapeSession::clear() {
    active = false;
    entries.clear();
    originalFrame = SelectionFrame{};
}

void MultiShapeSession::begin(const QList<ShapeItem*>& items, const SelectionFrame& frame) {
    clear();
    active = true;
    originalFrame = frame;
    entries.reserve(items.size());
    for (ShapeItem* item : items) {
        if (item != nullptr) {
            entries.append({item, item->shapeData()});
        }
    }
}

void MultiShapeSession::cancel() const {
    for (const MultiShapeEntry& entry : entries) {
        if (entry.item != nullptr) {
            entry.item->setShapeData(entry.snapshot);
        }
    }
}
