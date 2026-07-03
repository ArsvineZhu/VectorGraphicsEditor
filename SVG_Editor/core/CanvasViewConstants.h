// =====================================================================
// CanvasViewConstants.h
// ---------------------------------------------------------------------
// @brief   画布与交互常量集中定义
// @details 统一收纳命中判定、导出边距、默认画布尺寸、选择装饰留白等共享常量，
//          避免这些“跨模块魔法数字”散落在 canvas / graphics / core 代码里。
// @layer   core
// =====================================================================

#pragma once

#include <QSize>
#include <QtGlobal>

/// @brief 选中后拖动进入平移会话所需的最小场景距离。
inline constexpr qreal kSelectionDragMinDistance = 1.0;
/// @brief 连续粘贴的默认偏移距离。
inline constexpr qreal kPasteOffsetPx = 16.0;
/// @brief 导出图片时在内容外围补的白边。
inline constexpr qreal kExportMarginPx = 20.0;
/// @brief 导出图片的最小尺寸，避免极小内容导出成不可读位图。
inline constexpr QSize kExportMinSize{400, 300};
/// @brief 新文档默认画布宽度。
inline constexpr qreal kDefaultCanvasWidth = 1200.0;
/// @brief 新文档默认画布高度。
inline constexpr qreal kDefaultCanvasHeight = 800.0;
/// @brief 交互命中区域的最小 padding。
inline constexpr qreal kHitPaddingMinPx = 8.0;
/// @brief 交互命中区域在描边基础上的额外 padding。
inline constexpr qreal kHitPaddingExtraPx = 6.0;
/// @brief boundingRect 在真实几何外的最小留白。
inline constexpr qreal kBoundingRectPaddingPx = 4.0;
/// @brief 图形自带选中虚框的额外留白。
inline constexpr qreal kSelectionDecorationPaddingPx = 6.0;
