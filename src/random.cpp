#include "random.h"

#include "algorithm.h"
#include "geometry.h"
#include "layout.h"

#include <boost/random/seed_seq.hpp>

#include <cmath>
#include <random>

namespace logicsim {

auto get_random_number_generator() -> Rng {
    auto rd = std::random_device {};
    const auto seed_values = std::array {rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
    const auto seed_seq = boost::random::seed_seq(seed_values.begin(), seed_values.end());
    return Rng(seed_seq);
}

auto get_random_number_generator(uint32_t seed) -> Rng {
    return Rng {seed};
}

auto get_random_bool(Rng& rng) -> bool {
    return uint_distribution(0, 1)(rng) > 0;
}

auto get_random_bool(Rng& rng, double percentage) -> bool {
    if (percentage < 0.0 || percentage > 1.0) {
        throw_exception("percentage needs to be between 0 and 1");
    }

    // mantissa size
    constexpr static auto max_value = uint64_t {9007199254740992};  // 2 ** 53

    const auto max_double = gsl::narrow<double>(max_value);
    const auto threshold = gsl::narrow<uint64_t>(round_fast(max_double * percentage));

    return uint_distribution(uint64_t {0}, max_value)(rng) <= threshold;
}

auto get_random_grid(Rng& rng, grid_t::value_type min, grid_t::value_type max) -> grid_t {
    return uint_distribution(min, max)(rng);
}

auto get_random_point(Rng& rng, grid_t::value_type min, grid_t::value_type max)
    -> point_t {
    return point_t {
        get_random_grid(rng, min, max),
        get_random_grid(rng, min, max),
    };
}

auto get_random_point(Rng& rng, rect_t rect) -> point_t {
    return point_t {
        get_random_grid(rng, rect.p0.x.value, rect.p1.x.value),
        get_random_grid(rng, rect.p0.y.value, rect.p1.y.value),
    };
}

auto get_random_line(Rng& rng, grid_t::value_type min, grid_t::value_type max)
    -> ordered_line_t {
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

auto get_random_line(Rng& rng, grid_t::value_type min, grid_t::value_type max,
                     grid_t::value_type max_length) -> ordered_line_t {
    auto p0 = get_random_point(rng, min, max);

    if (max_length <= 0) [[unlikely]] {
        throw_exception("max length needs to be positive");
    }

    const auto N = [](int value) { return gsl::narrow_cast<grid_t::value_type>(value); };

    auto p1 = get_random_point(
        rng,
        rect_t {
            point_t {
                N(std::max(static_cast<int>(min), p0.x.value - (max_length + 1) / 2)),
                N(std::max(static_cast<int>(min), p0.y.value - (max_length + 1) / 2)),
            },
            point_t {
                N(std::min(static_cast<int>(max), p0.x.value + max_length / 2)),
                N(std::min(static_cast<int>(max), p0.y.value + max_length / 2)),
            },
        });

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

auto get_random_lines(Rng& rng, std::size_t count, grid_t::value_type min,
                      grid_t::value_type max) -> std::vector<ordered_line_t> {
    auto result = std::vector<ordered_line_t> {};

    for (const auto _ [[maybe_unused]] : range(count)) {
        result.push_back(get_random_line(rng, min, max));
    }
    return result;
}

auto get_random_part(Rng& rng, part_t full_part) -> part_t {
    auto begin = offset_t::value_type {};
    auto end = offset_t::value_type {};
    const auto part_dist = uint_distribution(full_part.begin.value, full_part.end.value);
    while (begin >= end) {
        begin = part_dist(rng);
        end = part_dist(rng);
    }
    return part_t {offset_t {begin}, offset_t {end}};
}

auto get_random_part(Rng& rng, ordered_line_t line) -> part_t {
    const auto full_part = to_part(line);
    return get_random_part(rng, full_part);
}

auto get_random_insertion_mode(Rng& rng) -> InsertionMode {
    using enum InsertionMode;

    int index = uint_distribution(0, 2)(rng);

    switch (index) {
        case 0:
            return temporary;
        case 1:
            return collisions;
        case 2:
            return insert_or_discard;
    }
    throw_exception("unknown case");
}

//
// Segment Trees
//

auto get_random_segment_tree(Rng& rng, const Layout& layout) -> element_id_t {
    if (!has_segments(layout)) {
        return null_element;
    }
    while (true) {
        const auto element_id = element_id_t {uint_distribution(
            std::size_t {0}, layout.element_count() - std::size_t {1})(rng)};

        if (!layout.segment_tree(element_id).empty()) {
            return element_id;
        }
    }
}

auto get_random_segment(Rng& rng, const SegmentTree& tree) -> segment_index_t {
    if (tree.empty()) {
        return null_segment_index;
    }

    return segment_index_t {
        uint_distribution(tree.first_index().value, tree.last_index().value)(rng)};
}

auto get_random_segment(Rng& rng, const Layout& layout) -> segment_t {
    const auto element_id = get_random_segment_tree(rng, layout);
    if (!element_id) {
        return null_segment;
    }

    const auto segment_index = get_random_segment(rng, layout.segment_tree(element_id));
    if (!segment_index) [[unlikely]] {
        throw_exception("should always return a valid index");
    }

    return segment_t {element_id, segment_index};
}

auto get_random_segment_part(Rng& rng, const Layout& layout) -> segment_part_t {
    auto segment = get_random_segment(rng, layout);
    if (!segment) {
        return null_segment_part;
    }

    auto part = get_random_part(rng, get_line(layout, segment));
    return segment_part_t {segment, part};
}

// randomly select tree

// get random segment
}  // namespace logicsim
