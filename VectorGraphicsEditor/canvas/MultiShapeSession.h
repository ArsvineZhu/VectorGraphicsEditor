// =====================================================================
// MultiShapeSession.h
// ---------------------------------------------------------------------
// @brief   多选图形快照会话（支持变换撤销）
// @details MultiShapeSession 在变换/平移操作开始前保存所有选中图形的
//          ShapeData 快照和原始选择框 frame，提供 cancel() 用于在
//          Esc 或工具切换时一键恢复初始状态。
// @layer   canvas
// =====================================================================

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
