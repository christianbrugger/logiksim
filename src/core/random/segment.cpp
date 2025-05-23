#include "core/random/segment.h"

#include "core/algorithm/uniform_int_distribution.h"
#include "core/layout.h"
#include "core/random/bool.h"
#include "core/random/grid.h"
#include "core/random/part.h"
#include "core/vocabulary/segment_index.h"
#include "core/vocabulary/segment_part.h"
#include "core/vocabulary/wire_id.h"

#include <stdexcept>

namespace logicsim {

auto get_random_segment_tree(Rng& rng, const Layout& layout) -> wire_id_t {
    if (!has_segments(layout)) {
        return null_wire_id;
    }

    while (true) {
        const auto wire_id = wire_id_t {uint_distribution(
            std::size_t {0}, layout.wires().size() - std::size_t {1})(rng)};

        if (!layout.wires().segment_tree(wire_id).empty()) {
            return wire_id;
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
    const auto wire_id = get_random_segment_tree(rng, layout);
    if (!wire_id) {
        return null_segment;
    }

    const auto segment_index =
        get_random_segment(rng, layout.wires().segment_tree(wire_id));
    if (!segment_index) [[unlikely]] {
        throw std::runtime_error("should always return a valid index");
    }

    return segment_t {wire_id, segment_index};
}

auto get_random_segment_part(Rng& rng, const Layout& layout) -> segment_part_t {
    auto segment = get_random_segment(rng, layout);
    if (!segment) {
        return null_segment_part;
    }

    auto part = get_random_part(rng, get_line(layout, segment));
    return segment_part_t {segment, part};
}

auto add_random_segment(Rng& rng, SegmentTree& tree) -> segment_index_t {
    const auto [type0, type1] = [&]() {
        using enum SegmentPointType;
        if (get_random_bool(rng)) {
            if (get_random_bool(rng)) {
                return std::make_pair(output, shadow_point);
            }
            return std::make_pair(shadow_point, output);
        }
        return std::make_pair(shadow_point, shadow_point);
    }();

    auto p0 = point_t {get_random_grid(rng), get_random_grid(rng)};
    auto p1 = point_t {get_random_grid(rng), get_random_grid(rng)};

    if (get_random_bool(rng)) {
        p0.x = p1.x;
    } else {
        p0.y = p1.y;
    }

    if (p0 == p1) {
        return add_random_segment(rng, tree);
    }

    const auto line = ordered_line_t {line_t {p0, p1}};

    const auto info = segment_info_t {
        .line = line,
        .p0_type = type0,
        .p1_type = type1,
    };

    auto orignal_count = tree.size();
    const auto new_index = tree.add_segment(info);

    // invariant
    if (tree.size() != orignal_count + 1) {
        throw std::runtime_error("assert failed");
    }
    if (tree.info(new_index) != info) {
        throw std::runtime_error("assert failed");
    }

    const auto part = get_random_part(rng, line);

    tree.mark_valid(new_index, part);
    return new_index;
}

}  // namespace logicsim
