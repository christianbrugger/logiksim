#include "core/geometry/point.h"

#include "core/geometry/grid.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/point_fine.h"

namespace logicsim {

auto is_representable(point_t point, int dx, int dy) -> bool {
    return is_representable(int {point.x} + dx, int {point.y} + dy);
}

auto add_unchecked(point_t point, int dx, int dy) -> point_t {
    return point_t {
        add_unchecked(point.x, dx),
        add_unchecked(point.y, dy),
    };
}

auto move_or_delete_points(std::span<const point_t> points, int delta_x,
                           int delta_y) -> std::vector<point_t> {
    auto result = std::vector<point_t> {};
    result.reserve(points.size());

    for (const auto& point : points) {
        if (is_representable(point, delta_x, delta_y)) {
            result.push_back(add_unchecked(point, delta_x, delta_y));
        }
    }

    return result;
}

auto to_grid(point_fine_t position) -> std::optional<point_t> {
    const auto x = round(position.x);
    const auto y = round(position.y);

    if (is_representable(x, y)) {
        return point_t {gsl::narrow_cast<grid_t::value_type>(double {x}),
                        gsl::narrow_cast<grid_t::value_type>(double {y})};
    }
    return std::nullopt;
}

}  // namespace logicsim
