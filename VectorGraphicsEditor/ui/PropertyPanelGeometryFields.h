// =====================================================================
// PropertyPanelGeometryFields.h
// ---------------------------------------------------------------------
// @brief   属性面板几何字段的静态映射表
// @details 把“某种 ShapeType 展示哪些字段、字段如何写回 ShapeData”收纳为
//          一张只读表，避免 PropertyPanel.cpp 中出现大段重复 switch 分支。
// @layer   ui
// =====================================================================

#pragma once

#include <array>
#include <functional>
#include <unordered_map>
#include <vector>

#include "core/ShapeData.h"

/// @brief 一个几何输入字段的显示信息与取值边界。
struct GeometryField {
    const char* label = "";
    double minimum = -100000.0;
    double maximum = 100000.0;
};

/// @brief 某类图形在属性面板中的完整几何编辑定义。
struct GeometryFieldSet {
    std::vector<GeometryField> fields;
    std::function<void(ShapeData&, const std::array<double, 6>&)> applyTo;
};

/// @brief 查询某种 ShapeType 对应的几何字段配置与写回逻辑。
/// @details 返回的是静态只读表中的引用；PropertyPanel 应只读取，不应缓存后修改。
inline const GeometryFieldSet& geometryFieldsFor(ShapeType type) {
    static const std::unordered_map<ShapeType, GeometryFieldSet> kFields = {
        {ShapeType::Point,
         {{{"x"}, {"y"}},
          [](ShapeData& data, const std::array<double, 6>& values) {
              if (!data.points.isEmpty()) {
                  data.points[0] = QPointF(values[0], values[1]);
              }
          }}},
        {ShapeType::Line,
         {{{"x1"}, {"y1"}, {"x2"}, {"y2"}},
          [](ShapeData& data, const std::array<double, 6>& values) {
              if (data.points.size() >= 2) {
                  data.points[0] = QPointF(values[0], values[1]);
                  data.points[1] = QPointF(values[2], values[3]);
              }
          }}},
        {ShapeType::Polyline,
         {{{"x"}, {"y"}},
          [](ShapeData& data, const std::array<double, 6>& values) {
              // 折线/多边形在当前产品里没有逐点表单；几何面板只提供整体平移，
              // 因而这里以当前包围盒左上角为锚点计算位移量。
              const QRectF currentBounds = pointsBoundingRect(data.points);
              translateShapeData(data, QPointF(values[0] - currentBounds.x(), values[1] - currentBounds.y()));
          }}},
        {ShapeType::Polygon,
         {{{"x"}, {"y"}},
          [](ShapeData& data, const std::array<double, 6>& values) {
              // 与 Polyline 相同：通过包围盒左上角表达整体平移，而不是编辑顶点集。
              const QRectF currentBounds = pointsBoundingRect(data.points);
              translateShapeData(data, QPointF(values[0] - currentBounds.x(), values[1] - currentBounds.y()));
          }}},
        {ShapeType::Circle,
         {{{"cx"}, {"cy"}, {"r", 0.0, 100000.0}},
          [](ShapeData& data, const std::array<double, 6>& values) {
              const qreal radius = values[2];
              const QPointF center(values[0], values[1]);
              data.rect = QRectF(center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0);
          }}},
        {ShapeType::Ellipse,
         {{{"cx"}, {"cy"}, {"rx", 0.0, 100000.0}, {"ry", 0.0, 100000.0}},
          [](ShapeData& data, const std::array<double, 6>& values) {
              const QPointF center(values[0], values[1]);
              data.rect = QRectF(center.x() - values[2], center.y() - values[3], values[2] * 2.0, values[3] * 2.0);
          }}},
        {ShapeType::Rectangle,
         {{{"x"}, {"y"}, {"width", 0.0, 100000.0}, {"height", 0.0, 100000.0}},
          [](ShapeData& data, const std::array<double, 6>& values) {
              data.rect = QRectF(values[0], values[1], values[2], values[3]);
          }}},
    };

    return kFields.at(type);
}
