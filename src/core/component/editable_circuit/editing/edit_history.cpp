#include "core/component/editable_circuit/editing/edit_history.h"

#include "core/component/editable_circuit/circuit_data.h"
#include "core/component/editable_circuit/editing/edit_decoration.h"
#include "core/component/editable_circuit/editing/edit_logicitem.h"
#include "core/component/editable_circuit/editing/edit_visible_selection.h"
#include "core/component/editable_circuit/editing/edit_wire.h"
#include "core/component/editable_circuit/history.h"
#include "core/geometry/segment_info.h"
#include "core/timer.h"

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

auto to_id(logicitem_key_t logicitem_key, CircuitData& circuit) -> logicitem_id_t {
    return circuit.index.key_index().get(logicitem_key);
}

auto to_id(segment_key_t segment_key, CircuitData& circuit) -> segment_t {
    return circuit.index.key_index().get(segment_key);
}

auto to_part(segment_key_t segment_key, CircuitData& circuit) -> segment_part_t {
    return get_segment_part(circuit.layout, to_id(segment_key, circuit));
}

auto to_part(std::pair<segment_key_t, part_t> value,
             CircuitData& circuit) -> segment_part_t {
    return segment_part_t {to_id(value.first, circuit), value.second};
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

            //
            // Logic Item
            //

        case logicitem_create_temporary: {
            auto [logicitem_key, placed_logicitem] =
                stack.pop_logicitem_create_temporary();

            editing::add_logicitem(circuit, std::move(placed_logicitem.definition),
                                   placed_logicitem.position, InsertionMode::temporary,
                                   logicitem_key);
            return;
        }

        case logicitem_delete_temporary: {
            auto logicitem_id = to_id(stack.pop_logicitem_delete_temporary(), circuit);
            editing::delete_temporary_logicitem(circuit, logicitem_id);
            return;
        }

        case logicitem_move_temporary: {
            const auto [logicitem_key, delta] = stack.pop_logicitem_move_temporary();
            const auto logicitem_id = to_id(logicitem_key, circuit);

            editing::move_temporary_logicitem_unchecked(circuit, logicitem_id, delta);
            return;
        }

        case logicitem_to_mode_temporary: {
            auto logicitem_id = to_id(stack.pop_logicitem_to_mode_temporary(), circuit);
            editing::change_logicitem_insertion_mode(circuit, logicitem_id,
                                                     InsertionMode::temporary);
            return;
        }

        case logicitem_to_mode_colliding: {
            auto logicitem_id = to_id(stack.pop_logicitem_to_mode_colliding(), circuit);
            editing::change_logicitem_insertion_mode(circuit, logicitem_id,
                                                     InsertionMode::collisions);
            return;
        }

        case logicitem_to_mode_insert: {
            auto logicitem_id = to_id(stack.pop_logicitem_to_mode_insert(), circuit);
            editing::change_logicitem_insertion_mode(circuit, logicitem_id,
                                                     InsertionMode::insert_or_discard);
            return;
        }

        case logicitem_change_attributes: {
            auto [logicitem_key, attrs] = stack.pop_logicitem_change_attributes();
            const auto logicitem_id = to_id(logicitem_key, circuit);

            editing::set_attributes_logicitem(circuit, logicitem_id, std::move(attrs));
            return;
        }

        case logicitem_add_visible_selection: {
            const auto logicitem_id =
                to_id(stack.pop_logicitem_add_visible_selection(), circuit);
            editing::add_to_visible_selection(circuit, logicitem_id);
            return;
        }

        case logicitem_remove_visible_selection: {
            const auto logicitem_id =
                to_id(stack.pop_logicitem_remove_visible_selection(), circuit);
            editing::remove_from_visible_selection(circuit, logicitem_id);
            return;
        }

            //
            // Decoration
            //

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

            //
            // Segment
            //

        case segment_create_temporary: {
            const auto [segment_key, line] = stack.pop_segment_create_temporary();
            editing::add_wire_segment(circuit, line, InsertionMode::temporary,
                                      segment_key);
            return;
        }

        case segment_delete_temporary: {
            auto segment_part = to_part(stack.pop_segment_delete_temporary(), circuit);
            editing::delete_temporary_wire_segment(circuit, segment_part);
            return;
        }

        case segment_move_temporary: {
            const auto [segment_key, delta] = stack.pop_segment_move_temporary();
            const auto segment = to_id(segment_key, circuit);
            editing::move_temporary_wire_unchecked(circuit, segment, delta);
            return;
        }

        case segment_to_mode_temporary: {
            auto segment_part = to_part(stack.pop_segment_to_mode_temporary(), circuit);
            editing::change_wire_insertion_mode(circuit, segment_part,
                                                InsertionMode::temporary);
            return;
        }

        case segment_to_mode_colliding: {
            auto segment_part = to_part(stack.pop_segment_to_mode_colliding(), circuit);
            editing::change_wire_insertion_mode(circuit, segment_part,
                                                InsertionMode::collisions);
            return;
        }

        case segment_to_mode_insert: {
            auto segment_part = to_part(stack.pop_segment_to_mode_insert(), circuit);
            editing::change_wire_insertion_mode(circuit, segment_part,
                                                InsertionMode::insert_or_discard);
            return;
        }

        case segment_set_endpoints: {
            const auto [segment_key, endpoints] = stack.pop_segment_set_endpoints();
            const auto segment = to_id(segment_key, circuit);
            editing::set_uninserted_endpoints_with_history(circuit, segment, endpoints);
            return;
        }

        case segment_merge: {
            const auto [segment_key_0, segment_key_1] = stack.pop_segment_merge();
            const auto segment_0 = to_id(segment_key_0, circuit);
            const auto segment_1 = to_id(segment_key_1, circuit);
            editing::merge_uninserted_segment_with_history(circuit, segment_0, segment_1);
            return;
        }

        case segment_split: {
            const auto definition = stack.pop_segment_split();
            editing::split_uninserted_segment(circuit,
                                              split_segment_t {
                                                  .source_key = definition.source,
                                                  .new_key = definition.new_key,
                                                  .split_offset = definition.split_offset,
                                              });
            return;
        }

        case segment_add_visible_selection: {
            stack.pop_segment_add_visible_selection();
            return;
        }

        case segment_remove_visible_selection: {
            stack.pop_segment_remove_visible_selection();
            return;
        }

            //
            // Visible Selection
            //

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

    auto& replay_stack = kind == ReplayStack::undo ? circuit.history.undo_stack
                                                   : circuit.history.redo_stack;
    if (replay_stack.empty()) {
        return;
    }

    {
        const auto _ = gsl::finally([&]() {  //
            circuit.history.state = HistoryState::track_undo_new;
        });
        circuit.history.state = kind == ReplayStack::undo
                                    ? HistoryState::track_redo_replay
                                    : HistoryState::track_undo_replay;

        _replay_one_group(circuit, replay_stack);
        _store_history_new_group(circuit.history);
    }

    Ensures(!has_ungrouped_entries(circuit.history.undo_stack));
    Ensures(!has_ungrouped_entries(circuit.history.redo_stack));
    Ensures(circuit.history.state == HistoryState::track_undo_new);
}

}  // namespace

auto undo_group(CircuitData& circuit) -> void {
    const auto _ = Timer {"undo", Timer::Unit::ms};
    finish_undo_group(circuit.history);
    _replay_stack(circuit, ReplayStack::undo);
}

auto redo_group(CircuitData& circuit) -> void {
    const auto _ = Timer {"redo", Timer::Unit::ms};
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
