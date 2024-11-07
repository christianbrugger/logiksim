#include "core/component/editable_circuit/editing/edit_history.h"

#include "core/component/editable_circuit/circuit_data.h"
#include "core/component/editable_circuit/editing/edit_decoration.h"
#include "core/component/editable_circuit/editing/edit_visible_selection.h"
#include "core/component/editable_circuit/history.h"

#include <gsl/gsl>

namespace logicsim {

namespace editable_circuit {

namespace editing {

auto is_history_enabled(const History& history) -> bool {
    return history.state != HistoryState::disabled;
}

auto has_undo_entries(const History& history) -> bool {
    return !history.undo_stack.empty();
}

auto has_redo_entries(const History& history) -> bool {
    return !history.redo_stack.empty();
}

auto has_ungrouped_undo_entries(const History& history) -> bool {
    return has_ungrouped_entries(history.undo_stack);
}

auto has_ungrouped_redo_entries(const History& history) -> bool {
    return has_ungrouped_entries(history.redo_stack);
}

auto enable_history(History& history) -> void {
    history.state = HistoryState::track_undo_new;
}

auto disable_history(History& history) -> void {
    history.state = HistoryState::disabled;
}

namespace {

auto to_id(decoration_key_t decoration_key, CircuitData& circuit) -> decoration_id_t {
    return circuit.index.key_index().get(decoration_key);
}

auto _store_history_new_group(History& history) -> void {
    if (auto stack = history.get_stack()) {
        stack->push_new_group();
    }
}

auto _replay_last_entry(CircuitData& circuit, HistoryStack& stack) -> void {
    switch (stack.top_entry().value()) {
        using enum HistoryEntry;
        case new_group: {
            stack.pop_new_group();
            _store_history_new_group(circuit.history);
            return;
        }

        case decoration_create_temporary: {
            auto [decoration_key, placed_decoration] =
                stack.pop_decoration_create_temporary();

            editing::add_decoration(circuit, std::move(placed_decoration.definition),
                                    placed_decoration.position, InsertionMode::temporary,
                                    decoration_key);
            return;
        }

        case decoration_delete_temporary: {
            auto decoration_id = to_id(stack.pop_decoration_delete_temporary(), circuit);
            editing::delete_temporary_decoration(circuit, decoration_id);
            return;
        }

        case decoration_move_temporary: {
            const auto [decoration_key, delta] = stack.pop_decoration_move_temporary();
            const auto decoration_id = to_id(decoration_key, circuit);

            editing::move_temporary_decoration_unchecked(circuit, decoration_id, delta);
            return;
        }

        case decoration_to_mode_temporary: {
            auto decoration_id = to_id(stack.pop_decoration_to_mode_temporary(), circuit);
            editing::change_decoration_insertion_mode(circuit, decoration_id,
                                                      InsertionMode::temporary);
            return;
        }

        case decoration_to_mode_colliding: {
            auto decoration_id = to_id(stack.pop_decoration_to_mode_colliding(), circuit);
            editing::change_decoration_insertion_mode(circuit, decoration_id,
                                                      InsertionMode::collisions);
            return;
        }

        case decoration_to_mode_insert: {
            auto decoration_id = to_id(stack.pop_decoration_to_mode_insert(), circuit);
            editing::change_decoration_insertion_mode(circuit, decoration_id,
                                                      InsertionMode::insert_or_discard);
            return;
        }

        case decoration_change_attributes: {
            auto [decoration_key, attrs] = stack.pop_decoration_change_attributes();
            const auto decoration_id = to_id(decoration_key, circuit);

            editing::set_attributes_decoration(circuit, decoration_id, std::move(attrs));
            return;
        }

        case decoration_add_visible_selection: {
            const auto decoration_id =
                to_id(stack.pop_decoration_add_visible_selection(), circuit);
            editing::add_to_visible_selection(circuit, decoration_id);
            return;
        }

        case decoration_remove_visible_selection: {
            const auto decoration_id =
                to_id(stack.pop_decoration_remove_visible_selection(), circuit);
            editing::remove_from_visible_selection(circuit, decoration_id);
            return;
        }

        case visible_selection_clear: {
            stack.pop_visible_selection_clear();
            editing::clear_visible_selection(circuit);
            return;
        }

        case visible_selection_set: {
            const auto stable_selection = stack.pop_visible_selection_set();
            auto selection = to_selection(stable_selection, circuit.index.key_index());
            editing::set_visible_selection(circuit, std::move(selection));
            return;
        }

        case visible_selection_add_operation: {
            const auto operation = stack.pop_visible_selection_add_operation();
            editing::add_visible_selection_rect(circuit, operation.function,
                                                operation.rect);
            return;
        }

        case visible_selection_update_last: {
            const auto rect = stack.pop_visible_selection_update_last();
            editing::update_last_visible_selection_rect(circuit, rect);
            return;
        }

        case visible_selection_pop_last: {
            stack.pop_visible_selection_pop_last();
            editing::pop_last_visible_selection_rect(circuit);
            return;
        }
    };
    std::terminate();
}

auto _replay_one_group(CircuitData& circuit, HistoryStack& stack) -> void {
    Expects(stack.top_entry() == HistoryEntry::new_group);
    _replay_last_entry(circuit, stack);

    while (has_ungrouped_entries(stack)) {
        _replay_last_entry(circuit, stack);
    }
}

enum class ReplayStack { undo, redo };

auto _replay_stack(CircuitData& circuit, ReplayStack kind) -> void {
    if (circuit.history.state != HistoryState::track_undo_new) [[unlikely]] {
        throw std::runtime_error("history is in wrong state");
    }
    if (has_ungrouped_entries(circuit.history.undo_stack) ||
        has_ungrouped_entries(circuit.history.redo_stack)) [[unlikely]] {
        throw std::runtime_error("stack needs to have finished group");
    }

    auto& replay_stack = kind == ReplayStack::undo ? circuit.history.undo_stack  //
                                                   : circuit.history.redo_stack;
    if (replay_stack.empty()) {
        return;
    }

    const auto _ =
        gsl::finally([&]() { circuit.history.state = HistoryState::track_undo_new; });
    circuit.history.state = kind == ReplayStack::undo ? HistoryState::track_redo_replay
                                                      : HistoryState::track_undo_replay;

    _replay_one_group(circuit, replay_stack);
    _store_history_new_group(circuit.history);

    Ensures(!has_ungrouped_entries(circuit.history.undo_stack));
    Ensures(!has_ungrouped_entries(circuit.history.redo_stack));
}

}  // namespace

auto undo_group(CircuitData& circuit) -> void {
    _replay_stack(circuit, ReplayStack::undo);
}

auto redo_group(CircuitData& circuit) -> void {
    _replay_stack(circuit, ReplayStack::redo);
}

auto finish_undo_group(History& history) -> void {
    if (history.state != HistoryState::track_undo_new) {
        return;
    }
    // Clear redo stack when a new group has been added to the undo stack
    if (history.undo_stack.push_new_group()) {
        history.redo_stack.clear();
    }
}

auto reopen_undo_group(History& history) -> void {
    return reopen_group(history.undo_stack);
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
