#ifndef LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_DECORATION_H
#define LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_DECORATION_H

#include "core/vocabulary/decoration_key.h"
#include "core/vocabulary/insertion_mode.h"

namespace logicsim {

struct move_delta_t;
struct decoration_id_t;
struct attributes_text_element_t;
struct DecorationDefinition;
struct point_t;
class Layout;
class Selection;

namespace editable_circuit {

struct CircuitData;

namespace editing {

auto delete_temporary_decoration(CircuitData& circuit,
                                 decoration_id_t& decoration_id) -> void;

[[nodiscard]] auto is_decoration_position_representable(const Layout& layout,
                                                        decoration_id_t decoration_id,
                                                        move_delta_t delta) -> bool;

[[nodiscard]] auto are_decoration_positions_representable(const Layout& layout,
                                                          const Selection& selection,
                                                          move_delta_t delta) -> bool;

/**
 * @brief:
 *
 * Pre-condition:
 *    + decoration is temporary
 *    + new position is representable
 */
auto move_temporary_decoration_unchecked(CircuitData& circuit,
                                         decoration_id_t decoration_id,
                                         move_delta_t delta) -> void;

auto move_or_delete_temporary_decoration(CircuitData& circuit,
                                         decoration_id_t& decoration_id,
                                         move_delta_t delta) -> void;

auto change_decoration_insertion_mode(CircuitData& circuit,
                                      decoration_id_t& decoration_id,
                                      InsertionMode new_mode) -> void;

auto add_decoration(CircuitData& circuit, DecorationDefinition&& definition,
                    point_t position, InsertionMode insertion_mode,
                    decoration_key_t decoration_key = null_decoration_key)
    -> decoration_id_t;

auto set_attributes_decoration(CircuitData& circuit, decoration_id_t decoration_id,
                               attributes_text_element_t&& attrs) -> void;

auto add_to_visible_selection(CircuitData& circuit_data,
                              decoration_id_t decoration_id) -> void;
auto remove_from_visible_selection(CircuitData& circuit_data,
                                   decoration_id_t decoration_id) -> void;

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim

#endif
