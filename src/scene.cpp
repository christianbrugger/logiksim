#include "scene.h"

#include <gsl/gsl>

#include <cassert>
#include <cfenv>
#include <cmath>

namespace logicsim {

template <typename result_type = double>
auto round(double value) -> result_type {
    // TODO test this
    // std::nearbyint is much faster than std::round, but we need to check rounding mode
    assert(std::fegetround() == FE_TONEAREST);
    return gsl::narrow<result_type>(std::nearbyint(value));
}

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
        round<grid_t::value_type>(fine.x),
        round<grid_t::value_type>(fine.y),
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
        (config.offset.x + position.x) * config.scale,
        (config.offset.y + position.y) * config.scale,
    };
}

auto to_context(point_t position, ViewConfig config) -> BLPoint {
    return to_context(static_cast<point_fine_t>(position), config);
}

auto to_widget(point_fine_t position, ViewConfig config) -> QPoint {
    auto bl_point = to_context(position, config);
    return QPoint {
        round<int>(bl_point.x),
        round<int>(bl_point.y),
    };
}

auto to_widget(point_t position, ViewConfig config) -> QPoint {
    return to_widget(static_cast<point_fine_t>(position), config);
}

}  // namespace logicsim