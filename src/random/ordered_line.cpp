#include "random/ordered_line.h"

#include "algorithm/range.h"
#include "random/bool.h"
#include "random/point.h"
#include "vocabulary/grid.h"
#include "vocabulary/line.h"
#include "vocabulary/ordered_line.h"
#include "vocabulary/rect.h"

#include <stdexcept>

namespace logicsim {

auto get_random_line(Rng& rng, grid_t min, grid_t max) -> ordered_line_t {
    auto p0 = get_random_point(rng, min, max);
    auto p1 = get_random_point(rng, min, max);

    if (get_random_bool(rng)) {
        p0.x = p1.x;
    } else {
        p0.y = p1.y;
    }

    if (p0 == p1) {
        return get_random_line(rng, min, max);
    }

    return ordered_line_t {line_t {p0, p1}};
}

auto get_random_line(Rng& rng, grid_t min, grid_t max, grid_t max_length)
    -> ordered_line_t {
    auto p0 = get_random_point(rng, min, max);

    if (max_length <= grid_t {0}) [[unlikely]] {
        throw std::runtime_error("max length needs to be positive");
    }

    const auto rect = rect_t {
        point_t {
            std::max(min, p0.x - (max_length + grid_t {1}) / 2),
            std::max(min, p0.y - (max_length + grid_t {1}) / 2),
        },
        point_t {
            std::min(max, p0.x + max_length / 2),
            std::min(max, p0.y + max_length / 2),
        },
    };

    auto p1 = get_random_point(rng, rect);

    if (get_random_bool(rng)) {
        p0.x = p1.x;
    } else {
        p0.y = p1.y;
    }

    if (p0 == p1) {
        return get_random_line(rng, min, max, max_length);
    }

    return ordered_line_t {line_t {p0, p1}};
}

auto get_random_lines(Rng& rng, std::size_t count, grid_t min, grid_t max)
    -> std::vector<ordered_line_t> {
    auto result = std::vector<ordered_line_t> {};

    for (const auto _ [[maybe_unused]] : range(count)) {
        result.push_back(get_random_line(rng, min, max));
    }
    return result;
}

}  // namespace logicsim
