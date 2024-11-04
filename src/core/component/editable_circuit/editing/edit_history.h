#ifndef LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_HISTORY_H
#define LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_HISTORY_H

namespace logicsim {

namespace editable_circuit {

struct CircuitHistory;
struct CircuitData;

namespace editing {

[[nodiscard]] auto is_history_enabled(const CircuitHistory& history) -> bool;
[[nodiscard]] auto has_undo_entries(const CircuitHistory& history) -> bool;
[[nodiscard]] auto has_redo_entries(const CircuitHistory& history) -> bool;
[[nodiscard]] auto has_ungrouped_undo_entries(const CircuitHistory& history) -> bool;
[[nodiscard]] auto has_ungrouped_redo_entries(const CircuitHistory& history) -> bool;

auto enable_history(CircuitHistory& history) -> void;
auto disable_history(CircuitHistory& history) -> void;

auto undo_group(CircuitData& circuit) -> void;
auto redo_group(CircuitData& circuit) -> void;

auto finish_undo_group(CircuitHistory& history) -> void;
auto finish_redo_group(CircuitHistory& history) -> void;

auto reopen_undo_group(CircuitHistory& history) -> void;
auto reopen_redo_group(CircuitHistory& history) -> void;

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim

#endif
