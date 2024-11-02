#include "core/component/editable_circuit/editing/edit_history.h"

#include "core/component/editable_circuit/circuit_data.h"
#include "core/component/editable_circuit/editing/edit_decoration.h"
#include "core/component/editable_circuit/history.h"
#include "core/logging.h"

#include <gsl/gsl>

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
    return !history.undo_stack.empty() &&
           history.undo_stack.back().type != UndoType::new_group;
}

auto enable_history(CircuitHistory& history) -> void {
    history.state = HistoryState::track_undo;
}

namespace {

auto _undo_last_entry(CircuitData& circuit) -> void {
    assert(circuit.history.state == HistoryState::track_redo);

    Expects(!circuit.history.undo_stack.empty());
    switch (circuit.history.undo_stack.back().type) {
        using enum UndoType;
        case new_group: {
            break;
        }

        case create_temporary_element: {
            Expects(!circuit.history.placed_decoration_stack.empty());
            auto decoration = std::move(circuit.history.placed_decoration_stack.back());
            circuit.history.placed_decoration_stack.pop_back();

            // TODO fix std::move
            const auto decoration_id =
                editing::add_decoration(circuit, std::move(decoration.definition),
                                        decoration.position, InsertionMode::temporary);

            Expects(!circuit.history.undo_stack.empty());
            circuit.index.set_key(decoration_id, circuit.history.undo_stack.back().key);
            break;
        }

        case delete_temporary_element: {
            auto decoration_id =
                circuit.index.key_index().get(circuit.history.undo_stack.back().key);
            editing::delete_temporary_decoration(circuit, decoration_id);
            break;
        }

        case move_temporary_element: {
            Expects(!circuit.history.move_delta_stack.empty());
            const auto [x, y] = circuit.history.move_delta_stack.back();
            circuit.history.move_delta_stack.pop_back();

            const auto decoration_id =
                circuit.index.key_index().get(circuit.history.undo_stack.back().key);

            editing::move_temporary_decoration_unchecked(circuit, decoration_id, x, y);
            break;
        }

        case to_insertion_temporary: {
            auto decoration_id =
                circuit.index.key_index().get(circuit.history.undo_stack.back().key);
            editing::change_decoration_insertion_mode(circuit, decoration_id,
                                                      InsertionMode::temporary);
            break;
        }

        case to_insertion_colliding: {
            auto decoration_id =
                circuit.index.key_index().get(circuit.history.undo_stack.back().key);
            editing::change_decoration_insertion_mode(circuit, decoration_id,
                                                      InsertionMode::collisions);
            break;
        }

        case to_insertion_insert: {
            auto decoration_id =
                circuit.index.key_index().get(circuit.history.undo_stack.back().key);
            editing::change_decoration_insertion_mode(circuit, decoration_id,
                                                      InsertionMode::insert_or_discard);
            break;
        }

        case change_attributes: {
            Expects(!circuit.history.placed_decoration_stack.empty());
            auto decoration = std::move(circuit.history.placed_decoration_stack.back());
            circuit.history.placed_decoration_stack.pop_back();

            auto decoration_id =
                circuit.index.key_index().get(circuit.history.undo_stack.back().key);

            if (decoration.definition.attrs_text_element.has_value()) {
                editing::set_attributes_decoration(
                    circuit, decoration_id,
                    std::move(decoration.definition.attrs_text_element.value()));
            }
            break;
        }
    };

    circuit.history.undo_stack.pop_back();
}

}  // namespace

auto undo_group(CircuitData& circuit) -> void {
    const auto _ = gsl::finally([&, initial_state = circuit.history.state]() {
        circuit.history.state = initial_state;
    });
    circuit.history.state = HistoryState::track_redo;

    while (!circuit.history.undo_stack.empty() &&
           circuit.history.undo_stack.back().type == UndoType::new_group) {
        circuit.history.undo_stack.pop_back();
    }

    while (!circuit.history.undo_stack.empty() &&
           circuit.history.undo_stack.back().type != UndoType::new_group) {
        _undo_last_entry(circuit);
    }

    Ensures(!has_ungrouped_undo_entries(circuit.history));
}

auto redo_group(CircuitData& circuit) -> void {
    static_cast<void>(circuit);
    print("RUN REDO GROUP");
}

auto finish_undo_group(CircuitHistory& history) -> void {
    if (has_ungrouped_undo_entries(history)) {
        history.undo_stack.emplace_back(DecorationUndoEntry {
            .type = UndoType::new_group,
        });
    }
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
