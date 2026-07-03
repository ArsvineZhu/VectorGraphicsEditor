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
