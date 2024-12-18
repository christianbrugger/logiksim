#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_WIRE_DETAIL_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_WIRE_DETAIL_H

#include "core/algorithm/sort_pair.h"
#include "core/format/enum.h"
#include "core/format/struct.h"
#include "core/vocabulary/segment.h"
#include "core/vocabulary/segment_key.h"
#include "core/vocabulary/segment_part.h"
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
 * + Works for inserted and uninserted trees.
 * + Source tree can become empty.
 * + Newly inserted endpoints as of type shadow_point.
 *
 * Note, the segment with the smallest line preserves the initial key.
 */
auto move_segment_between_trees(CircuitData& circuit, segment_part_t& segment_part,
                                wire_id_t destination_id) -> void;

auto move_segment_between_trees_with_history(CircuitData& circuit,
                                             segment_part_t& segment_part,
                                             wire_id_t destination_id) -> void;

enum class move_segment_type {
    move_full_segment,
    move_touching_segment,
    move_splitting_segment,
};

}  // namespace editing
}  // namespace editable_circuit

template <>
[[nodiscard]] auto format(editable_circuit::editing::move_segment_type type)
    -> std::string;

namespace editable_circuit {
namespace editing {

[[nodiscard]] auto get_move_segment_type(
    const Layout& layout, segment_part_t segment_part) -> move_segment_type;

auto move_full_segment_between_trees(CircuitData& circuit, segment_t& source_segment,
                                     wire_id_t destination_id) -> void;

struct move_touching_result_t {
    // the moved segment
    segment_part_t moved;
    // the non moved segment that was remaining
    segment_part_t other;

    // the part that was at the beginning of the segment
    segment_part_t begin;
    // the part at the end of the segment
    segment_part_t end;

    [[nodiscard]] auto operator==(const move_touching_result_t&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

auto move_touching_segment_between_trees(
    CircuitData& circuit, segment_part_t& source_segment_part, wire_id_t destination_id,
    segment_key_t optional_end_key = null_segment_key) -> move_touching_result_t;

auto move_touching_segment_between_trees_with_history(
    CircuitData& circuit, segment_part_t& source_segment_part, wire_id_t destination_id,
    segment_key_t optional_end_key = null_segment_key) -> move_touching_result_t;

struct move_splitting_keys_t {
    segment_key_t new_middle_key {null_segment_key};
    segment_key_t new_end_key {null_segment_key};

    [[nodiscard]] auto operator==(const move_splitting_keys_t&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct move_splitting_result_t {
    segment_part_t begin;
    segment_part_t middle;
    segment_part_t end;

    [[nodiscard]] auto operator==(const move_splitting_result_t&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

auto move_splitting_segment_between_trees(
    CircuitData& circuit, segment_part_t& source_segment_part, wire_id_t destination_id,
    move_splitting_keys_t optional_keys = {}) -> move_splitting_result_t;

auto move_splitting_segment_between_trees_with_history(
    CircuitData& circuit, segment_part_t& source_segment_part, wire_id_t destination_id,
    move_splitting_keys_t optional_keys = {}) -> move_splitting_result_t;

/**
 * @brief: Deletes segment_part of tree (only uninserted)
 *
 * + Source tree can become empty
 * + Newly inserts endpoints as shadow points
 */
auto remove_full_segment_from_uninserted_tree(CircuitData& circuit,
                                              segment_part_t& full_segment_part) -> void;

/**
 * @brief: Split the line segment at the position.
 *
 * Note, the segment with the smallest line preserves the initial key.
 */
auto split_line_segment(CircuitData& circuit, segment_t segment,
                        point_t position) -> segment_part_t;

[[nodiscard]] auto are_segments_mergeable(const Layout& layout, segment_t segment_0,
                                          segment_t segment_1) -> bool;
/**
 * @brief: Merges the two given segments. Optionally preserves an additional segment.
 *
 * Throws, if segments are not adjacent
 *
 * Note, key of the smaller line is the final key.
 *
 * Returns merged segment.
 */
auto merge_line_segment(CircuitData& circuit, segment_t segment_0, segment_t segment_1,
                        segment_part_t* preserve_segment = nullptr) -> segment_t;

/**
 * @brief: Merge two line segments.
 *
 * Note, the resulting segment has the key of first segment when ordered by lines.
 */
auto merge_line_segment_with_history(
    CircuitData& circuit, segment_t segment_0, segment_t segment_1,
    segment_part_t* preserve_segment = nullptr) -> segment_t;

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

[[nodiscard]] auto temporary_endpoints_valid(endpoints_t) -> bool;

auto set_temporary_endpoints(Layout& layout, segment_t segment,
                             endpoints_t endpoints) -> void;

auto set_inserted_endpoints(CircuitData& circuit, segment_t segment,
                            endpoints_t endpoints) -> void;
auto set_inserted_endpoints(CircuitData& circuit, segment_t segment, point_t position,
                            SegmentPointType type) -> void;

auto reset_temporary_endpoints(Layout& layout, segment_t segment) -> void;

auto set_temporary_crosspoint(Layout& layout, segment_t segment, point_t point) -> void;

using point_update_t =
    std::initializer_list<const std::pair<segment_index_t, SegmentPointType>>;

auto update_inserted_segment_endpoints(CircuitData& circuit, wire_id_t wire_id,
                                       point_update_t data, point_t position) -> void;

auto fix_and_merge_inserted_segments(CircuitData& circuit, point_t position,
                                     segment_part_t* preserve_segment = nullptr) -> void;

//
// Valid
//

auto mark_valid(Layout& layout, segment_part_t segment_part) -> void;

auto unmark_valid(Layout& layout, segment_part_t segment_part) -> void;

auto mark_valid_with_history(CircuitData& circuit, segment_part_t segment_part) -> void;

auto unmark_valid_with_history(CircuitData& circuit, segment_part_t segment_part) -> void;

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
