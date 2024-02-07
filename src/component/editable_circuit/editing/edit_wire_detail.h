#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_WIRE_DETAIL_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_WIRE_DETAIL_H

#include "vocabulary/display_state.h"
#include "vocabulary/insertion_mode.h"
#include "vocabulary/segment_point_type.h"

#include <initializer_list>
#include <utility>

namespace logicsim {

struct wire_id_t;
struct segment_index_t;
struct segment_t;
struct segment_part_t;

struct point_t;
struct ordered_line_t;

class Layout;

namespace editable_circuit {

struct CircuitData;

namespace editing {

//
// Segment Operations
//

auto add_segment_to_tree(CircuitData& circuit, wire_id_t wire_id, ordered_line_t line)
    -> segment_part_t;

/**
 * @brief: Moves the segment_part to the destination tree.
 *
 * Note either one of the trees can be inserted or uninserted
 *
 * + Source tree can become empty
 * + Newly inserted endpoints as shadow points
 */
auto move_segment_between_trees(CircuitData& circuit, segment_part_t& segment_part,
                                wire_id_t destination_id) -> void;

/**
 * @brief: Deletes segment_part of tree (only uninserted)
 *
 * + Source tree can become empty
 * + Newly inserts endpoints as shadow points
 */
auto remove_segment_from_tree(CircuitData& circuit, segment_part_t& segment_part) -> void;

auto split_line_segment(CircuitData& circuit, segment_t segment, point_t position)
    -> segment_part_t;

auto merge_line_segments(CircuitData& circuit, segment_t segment_0, segment_t segment_1,
                         segment_part_t* preserve_segment) -> void;

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

auto reset_segment_endpoints(Layout& layout, segment_t segment) -> void;

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

[[nodiscard]] auto is_wire_colliding(const CircuitData& circuit, ordered_line_t line)
    -> bool;

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim

#endif
