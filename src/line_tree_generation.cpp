#include "line_tree_generation.h"

#include "algorithm/transform_if.h"
#include "algorithm/transform_to_vector.h"
#include "line_tree.h"
#include "segment_tree.h"

#include <stdexcept>

namespace logicsim {

namespace {

auto generate_line_tree_impl(const SegmentTree& segment_tree) -> LineTree {
    if (segment_tree.empty()) {
        return LineTree {};
    }

    const auto root = [&]() {
        if (segment_tree.has_input()) {
            return segment_tree.input_position();
        }
        for (const segment_info_t& info : segment_tree) {
            if (info.p0_type == SegmentPointType::output) {
                return info.line.p0;
            }
            if (info.p1_type == SegmentPointType::output) {
                return info.line.p1;
            }
        }
        throw std::runtime_error("line tree needs to have either an input or output");
    }();

    const auto segments = transform_to_vector(all_lines(segment_tree));

    return to_line_tree(segments, root);
}

}  // namespace

auto generate_line_tree(const SegmentTree& segment_tree) -> LineTree {
    const auto line_tree = generate_line_tree_impl(segment_tree);
    assert(is_equivalent(segment_tree, line_tree));
    return line_tree;
}

auto has_same_segments(const SegmentTree& segment_tree, const LineTree& line_tree)
    -> bool {
    // line tree
    if (line_tree.size() != segment_tree.size()) [[unlikely]] {
        return false;
    }

    auto segments_1 = transform_to_vector(
        line_tree.lines(), [](const line_t& line) { return ordered_line_t {line}; });
    auto segments_2 = transform_to_vector(all_lines(segment_tree));

    std::ranges::sort(segments_1);
    std::ranges::sort(segments_2);
    return segments_1 == segments_2;
}

auto has_same_cross_points(const SegmentTree& segment_tree, const LineTree& line_tree)
    -> bool {
    // line tree
    auto cross_points_1 = std::vector<point_t> {};
    transform_if(
        indices(line_tree), std::back_inserter(cross_points_1),
        [&](line_index_t index) -> point_t { return line_tree.line(index).p0; },
        [&](line_index_t index) -> bool { return line_tree.has_cross_point_p0(index); });

    std::ranges::sort(cross_points_1);
    const auto duplicates = std::ranges::unique(cross_points_1);
    cross_points_1.erase(duplicates.begin(), duplicates.end());

    // segment tree
    auto cross_points_2 = std::vector<point_t> {};
    transform_if(
        segment_tree, std::back_inserter(cross_points_2),
        [](segment_info_t info) { return info.line.p0; },
        [](segment_info_t info) { return is_cross_point(info.p0_type); });
    transform_if(
        segment_tree, std::back_inserter(cross_points_2),
        [](segment_info_t info) { return info.line.p1; },
        [](segment_info_t info) { return is_cross_point(info.p1_type); });
    std::ranges::sort(cross_points_2);

    return cross_points_1 == cross_points_2;
}

auto has_same_input_position(const SegmentTree& segment_tree, const LineTree& line_tree)
    -> bool {
    return !segment_tree.has_input() ||
           segment_tree.input_position() == line_tree.input_position();
}

auto has_same_output_positions(const SegmentTree& segment_tree, const LineTree& line_tree)
    -> bool {
    // line tree
    auto positions_1 = transform_to_vector(
        output_ids(line_tree),
        [&](connection_id_t output) { return line_tree.output_position(output); });

    // we take an output at random as input to generate the line tree
    if (!segment_tree.has_input()) {
        positions_1.push_back(line_tree.input_position());
    }

    // segment tree
    auto positions_2 = std::vector<point_t> {};
    transform_if(
        segment_tree, std::back_inserter(positions_2),
        [](segment_info_t info) { return info.line.p0; },
        [](segment_info_t info) { return info.p0_type == SegmentPointType::output; });
    transform_if(
        segment_tree, std::back_inserter(positions_2),
        [](segment_info_t info) { return info.line.p1; },
        [](segment_info_t info) { return info.p1_type == SegmentPointType::output; });

    // compare
    std::ranges::sort(positions_1);
    std::ranges::sort(positions_2);
    return positions_1 == positions_2;
}

[[nodiscard]] auto is_equivalent(const SegmentTree& segment_tree,
                                 const LineTree& line_tree) -> bool {
    return has_same_segments(segment_tree, line_tree) &&
           has_same_cross_points(segment_tree, line_tree) &&
           has_same_input_position(segment_tree, line_tree) &&
           has_same_output_positions(segment_tree, line_tree);
}

}  // namespace logicsim
