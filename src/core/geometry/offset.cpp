#include "geometry/offset.h"

#include "geometry/orientation.h"
#include "vocabulary/grid.h"
#include "vocabulary/offset.h"
#include "vocabulary/ordered_line.h"
#include "vocabulary/point.h"

namespace logicsim {

auto to_offset(grid_t x, grid_t reference) -> offset_t {
    const auto value = x.value - reference.value;

    static_assert(sizeof(value) > sizeof(grid_t::value_type));
    static_assert(std::is_signed_v<decltype(value)>);

    return offset_t {gsl::narrow_cast<offset_t::value_type>(value)};
}

auto to_grid(offset_t offset, grid_t reference) -> grid_t {
    const auto value = reference.value + offset.value;

    static_assert(sizeof(value) > sizeof(grid_t::value_type));
    static_assert(std::is_signed_v<decltype(value)>);

    return grid_t {gsl::narrow<grid_t::value_type>(value)};
}

auto to_offset(ordered_line_t line) -> offset_t {
    if (is_horizontal(line)) {
        return to_offset(line.p1.x, line.p0.x);
    }
    return to_offset(line.p1.y, line.p0.y);
}

auto to_offset(ordered_line_t full_line, point_t point) -> offset_t {
    const auto line = ordered_line_t {full_line.p0, point};

    if (is_horizontal(full_line) != is_horizontal(line)) {
        throw std::runtime_error("point is not on full_line");
    }
    if (line.p1 > full_line.p1) [[unlikely]] {
        throw std::runtime_error("point is not part of full_line");
    }

    return to_offset(line);
}

auto to_point(ordered_line_t full_line, offset_t offset) -> point_t {
    if (is_horizontal(full_line)) {
        const auto x = to_grid(offset, full_line.p0.x);
        if (x > full_line.p1.x) [[unlikely]] {
            throw std::runtime_error("offset is not within line");
        }
        return point_t {x, full_line.p0.y};
    }

    const auto y = to_grid(offset, full_line.p0.y);
    if (y > full_line.p1.y) [[unlikely]] {
        throw std::runtime_error("offset is not within line");
    }
    return point_t {full_line.p0.x, y};
}

}  // namespace logicsim
