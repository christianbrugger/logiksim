#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_LOGICITEM_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_LOGICITEM_H

#include "core/vocabulary/insertion_mode.h"
#include "core/vocabulary/logicitem_key.h"

namespace logicsim {

struct logicitem_id_t;
struct attributes_clock_generator_t;
struct LogicItemDefinition;
struct point_t;
struct move_delta_t;
class Layout;
class Selection;

namespace editable_circuit {

struct CircuitData;

namespace editing {

auto delete_temporary_logicitem(CircuitData& circuit,
                                logicitem_id_t& logicitem_id) -> void;

[[nodiscard]] auto is_logicitem_position_representable(const Layout& layout,
                                                       logicitem_id_t logicitem_id,
                                                       move_delta_t delta) -> bool;

[[nodiscard]] auto are_logicitem_positions_representable(const Layout& layout,
                                                         const Selection& selection,
                                                         move_delta_t delta) -> bool;

/**
 * @brief:
 *
 * Pre-condition:
 *    + logicitem is temporary
 *    + new position is representable
 */
auto move_temporary_logicitem_unchecked(CircuitData& circuit, logicitem_id_t logicitem_id,
                                        move_delta_t delta) -> void;

auto move_or_delete_temporary_logicitem(CircuitData& circuit,
                                        logicitem_id_t& logicitem_id,
                                        move_delta_t delta) -> void;

auto change_logicitem_insertion_mode(CircuitData& circuit, logicitem_id_t& logicitem_id,
                                     InsertionMode new_mode,
                                     InsertionHint hint = InsertionHint::no_hint) -> void;

auto add_logicitem(CircuitData& circuit, LogicItemDefinition&& definition,
                   point_t position, InsertionMode insertion_mode,
                   logicitem_key_t logicitem_key = null_logicitem_key) -> logicitem_id_t;

auto set_attributes_logicitem(CircuitData& circuit, logicitem_id_t logicitem_id,
                              attributes_clock_generator_t&& attrs) -> void;

auto toggle_inverter(CircuitData& circuit, point_t point) -> void;

auto add_to_visible_selection(CircuitData& circuit_data,
                              logicitem_id_t logicitem_id) -> void;
auto remove_from_visible_selection(CircuitData& circuit_data,
                                   logicitem_id_t logicitem_id) -> void;
}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim

#endif
