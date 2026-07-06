#pragma once

#include <QList>

#include "core/SelectionFrame.h"
#include "core/ShapeData.h"

class ShapeItem;

struct MultiShapeEntry {
    ShapeItem* item = nullptr;
    ShapeData snapshot;
};

class MultiShapeSession {
  public:
    bool active = false;
    QList<MultiShapeEntry> entries;
    SelectionFrame originalFrame;

    void clear();
    void begin(const QList<ShapeItem*>& items, const SelectionFrame& frame);
    void cancel() const;
};
