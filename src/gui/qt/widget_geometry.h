#ifndef LOGICSIM_QT_WIDGET_GEOMETRY_H
#define LOGICSIM_QT_WIDGET_GEOMETRY_H

#include "core/format/struct.h"

#include <QPoint>
#include <QRect>
#include <QSize>

#include <optional>

class QWidget;

namespace logicsim {

struct GeometryInfo {
    /**
     * @brief: Geometry of the widget relative to the top level widget in
     *         device independent / logical coordinates.
     */
    QRect geometry_top_level_logical;

    /**
     * @brief: Device pixels of one logical coordinate.
     */
    double device_pixel_ratio;

    [[nodiscard]] auto format() const -> std::string;
};

auto get_geometry_info(const QWidget& widget) -> GeometryInfo;
/**
 * @brief: Geometry of the widget relative to the top level widget in
 *         device independent / logical coordinates.
 */
auto get_geometry_top_level_logical(const QWidget& widget) -> QRect;

auto to_device_rounded(GeometryInfo geometry_info) -> QRect;
auto to_size_device(GeometryInfo geometry_info) -> QSize;

auto get_size_device(const QWidget& widget) -> QSize;

}  // namespace logicsim

#endif
