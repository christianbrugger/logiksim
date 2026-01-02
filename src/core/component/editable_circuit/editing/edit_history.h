#ifndef LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_HISTORY_H
#define LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_HISTORY_H

namespace logicsim {

namespace editable_circuit {

struct History;
struct CircuitData;

namespace editing {

[[nodiscard]] auto is_history_enabled(const History& history) -> bool;
[[nodiscard]] auto has_undo_entries(const History& history) -> bool;
[[nodiscard]] auto has_redo_entries(const History& history) -> bool;
[[nodiscard]] auto has_ungrouped_undo_entries(const History& history) -> bool;
[[nodiscard]] auto has_ungrouped_redo_entries(const History& history) -> bool;

auto enable_history(History& history) -> void;
auto disable_history(History& history) -> void;

auto undo_group(CircuitData& circuit) -> bool;
auto redo_group(CircuitData& circuit) -> bool;

auto clear_undo_history(CircuitData& circuit) -> void;
auto clear_redo_history(CircuitData& circuit) -> void;

auto finish_undo_group(History& history) -> void;
auto reopen_undo_group(History& history) -> void;

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim

#endif
