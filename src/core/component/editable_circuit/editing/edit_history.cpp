#include "core/component/editable_circuit/editing/edit_history.h"

#include "core/algorithm/vector_operations.h"
#include "core/component/editable_circuit/circuit_data.h"
#include "core/component/editable_circuit/editing/edit_decoration.h"
#include "core/component/editable_circuit/editing/edit_visible_selection.h"
#include "core/component/editable_circuit/history.h"
#include "core/logging.h"

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
    return !history.undo_stack.entries.empty() &&
           history.undo_stack.entries.back() != HistoryEntry::new_group;
}

auto has_ungrouped_redo_entries(const History& history) -> bool {
    return !history.redo_stack.entries.empty() &&
           history.redo_stack.entries.back() != HistoryEntry::new_group;
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

auto _apply_last_entry(CircuitData& circuit, HistoryStack& stack) -> void {
    switch (at_back_vector(stack.entries)) {
        using enum HistoryEntry;
        case new_group: {
            return;
        }

        case decoration_create_temporary: {
            auto [decoration_key, placed_decoration] =
                stack.pop_decoration_create_temporary();

            // TODO fix std::move
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
            auto [decoration_key, placed_decoration] =
                stack.pop_decoration_change_attributes();
            const auto decoration_id = to_id(decoration_key, circuit);

            if (placed_decoration.definition.attrs_text_element.has_value()) {
                editing::set_attributes_decoration(
                    circuit, decoration_id,
                    std::move(placed_decoration.definition.attrs_text_element.value()));
            }
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

        case visible_selection_add: {
            const auto operation = stack.pop_visible_selection_add();
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

auto _apply_last_group(CircuitData& circuit, HistoryStack& stack) -> void {
    while (!stack.entries.empty() && stack.entries.back() == HistoryEntry::new_group) {
        stack.entries.pop_back();
    }

    while (!stack.entries.empty() && stack.entries.back() != HistoryEntry::new_group) {
        _apply_last_entry(circuit, stack);
    }
}

}  // namespace

auto undo_group(CircuitData& circuit) -> void {
    const auto _ = gsl::finally([&, initial_state = circuit.history.state]() {
        circuit.history.state = initial_state;
    });
    circuit.history.state = HistoryState::track_redo_replay;

    _apply_last_group(circuit, circuit.history.undo_stack);
    finish_redo_group(circuit.history);

    Ensures(!has_ungrouped_undo_entries(circuit.history));
    Ensures(!has_ungrouped_redo_entries(circuit.history));
}

auto redo_group(CircuitData& circuit) -> void {
    const auto _ = gsl::finally([&, initial_state = circuit.history.state]() {
        circuit.history.state = initial_state;
    });
    circuit.history.state = HistoryState::track_undo_replay;

    _apply_last_group(circuit, circuit.history.redo_stack);
    finish_undo_group(circuit.history);

    Ensures(!has_ungrouped_undo_entries(circuit.history));
    Ensures(!has_ungrouped_redo_entries(circuit.history));
}

auto finish_undo_group(History& history) -> void {
    if (has_ungrouped_undo_entries(history)) {
        history.undo_stack.entries.emplace_back(HistoryEntry::new_group);
    }
}

auto finish_redo_group(History& history) -> void {
    if (has_ungrouped_redo_entries(history)) {
        history.redo_stack.entries.emplace_back(HistoryEntry::new_group);
    }
}

auto reopen_undo_group(History& history) -> void {
    auto& entries = history.undo_stack.entries;

    while (!entries.empty() && entries.back() == HistoryEntry::new_group) {
        entries.pop_back();
    }
}

auto reopen_redo_group(History& history) -> void {
    auto& entries = history.redo_stack.entries;

    while (!entries.empty() && entries.back() == HistoryEntry::new_group) {
        entries.pop_back();
    }
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
