#ifndef LOGICSIM_QT_WIDGET_GEOMETRY_H
#define LOGICSIM_QT_WIDGET_GEOMETRY_H

#include <QPoint>
#include <QRect>
#include <QSize>

#include <optional>

class QWidget;

namespace logicsim {

struct GeometryInfo {
    /**
     * @brief: Geometry of the widget relative to the top level widget in logical
     *         coordinates.
     */
    QRect geometry_top_level_logical;

    /**
     * @brief: Device pixels of one logical coordinate.
     */
    double device_pixel_ratio;
};

auto get_geometry_top_level_logical(const QWidget& widget) -> QRect;
auto get_geometry_info(const QWidget& widget) -> GeometryInfo;

auto to_device(GeometryInfo geometry_info) -> QRect;
auto to_device(GeometryInfo geometry_info, QRect clip) -> QRect;
auto get_size_device(GeometryInfo geometry_info) -> QSize;

}  // namespace logicsim

#endif
