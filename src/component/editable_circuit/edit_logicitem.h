#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDIT_LOGICITEM_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDIT_LOGICITEM_H

#include "vocabulary/insertion_mode.h"

namespace logicsim {

struct logicitem_id_t;
struct LogicItemDefinition;
struct point_t;
class Layout;

namespace editable_circuit {

struct CircuitData;

auto delete_temporary_logic_item(CircuitData& circuit, logicitem_id_t& logicitem_id,
                                 logicitem_id_t* preserve_element = nullptr) -> void;

auto is_logic_item_position_representable(const Layout& layout,
                                          const logicitem_id_t logicitem_id, int dx,
                                          int dy) -> bool;

/**
 * @brief:
 *
 * Pre-condition:
 *    + logicitem is temporary
 *    + new position is representable
 */
auto move_temporary_logic_item_unchecked(Layout& layout,
                                         const logicitem_id_t logicitem_id, int dx,
                                         int dy) -> void;

auto move_or_delete_temporary_logic_item(CircuitData& circuit,
                                         logicitem_id_t& logicitem_id, int dx, int dy)
    -> void;

auto change_logic_item_insertion_mode(CircuitData& circuit, logicitem_id_t& logicitem_id,
                                      InsertionMode new_insertion_mode) -> void;

auto add_logic_item(CircuitData& circuit, const LogicItemDefinition& definition,
                    point_t position, InsertionMode insertion_mode) -> logicitem_id_t;

auto toggle_inverter(CircuitData& circuit, point_t point) -> void;

}  // namespace editable_circuit

}  // namespace logicsim

#endif
