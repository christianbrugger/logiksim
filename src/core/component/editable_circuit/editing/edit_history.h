#ifndef LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_HISTORY_H
#define LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_HISTORY_H

namespace logicsim {

namespace editable_circuit {

struct CircuitHistory;
struct CircuitData;

namespace editing {

[[nodiscard]] auto has_undo_entries(CircuitHistory& history) -> bool;
[[nodiscard]] auto has_redo_entries(CircuitHistory& history) -> bool;
[[nodiscard]] auto has_ungrouped_undo_entries(CircuitHistory& history) -> bool;

auto undo_group(CircuitData& circuit) -> void;
auto redo_group(CircuitData& circuit) -> void;
auto finish_undo_group(CircuitHistory& history) -> void;

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim

#endif
