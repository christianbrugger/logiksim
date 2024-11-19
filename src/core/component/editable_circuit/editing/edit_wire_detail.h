#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_WIRE_DETAIL_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_WIRE_DETAIL_H

#include "core/algorithm/sort_pair.h"
#include "core/vocabulary/segment.h"
#include "core/vocabulary/segment_point_type.h"

#include <initializer_list>
#include <utility>
#include <vector>

namespace logicsim {

struct wire_id_t;
struct segment_index_t;
struct segment_t;
struct segment_part_t;
struct segment_info_t;
struct endpoints_t;

struct point_t;
struct ordered_line_t;

class Layout;

namespace editable_circuit {

struct CircuitData;

namespace editing {

//
// Segment Operations
//

auto add_temporary_segment(CircuitData& circuit, ordered_line_t line) -> segment_part_t;

/**
 * @brief: Moves the segment_part to the destination tree.
 *
 * Note either one of the trees can be inserted or uninserted
 *
 * + Source tree can become empty
 * + Newly inserted endpoints as shadow points
 */
auto move_segment_between_trees(CircuitData& circuit, segment_part_t& segment_part,
                                wire_id_t destination_id)
    -> std::pair<segment_part_t, segment_part_t>;

/**
 * @brief: Deletes segment_part of tree (only uninserted)
 *
 * + Source tree can become empty
 * + Newly inserts endpoints as shadow points
 */
auto remove_full_segment_from_uninserted_tree(CircuitData& circuit,
                                              segment_part_t& full_segment_part) -> void;

auto split_line_segment(CircuitData& circuit, segment_t segment,
                        point_t position) -> segment_part_t;

auto merge_line_segments(CircuitData& circuit, segment_t segment_0, segment_t segment_1,
                         segment_part_t* preserve_segment) -> segment_t;

auto merge_all_line_segments(CircuitData& circuit,
                             std::vector<std::pair<segment_t, segment_t>>& pairs) -> void;
template <std::invocable<CircuitData&, segment_t, segment_t> MergeFunc>
auto merge_all_line_segments(CircuitData& circuit,
                             std::vector<std::pair<segment_t, segment_t>>& pairs,
                             MergeFunc merge_segments) -> void;

//
// Wire Operations
//

auto swap_and_delete_empty_wire(CircuitData& circuit, wire_id_t& wire_id,
                                wire_id_t* preserve_element = nullptr) -> void;

/**
 * @brief: Splits a tree with a missing middle segment into two trees.
 *
 * + assume we get a valid tree where the part between p0 and p1 is missing
 * + puts the segments at p1 into a new tree that is returned
 */
// TODO throw if not broken tree
auto split_broken_tree(CircuitData& circuit, point_t p0, point_t p1) -> wire_id_t;

// TODO sort arguments
auto merge_and_delete_tree(CircuitData& circuit, wire_id_t& tree_destination,
                           wire_id_t& tree_source) -> void;

//
// Endpoints
//

auto set_uninserted_endpoints(Layout& layout, segment_t segment,
                              endpoints_t endpoints) -> void;

auto reset_uninserted_endpoints(Layout& layout, segment_t segment) -> void;

auto set_uninserted_crosspoint(Layout& layout, segment_t segment, point_t point) -> void;

using point_update_t =
    std::initializer_list<const std::pair<segment_index_t, SegmentPointType>>;

auto update_segment_point_types(CircuitData& circuit, wire_id_t wire_id,
                                point_update_t data, point_t position) -> void;

auto fix_and_merge_segments(CircuitData& circuit, point_t position,
                            segment_part_t* preserve_segment = nullptr) -> void;

//
// Valid
//

auto mark_valid(Layout& layout, segment_part_t segment_part) -> void;

auto unmark_valid(Layout& layout, segment_part_t segment_part) -> void;

//
// Collisions
//

[[nodiscard]] auto is_wire_colliding(const CircuitData& circuit,
                                     ordered_line_t line) -> bool;

//
// Connections
//

auto set_wire_inputs_at_logicitem_outputs(CircuitData& circuit,
                                          segment_t segment) -> void;

//
// Implementation
//

template <std::invocable<CircuitData&, segment_t, segment_t> MergeFunc>
auto merge_all_line_segments(CircuitData& circuit,
                             std::vector<std::pair<segment_t, segment_t>>& pairs,
                             MergeFunc merge_segments) -> void {
    // merging deletes the segment with highest segment index,
    // so for this to work with multiple segments
    // we need to be sort them in descendant order
    for (auto& pair : pairs) {
        sort_inplace(pair.first, pair.second, std::ranges::greater {});
    }
    std::ranges::sort(pairs, std::ranges::greater {});

    // Sorted pairs example:
    //  (<Wire 0, Segment 6>, <Wire 0, Segment 5>)
    //  (<Wire 0, Segment 5>, <Wire 0, Segment 3>)
    //  (<Wire 0, Segment 4>, <Wire 0, Segment 2>)
    //  (<Wire 0, Segment 4>, <Wire 0, Segment 0>)  <-- 4 needs to become 2 (as merged)
    //  (<Wire 0, Segment 3>, <Wire 0, Segment 1>)
    //  (<Wire 0, Segment 2>, <Wire 0, Segment 1>)
    //                                              <-- move here & become 1

    for (auto it = pairs.begin(); it != pairs.end(); ++it) {
        // first one is save to merge, as it has the largest indices in first and second
        std::invoke(merge_segments, circuit, it->first, it->second);

        // check if there is another element merged with the same segment
        const auto other = std::ranges::lower_bound(
            std::next(it), pairs.end(), it->first, std::ranges::greater {},
            [](std::pair<segment_t, segment_t> pair) { return pair.first; });

        // if one is found there is at most one other segment
        // the first part now moved and is now the second, as it has been merged
        if (other != pairs.end() && other->first == it->first) {
            // update the other side of the merged element to the lower index
            other->first = it->second;

            // rebuild the ordering
            sort_inplace(other->first, other->second, std::ranges::greater {});
            std::ranges::sort(std::next(it), pairs.end(), std::ranges::greater {});
        }
    }
}
}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim

#endif
