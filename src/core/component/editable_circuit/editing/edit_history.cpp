#include "core/component/editable_circuit/editing/edit_history.h"

#include "core/component/editable_circuit/circuit_data.h"
#include "core/component/editable_circuit/history.h"
#include "core/logging.h"

namespace logicsim {

namespace editable_circuit {

namespace editing {

auto is_history_enabled(const CircuitHistory& history) -> bool {
    return history.state != HistoryState::disabled;
}

auto has_undo_entries(const CircuitHistory& history) -> bool {
    return !history.undo_stack.empty();
}

auto has_redo_entries(const CircuitHistory& history) -> bool {
    static_cast<void>(history);
    return false;
}

auto has_ungrouped_undo_entries(const CircuitHistory& history) -> bool {
    return false && (!history.undo_stack.empty() &&
                     history.undo_stack.back().type != UndoType::new_group);
}

auto enable_history(CircuitHistory& history) -> void {
    history.state = HistoryState::track_undo;
}

auto undo_group(CircuitData& circuit) -> void {
    static_cast<void>(circuit);
    print("RUN UNDO GROUP");
}

auto redo_group(CircuitData& circuit) -> void {
    static_cast<void>(circuit);
    print("RUN REDO GROUP");
}

auto finish_undo_group(CircuitHistory& history) -> void {
    // if (has_ungrouped_undo_entries(history)) {
    if (history.state == HistoryState::track_undo) {
        history.undo_stack.emplace_back(DecorationUndoEntry {
            .type = UndoType::new_group,
        });
    }
    // }
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
