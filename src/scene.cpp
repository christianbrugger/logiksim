#include "scene.h"

#include <gsl/gsl>

#include <cassert>
#include <cmath>

namespace logicsim {

auto to_grid_fine(double x, double y, ViewConfig config) -> point_fine_t {
    return {
        x / config.scale - config.offset.x,
        y / config.scale - config.offset.y,
    };
}

auto to_grid_fine(QPointF position, ViewConfig config) -> point_fine_t {
    return to_grid_fine(position.x(), position.y(), config);
}

auto to_grid_fine(QPoint position, ViewConfig config) -> point_fine_t {
    return to_grid_fine(position.x(), position.y(), config);
}

auto to_grid(double x, double y, ViewConfig config) -> point_t {
    auto fine = to_grid_fine(x, y, config);
    return {
        round_to<grid_t::value_type>(fine.x),
        round_to<grid_t::value_type>(fine.y),
    };
}

auto to_grid(QPointF position, ViewConfig config) -> point_t {
    return to_grid(position.x(), position.y(), config);
}

auto to_grid(QPoint position, ViewConfig config) -> point_t {
    return to_grid(position.x(), position.y(), config);
}

auto to_context(point_fine_t position, ViewConfig config) -> BLPoint {
    return BLPoint {
        round_fast((config.offset.x + position.x) * config.scale),
        round_fast((config.offset.y + position.y) * config.scale),
    };
}

auto to_context(point_t position, ViewConfig config) -> BLPoint {
    return to_context(static_cast<point_fine_t>(position), config);
}

auto to_widget(point_fine_t position, ViewConfig config) -> QPoint {
    auto bl_point = to_context(position, config);
    return QPoint {
        gsl::narrow<int>(bl_point.x),
        gsl::narrow<int>(bl_point.y),
    };
}

auto to_widget(point_t position, ViewConfig config) -> QPoint {
    return to_widget(static_cast<point_fine_t>(position), config);
}

auto to_context(double length, ViewConfig config) -> double {
    return round_fast(length * config.scale);
}

auto to_context(grid_t length, ViewConfig config) -> double {
    return to_context(length.value, config);
}

}  // namespace logicsim