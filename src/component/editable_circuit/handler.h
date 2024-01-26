#ifndef LOGIKSIM_COMPONENT_EDITABLE_CIRCUIT_HANDLER_H
#define LOGIKSIM_COMPONENT_EDITABLE_CIRCUIT_HANDLER_H

#include "layout_message_forward.h"
#include "vocabulary/insertion_mode.h"
#include "vocabulary/line_insertion_type.h"

#include <optional>
#include <span>
#include <vector>

namespace logicsim {

struct wire_id_t;
struct logicitem_id_t;

struct point_t;
struct line_t;
struct ordered_line_t;
struct part_t;
struct segment_t;
struct segment_part_t;
struct LogicItemDefinition;
struct PlacedElement;

class Layout;
class Selection;
class LayoutIndex;

namespace editable_circuit {
class MessageSender;
}

namespace editable_circuit {

// contains common state for the handlers

// TODO class-invariants:
//    + Expects(segment_tree(temporary_wire).output_count() == connection_count_t {0});
//    + Expects(segment_tree(invalid_wire).output_count() == connection_count_t {0});
//    + no-valid-parts - std::ranges::any_of(tree.valid_parts(), &PartSelection::size);

struct State {
    Layout& layout;
    MessageSender& sender;
    const LayoutIndex& cache;
};

//
// Deletion Handling
//

// using delete_queue_t = folly::small_vector<element_id_t, 6>;

// auto swap_and_delete_multiple_elements(Layout& layout, MessageSender& sender,
//                                        std::span<const element_id_t> element_ids,
//                                        element_id_t* preserve_element = nullptr) ->
//                                        void;

// auto swap_and_delete_single_element(Layout& layout, MessageSender& sender,
//                                     element_id_t& element_id,
//                                     element_id_t* preserve_element = nullptr) -> void;

auto swap_and_delete_empty_wire(Layout& layout, MessageSender& sender, wire_id_t& wire_id,
                                wire_id_t* preserve_element = nullptr) -> void;

auto swap_and_delete_logic_item(Layout& layout, MessageSender& sender,
                                logicitem_id_t& logicitem_id,
                                logicitem_id_t* preserve_element = nullptr) -> void;

//
// Logic Item Handling
//

auto add_logic_item(State state, const LogicItemDefinition& definition, point_t position,
                    InsertionMode insertion_mode) -> logicitem_id_t;

auto change_logic_item_insertion_mode(State state, logicitem_id_t& logicitem_id,
                                      InsertionMode new_insertion_mode) -> void;

auto is_logic_item_position_representable(const Layout& Layout,
                                          const logicitem_id_t logicitem_id, int dx,
                                          int dy) -> bool;

auto move_or_delete_logic_item(Layout& Layout, MessageSender& sender,
                               logicitem_id_t& logicitem_id, int dx, int dy) -> void;

// Assumptions:
//   * all new positions are representable
auto move_logic_item_unchecked(Layout& layout, const logicitem_id_t logicitem_id, int dx,
                               int dy) -> void;

auto toggle_inverter(Layout& layout, const LayoutIndex& cache, point_t point) -> void;

//
// Wire - High level Methods
//

auto delete_wire_segment(Layout& layout, MessageSender& sender,
                         segment_part_t& segment_part) -> void;

auto add_wire(State state, point_t p0, point_t p1, LineInsertionType segment_type,
              InsertionMode insertion_mode, Selection* selection) -> void;

auto add_wire_segment(State state, Selection* selection, line_t line,
                      InsertionMode insertion_mode) -> void;

auto change_wire_insertion_mode(State state, segment_part_t& segment_part,
                                InsertionMode new_insertion_mode) -> void;

auto is_wire_position_representable(const Layout& layout, segment_part_t segment_part,
                                    int dx, int dy) -> bool;

auto move_or_delete_wire(Layout& layout, MessageSender& sender,
                         segment_part_t& segment_part, int dx, int dy) -> void;

// Assumptions:
//   * all new positions are representable
auto move_wire_unchecked(Layout& layout, segment_t segment, part_t verify_full_part,
                         int dx, int dy) -> void;

auto toggle_inserted_wire_crosspoint(State state, point_t point) -> void;

//
// Wire - Low level Methods
//

auto move_segment_between_trees(Layout& layout, MessageSender& sender,
                                segment_part_t& segment_part,
                                const wire_id_t destination_id) -> void;

auto remove_segment_from_tree(Layout& layout, MessageSender& sender,
                              segment_part_t& segment_part) -> void;

auto merge_and_delete_tree(Layout& Layout, MessageSender& sender,
                           wire_id_t& tree_destination, wire_id_t& tree_source) -> void;

auto merge_line_segments(Layout& layout, MessageSender& sender, segment_t segment_0,
                         segment_t segment_1, segment_part_t* preserve_segment) -> void;

auto fix_and_merge_segments(State state, const point_t position,
                            segment_part_t* preserve_segment) -> void;

auto add_wire_segment(State state, ordered_line_t line, InsertionMode insertion_mode)
    -> segment_part_t;

//
// Handle Methods
//

// TODO needs to be tracked selection
auto change_insertion_mode(Selection& selection, State state,
                           InsertionMode new_insertion_mode) -> void;

auto new_positions_representable(const Selection& selection, const Layout& Layout,
                                 int delta_x, int delta_y) -> bool;

// TODO needs to be tracked selection
auto move_or_delete_elements(Selection& selection, Layout& Layout, MessageSender& sender,
                             int delta_x, int delta_y) -> void;

// Assumptions:
//   * all new positions are representable
auto move_unchecked(const Selection& selection, Layout& layout, int delta_x, int delta_y)
    -> void;

// TODO needs to be tracked selection
auto delete_all(Selection& selection, State state) -> void;

//
// Wire Mode Change Helpers
//

// TODO does not need tracked selection
auto regularize_temporary_selection(Layout& layout, MessageSender& sender,
                                    const Selection& selection,
                                    std::optional<std::vector<point_t>> true_cross_points)
    -> std::vector<point_t>;

auto capture_inserted_cross_points(const Layout& layout, const LayoutIndex& cache,
                                   const Selection& selection) -> std::vector<point_t>;

// TODO does not need tracked selection
auto split_temporary_segments(Layout& layout, MessageSender& sender,
                              std::span<const point_t> split_points,
                              const Selection& selection) -> void;

auto capture_new_splitpoints(const Layout& layout, const LayoutIndex& cache,
                             const Selection& selection) -> std::vector<point_t>;

}  // namespace editable_circuit
}  // namespace logicsim

#endif