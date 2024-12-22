#include "core/line_tree_generation.h"

#include "core/algorithm/make_unique.h"
#include "core/algorithm/transform_if.h"
#include "core/algorithm/transform_to_vector.h"
#include "core/geometry/segment_info.h"
#include "core/index/connection_index.h"
#include "core/layout.h"
#include "core/line_tree.h"
#include "core/segment_tree.h"
#include "core/tree_normalization.h"

#include <stdexcept>

namespace logicsim {

namespace {

auto is_convertable_output(point_t point, SegmentPointType type,
                           const LogicItemInputIndex& index) -> bool {
    return type == SegmentPointType::output && !index.find(point);
}

auto find_root(const SegmentTree& segment_tree,
               const LogicItemInputIndex& index) -> point_t {
    if (segment_tree.has_input()) {
        return segment_tree.input_position();
    }

    for (const segment_info_t& info : segment_tree) {
        for (const auto& [point, type] : to_point_and_type(info)) {
            if (is_convertable_output(point, type, index)) {
                return point;
            }
        }
    }

    throw std::runtime_error("tree has no input or convertible outputs");
}

auto generate_line_tree_impl(const SegmentTree& segment_tree,
                             const LogicItemInputIndex& index) -> LineTree {
    const auto root = find_root(segment_tree, index);
    const auto segments = transform_to_vector(all_lines(segment_tree));
    return to_line_tree(segments, root);
}

}  // namespace

auto generate_line_tree(const SegmentTree& segment_tree,
                        const LogicItemInputIndex& index) -> LineTree {
    // pre-condition
    assert(is_contiguous_tree_with_correct_endpoints(segment_tree));

    const auto line_tree = generate_line_tree_impl(segment_tree, index);

    // post-condition
    assert(is_equivalent(segment_tree, line_tree));
    return line_tree;
}

auto generate_line_trees(const Layout& layout,
                         const LogicItemInputIndex& index) -> std::vector<LineTree> {
    const auto gen_line_tree = [&](wire_id_t wire_id) {
        if (is_inserted(wire_id)) {
            return generate_line_tree(layout.wires().segment_tree(wire_id), index);
        }
        return LineTree {};
    };

    return transform_to_vector(wire_ids(layout), gen_line_tree);
}

auto has_same_segments(const SegmentTree& segment_tree,
                       const LineTree& line_tree) -> bool {
    // line tree
    if (line_tree.size() != segment_tree.size()) {
        return false;
    }

    auto segments_1 = transform_to_vector(
        line_tree.lines(), [](const line_t& line) { return ordered_line_t {line}; });
    auto segments_2 = transform_to_vector(all_lines(segment_tree));

    std::ranges::sort(segments_1);
    std::ranges::sort(segments_2);
    return segments_1 == segments_2;
}

auto get_cross_points(const LineTree& line_tree) -> std::vector<point_t> {
    auto cross_points = std::vector<point_t> {};
    transform_if(
        indices(line_tree), std::back_inserter(cross_points),
        [&](line_index_t index) -> point_t { return line_tree.line(index).p0; },
        [&](line_index_t index) -> bool { return line_tree.has_cross_point_p0(index); });

    sort_and_make_unique(cross_points);
    return cross_points;
}

auto has_same_cross_points(const SegmentTree& segment_tree,
                           const LineTree& line_tree) -> bool {
    // line tree
    auto cross_points_1 = get_cross_points(line_tree);

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

auto has_same_corner_points(const SegmentTree& segment_tree,
                            const LineTree& line_tree) -> bool {
    // line tree
    auto corners_1 = std::vector<point_t> {};

    transform_if(
        indices(line_tree), std::back_inserter(corners_1),
        [&](line_index_t index) -> point_t { return line_tree.line(index).p0; },
        [&](line_index_t index) -> bool { return line_tree.is_corner_p0(index); });
    transform_if(
        indices(line_tree), std::back_inserter(corners_1),
        [&](line_index_t index) -> point_t { return line_tree.line(index).p1; },
        [&](line_index_t index) -> bool { return line_tree.is_corner_p1(index); });

    sort_and_make_unique(corners_1);

    // remove cross-points (false-positives)
    auto cross_points_1 = get_cross_points(line_tree);
    auto corners_1_filtered = std::vector<point_t> {};
    std::ranges::set_difference(corners_1, cross_points_1,
                                std::back_inserter(corners_1_filtered));

    // segment tree
    auto corners_2 = std::vector<point_t> {};

    transform_if(
        segment_tree, std::back_inserter(corners_2),
        [](segment_info_t info) { return info.line.p0; },
        [](segment_info_t info) { return is_corner_point(info.p0_type); });
    transform_if(
        segment_tree, std::back_inserter(corners_2),
        [](segment_info_t info) { return info.line.p1; },
        [](segment_info_t info) { return is_corner_point(info.p1_type); });

    sort_and_make_unique(corners_2);

    return corners_1_filtered == corners_2;
}

auto has_same_input_position(const SegmentTree& segment_tree,
                             const LineTree& line_tree) -> bool {
    return !segment_tree.has_input() ||
           segment_tree.input_position() == line_tree.input_position();
}

auto has_same_output_positions(const SegmentTree& segment_tree,
                               const LineTree& line_tree) -> bool {
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
           has_same_corner_points(segment_tree, line_tree) &&
           has_same_input_position(segment_tree, line_tree) &&
           has_same_output_positions(segment_tree, line_tree);
}

auto all_wires_equivalent(const Layout& layout,
                          const std::vector<LineTree>& line_trees) -> bool {
    if (layout.wires().size() != line_trees.size()) {
        return false;
    }

    const auto is_wire_equivalent = [&](wire_id_t wire_id) {
        const auto& line_tree = line_trees.at(wire_id.value);

        if (is_inserted(wire_id)) {
            return is_equivalent(layout.wires().segment_tree(wire_id), line_tree);
        }
        return line_tree.empty();
    };

    return std::ranges::all_of(wire_ids(layout), is_wire_equivalent);
}

}  // namespace logicsim
