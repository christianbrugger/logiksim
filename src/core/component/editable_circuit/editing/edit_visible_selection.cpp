#include "core/component/editable_circuit/editing/edit_visible_selection.h"

#include "core/component/editable_circuit/circuit_data.h"

namespace logicsim {

namespace editable_circuit {

namespace editing {

namespace {

auto _store_history_visible_selection_set(CircuitHistory& history,
                                          VisibleSelection&& visible_selection) -> void {
    if (const auto& stack = history.get_stack()) {
        visible_selection.clear_cache();

        stack->entries.emplace_back(HistoryEntry::visible_selection_set);
        stack->visible_selections.emplace_back(std::move(visible_selection));
    }
}

auto _store_history_visible_selection_pop_last(CircuitHistory& history) -> void {
    if (const auto& stack = history.get_stack()) {
        stack->entries.emplace_back(HistoryEntry::visible_selection_pop_last);
    }
}

auto _store_history_visible_selection_add(CircuitData& circuit_data) -> void {
    if (const auto& stack = circuit_data.history.get_stack()) {
        const auto operation = circuit_data.visible_selection.last_opereration().value();

        stack->entries.emplace_back(HistoryEntry::visible_selection_add);
        stack->selection_functions.emplace_back(operation.function);
        stack->selection_rects.emplace_back(operation.rect);
    }
}

auto _store_history_visible_selection_update_last(CircuitData& circuit_data) -> void {
    if (const auto& stack = circuit_data.history.get_stack()) {
        // skip similar changes
        if (last_non_group_entry(stack->entries) ==
            HistoryEntry::visible_selection_update_last) {
            return;
        }

        const auto operation = circuit_data.visible_selection.last_opereration().value();
        stack->entries.emplace_back(HistoryEntry::visible_selection_update_last);
        stack->selection_rects.emplace_back(operation.rect);
    }
}

}  // namespace

auto clear_visible_selection(CircuitData& circuit_data) -> void {
    _store_history_visible_selection_set(circuit_data.history,
                                         std::move(circuit_data.visible_selection));
    circuit_data.visible_selection = VisibleSelection {};
}

auto set_visible_selection(CircuitData& circuit_data, Selection selection_) -> void {
    set_visible_selection(circuit_data, VisibleSelection {std::move(selection_)});
}

auto set_visible_selection(CircuitData& circuit_data,
                           VisibleSelection&& visible_selection_) -> void {
    if (!is_valid_selection(visible_selection_.initial_selection(),
                            circuit_data.layout)) {
        throw std::runtime_error("Selection contains elements not in layout");
    }

    _store_history_visible_selection_set(circuit_data.history,
                                         std::move(circuit_data.visible_selection));
    circuit_data.visible_selection = std::move(visible_selection_);
}

auto add_visible_selection_rect(CircuitData& circuit_data, SelectionFunction function,
                                rect_fine_t rect) -> void {
    _store_history_visible_selection_pop_last(circuit_data.history);
    circuit_data.visible_selection.add(function, rect);
}

auto pop_last_visible_selection_rect(CircuitData& circuit_data) -> void {
    _store_history_visible_selection_add(circuit_data);
    circuit_data.visible_selection.pop_last();
}

auto update_last_visible_selection_rect(CircuitData& circuit_data,
                                        rect_fine_t rect) -> void {
    _store_history_visible_selection_update_last(circuit_data);
    circuit_data.visible_selection.update_last(rect);
}

auto apply_all_visible_selection_operations(CircuitData& circuit_data) -> void {
    _store_history_visible_selection_set(
        circuit_data.history, VisibleSelection {circuit_data.visible_selection});

    circuit_data.visible_selection.apply_all_operations(circuit_data.layout,
                                                        circuit_data.index);
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
