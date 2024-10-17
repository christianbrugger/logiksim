#ifndef LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_DECORATION_H
#define LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_DECORATION_H

#include "core/vocabulary/insertion_mode.h"

namespace logicsim {

struct decoration_id_t;
struct DecorationDefinition;
struct point_t;
class Layout;
class Selection;

namespace editable_circuit {

struct CircuitData;

namespace editing {

auto delete_temporary_decoration(CircuitData& circuit, decoration_id_t& decoration_id,
                                 decoration_id_t* preserve_element = nullptr) -> void;

[[nodiscard]] auto is_decoration_position_representable(const Layout& layout,
                                                        decoration_id_t decoration_id,
                                                        int dx, int dy) -> bool;

[[nodiscard]] auto are_decoration_positions_representable(const Layout& layout,
                                                          const Selection& selection,
                                                          int delta_x,
                                                          int delta_y) -> bool;

/**
 * @brief:
 *
 * Pre-condition:
 *    + decoration is temporary
 *    + new position is representable
 */
auto move_temporary_decoration_unchecked(Layout& layout, decoration_id_t decoration_id,
                                         int dx, int dy) -> void;

auto move_or_delete_temporary_decoration(CircuitData& circuit,
                                         decoration_id_t& decoration_id, int dx,
                                         int dy) -> void;

auto change_decoration_insertion_mode(CircuitData& circuit,
                                      decoration_id_t& decoration_id,
                                      InsertionMode new_mode) -> void;

auto add_decoration(CircuitData& circuit, const DecorationDefinition& definition,
                    point_t position, InsertionMode insertion_mode) -> decoration_id_t;

auto toggle_inverter(CircuitData& circuit, point_t point) -> void;

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim

#endif
