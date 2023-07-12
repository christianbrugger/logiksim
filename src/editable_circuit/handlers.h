#ifndef LOGIKSIM_EDITABLE_CIRCUIT_HANDLERS_H
#define LOGIKSIM_EDITABLE_CIRCUIT_HANDLERS_H

#include "editable_circuit/messages.h"
#include "layout.h"
#include "vocabulary.h"

#include <span>

namespace logicsim {

constexpr static inline auto DEBUG_PRINT_HANDLER_INPUTS = false;

class Selection;
class selection_handle_t;
class element_handle_t;
class SelectionRegistrar;
class CacheProvider;

namespace editable_circuit {

// contains common state for the handlers

struct State {
    Layout& layout;
    MessageSender sender;
    const CacheProvider& cache;
};

//
// Deletion Handling
//

using delete_queue_t = folly::small_vector<element_id_t, 6>;

auto swap_and_delete_multiple_elements(Layout& layout, MessageSender sender,
                                       std::span<const element_id_t> element_ids,
                                       element_id_t* preserve_element = nullptr) -> void;

auto swap_and_delete_single_element(Layout& layout, MessageSender sender,
                                    element_id_t& element_id,
                                    element_id_t* preserve_element = nullptr) -> void;

//
// Logic Item Handling
//

struct StandardLogicAttributes {
    ElementType type;
    std::size_t input_count;
    point_t position;
    orientation_t orientation = orientation_t::right;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const StandardLogicAttributes& other) const -> bool
        = default;
};

auto add_standard_logic_item(State state, StandardLogicAttributes attributes,
                             InsertionMode insertion_mode) -> element_id_t;

auto change_logic_item_insertion_mode(State state, element_id_t& element_id,
                                      InsertionMode new_insertion_mode) -> void;

// TODO move to dx, dy
auto is_logic_item_position_representable(const Layout& Layout,
                                          const element_id_t element_id, int x, int y)
    -> bool;

// TODO move to dx, dy
auto move_or_delete_logic_item(Layout& Layout, MessageSender sender,
                               element_id_t& element_id, int x, int y) -> void;

//
// Wire Handling
//

auto delete_wire_segment(Layout& layout, MessageSender sender,
                         segment_part_t& segment_part) -> void;

auto add_wire(State state, point_t p0, point_t p1, LineSegmentType segment_type,
              InsertionMode insertion_mode, Selection* selection) -> void;

auto change_wire_insertion_mode(State state, segment_part_t& segment_part,
                                InsertionMode new_insertion_mode) -> void;

auto is_wire_position_representable(const Layout& layout, segment_part_t segment_part,
                                    int dx, int dy) -> bool;

auto move_or_delete_wire(Layout& layout, MessageSender sender,
                         segment_part_t& segment_part, int dx, int dy) -> void;

//
// Wire - Low level Methods
//

auto move_segment_between_trees(Layout& layout, MessageSender sender,
                                segment_part_t& segment_part,
                                const element_id_t destination_element_id) -> void;

auto remove_segment_from_tree(Layout& layout, MessageSender sender,
                              segment_part_t& segment_part) -> void;

auto merge_and_delete_tree(Layout& Layout, MessageSender sender,
                           element_id_t& tree_destination, element_id_t& tree_source)
    -> void;

auto merge_line_segments(Layout& layout, MessageSender sender, segment_t segment_0,
                         segment_t segment_1, segment_part_t* preserve_segment) -> void;

auto fix_and_merge_segments(State state, const point_t position,
                            segment_part_t* preserve_segment) -> void;

auto add_wire_segment(State state, ordered_line_t line, InsertionMode insertion_mode)
    -> segment_part_t;

//
// Handle Methods
//

auto change_insertion_mode(selection_handle_t handle, State state,
                           InsertionMode new_insertion_mode) -> void;

auto new_positions_representable(const Selection& selection, const Layout& Layout,
                                 int delta_x, int delta_y) -> bool;

auto move_or_delete_elements(selection_handle_t handle, Layout& Layout,
                             MessageSender sender, int delta_x, int delta_y) -> void;

auto delete_all(selection_handle_t handle, State state) -> void;

}  // namespace editable_circuit
}  // namespace logicsim

#endif