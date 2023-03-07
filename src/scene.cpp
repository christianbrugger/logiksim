#include "scene.h"

#include <gsl/gsl>

#include <cassert>
#include <cmath>

namespace logicsim {

auto is_representable(int x, int y) -> bool {
    return (grid_t::min() <= x && x <= grid_t::max())
           && (grid_t::min() <= y && y <= grid_t::max());
}

auto is_representable(double x, double y) -> bool {
    return (grid_t::min() <= x && x <= grid_t::max())
           && (grid_t::min() <= y && y <= grid_t::max());
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

auto to_grid(double x_, double y_, ViewConfig config) -> std::optional<point_t> {
    const auto fine = to_grid_fine(x_, y_, config);

    const auto x = round_fast(fine.x);
    const auto y = round_fast(fine.y);

    if (is_representable(x, y)) {
        return point_t {gsl::narrow_cast<grid_t::value_type>(x),
                        gsl::narrow_cast<grid_t::value_type>(y)};
    }
    return std::nullopt;
}

auto to_grid(QPointF position, ViewConfig config) -> std::optional<point_t> {
    return to_grid(position.x(), position.y(), config);
}

auto to_grid(QPoint position, ViewConfig config) -> std::optional<point_t> {
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