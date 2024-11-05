#include "core/component/editable_circuit/editing/edit_visible_selection.h"

#include "core/algorithm/vector_operations.h"
#include "core/component/editable_circuit/circuit_data.h"

namespace logicsim {

namespace editable_circuit {

namespace editing {

namespace {

auto _store_history_visible_selection_set_operations(
    CircuitHistory& history, const VisibleSelection& visible_selection) -> void {
    if (const auto stack = history.get_stack()) {
        for (const auto& operation :
             visible_selection.operations() | std::ranges::views::reverse) {
            stack->push_visible_selection_add(operation);
        }
    }
}

auto _store_history_visible_selection_set(CircuitHistory& history,
                                          const VisibleSelection& visible_selection,
                                          const KeyIndex& key_index) -> void {
    if (const auto stack = history.get_stack()) {
        if (const auto& initial_selection = visible_selection.initial_selection();
            !initial_selection.empty()) {
            stack->push_visible_selection_set(
                to_stable_selection(initial_selection, key_index));
        } else {
            stack->push_visible_selection_clear();
        }
    }
}

auto _store_history_visible_selection_pop_last(CircuitHistory& history) -> void {
    if (const auto stack = history.get_stack()) {
        stack->push_visible_selection_pop_last();
    }
}

auto _store_history_visible_selection_add(CircuitData& circuit_data) -> void {
    static_cast<void>(circuit_data);
    if (const auto stack = circuit_data.history.get_stack()) {
        const auto operation = last_operation(circuit_data.visible_selection).value();
        stack->push_visible_selection_add(operation);
    }
}

auto _store_history_visible_selection_update_last(CircuitData& circuit_data) -> void {
    static_cast<void>(circuit_data);
    if (const auto stack = circuit_data.history.get_stack()) {
        const auto operation = last_operation(circuit_data.visible_selection).value();
        stack->push_visible_selection_update_last(operation.rect);
    }
}

}  // namespace

auto clear_visible_selection(CircuitData& circuit_data) -> void {
    set_visible_selection(circuit_data, Selection {});
}

auto set_visible_selection(CircuitData& circuit_data, Selection&& selection_) -> void {
    if (!is_valid_selection(selection_, circuit_data.layout)) {
        throw std::runtime_error("Selection contains elements not in layout");
    }

    _store_history_visible_selection_set_operations(circuit_data.history,
                                                    circuit_data.visible_selection);

    if (circuit_data.visible_selection.initial_selection() != selection_) {
        _store_history_visible_selection_set(circuit_data.history,
                                             circuit_data.visible_selection,
                                             circuit_data.index.key_index());
    }

    circuit_data.visible_selection = VisibleSelection {std::move(selection_)};
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
    if (circuit_data.visible_selection.operations().empty()) {
        return;
    }

    auto selection = Selection {circuit_data.visible_selection.selection(
        circuit_data.layout, circuit_data.index)};

    set_visible_selection(circuit_data, std::move(selection));
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
