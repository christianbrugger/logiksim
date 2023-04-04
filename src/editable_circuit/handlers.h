#ifndef LOGIKSIM_EDITABLE_CIRCUIT_HANDLERS_H
#define LOGIKSIM_EDITABLE_CIRCUIT_HANDLERS_H

#include "circuit.h"
#include "editable_circuit/messages.h"
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
    Circuit& circuit;
    MessageSender sender;
    const CacheProvider& cache;

    // derived
    Schematic& schematic;
    Layout& layout;
};

//
// Deletion Handling
//

using delete_queue_t = folly::small_vector<element_id_t, 6>;

auto swap_and_delete_multiple_elements(Circuit& circuit, MessageSender sender,
                                       std::span<const element_id_t> element_ids,
                                       element_id_t* preserve_element = nullptr) -> void;

auto swap_and_delete_single_element(Circuit& circuit, MessageSender sender,
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

auto is_logic_item_position_representable(const Circuit& circuit, element_id_t element_id,
                                          int x, int y) -> bool;

auto move_or_delete_logic_item(State state, element_id_t& element_id, int x, int y)
    -> void;

//
// Wire Handling
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