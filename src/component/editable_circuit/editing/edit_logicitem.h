#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_LOGICITEM_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_LOGICITEM_H

#include "vocabulary/insertion_mode.h"

namespace logicsim {

struct logicitem_id_t;
struct LogicItemDefinition;
struct point_t;
class Layout;
class Selection;

namespace editable_circuit {

struct CircuitData;

namespace editing {

auto delete_temporary_logicitem(CircuitData& circuit, logicitem_id_t& logicitem_id,
                                logicitem_id_t* preserve_element = nullptr) -> void;

[[nodiscard]] auto is_logicitem_position_representable(const Layout& layout,
                                                       logicitem_id_t logicitem_id,
                                                       int dx, int dy) -> bool;

[[nodiscard]] auto are_logicitem_positions_representable(const Layout& layout,
                                                         const Selection& selection,
                                                         int delta_x, int delta_y)
    -> bool;

/**
 * @brief:
 *
 * Pre-condition:
 *    + logicitem is temporary
 *    + new position is representable
 */
auto move_temporary_logicitem_unchecked(Layout& layout, logicitem_id_t logicitem_id,
                                        int dx, int dy) -> void;

auto move_or_delete_temporary_logicitem(CircuitData& circuit,
                                        logicitem_id_t& logicitem_id, int dx, int dy)
    -> void;

auto change_logicitem_insertion_mode(CircuitData& circuit, logicitem_id_t& logicitem_id,
                                     InsertionMode new_mode) -> void;

auto add_logicitem(CircuitData& circuit, const LogicItemDefinition& definition,
                   point_t position, InsertionMode insertion_mode) -> logicitem_id_t;

auto toggle_inverter(CircuitData& circuit, point_t point) -> void;

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim

#endif
