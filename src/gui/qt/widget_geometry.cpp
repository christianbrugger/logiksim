#include "gui/qt/widget_geometry.h"

#include <gsl/gsl>

#include <QWidget>

namespace logicsim {

auto GeometryInfo::format() const -> std::string {
    const auto& geometry = geometry_top_level_logical;

    return fmt::format("<GeometryInfo: rect = ({}, {}, {}, {}), scale = {}>",
                       geometry.x(), geometry.y(), geometry.x() + geometry.width(),
                       geometry.y() + geometry.height(), device_pixel_ratio);
}

auto get_geometry_top_level_logical(const QWidget& widget) -> QRect {
    const auto geometry = widget.geometry();
    const auto top_left = widget.mapTo(widget.topLevelWidget(), QPoint {0, 0});

    return QRect {top_left.x(), top_left.y(), geometry.width(), geometry.height()};
}

auto get_geometry_info(const QWidget& widget) -> GeometryInfo {
    return GeometryInfo {
        .geometry_top_level_logical = get_geometry_top_level_logical(widget),
        .device_pixel_ratio = widget.devicePixelRatioF(),
    };
}

namespace {

auto round_logical_to_device(QRect rect, double pixel_ratio) -> QRect {
    const auto p0_logic = rect.topLeft();
    // note QPoint::bottomRight() substracts one
    const auto p1_logic = QPoint {rect.x() + rect.width(), rect.y() + rect.height()};

    const auto p0 = (QPointF {p0_logic} * pixel_ratio).toPoint();
    const auto p1 = (QPointF {p1_logic} * pixel_ratio).toPoint();

    return QRect {p0.x(), p0.y(), p1.x() - p0.x(), p1.y() - p0.y()};
}

}  // namespace

auto to_device_rounded(GeometryInfo geometry_info) -> QRect {
    return round_logical_to_device(geometry_info.geometry_top_level_logical,
                                   geometry_info.device_pixel_ratio);
}

auto to_size_device(GeometryInfo geometry_info) -> QSize {
    return to_device_rounded(geometry_info).size();
}

auto get_size_device(const QWidget& widget) -> QSize {
    return to_size_device(get_geometry_info(widget));
}

}  // namespace logicsim
