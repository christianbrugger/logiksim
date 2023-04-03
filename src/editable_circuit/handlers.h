#ifndef LOGIKSIM_EDITABLE_CIRCUIT_HANDLERS_H
#define LOGIKSIM_EDITABLE_CIRCUIT_HANDLERS_H

#include "circuit.h"
#include "editable_circuit/messages.h"
#include "vocabulary.h"

#include <span>

namespace logicsim {

class Selection;
class selection_handle_t;
class element_handle_t;
class SelectionRegistrar;
class CacheProvider;

inline SelectionRegistrar* _hack_registrar = {nullptr};
auto _hack_element_handle(element_id_t element_id) -> element_handle_t;

namespace editable_circuit {

// contains common data for the handlers

struct State {
    Circuit& circuit;
    Schematic& schematic;
    Layout& layout;

    MessageSender sender;
    const CacheProvider& cache;
};

//
// Deletion Handler
//

using delete_queue_t = folly::small_vector<element_id_t, 6>;

auto swap_and_delete_multiple_elements(Circuit& circuit, MessageSender sender,
                                       std::span<const element_id_t> element_ids) -> void;
auto swap_and_delete_single_element(Circuit& circuit, MessageSender sender,
                                    element_id_t& element_id) -> void;

//
// Logic Item Handler
//

struct StandardLogicAttributes {
    ElementType type;
    std::size_t input_count;
    point_t position;
    orientation_t orientation = orientation_t::right;
};

auto add_standard_logic_item(State state, Selection& selection,
                             StandardLogicAttributes attributes,
                             InsertionMode insertion_mode) -> void;

auto change_logic_item_insertion_mode(State state, element_id_t& element_id,
                                      InsertionMode new_insertion_mode) -> void;

auto is_logic_item_position_representable(const Circuit& circuit, element_id_t element_id,
                                          int x, int y) -> bool;

auto move_or_delete_logic_item(State state, element_id_t& element_id, int x, int y)
    -> void;

//
// Wire Handler
//

/*
auto add_connected_line(point_t p0, point_t p1, LineSegmentType segment_type,
                        InsertionMode insertion_mode) -> selection_handle_t;

auto change_wire_insertion_mode(segment_t& segment, segment_part_t part,
                                InsertionMode new_insertion_mode) -> void;
*/

}  // namespace editable_circuit
}  // namespace logicsim

#endif