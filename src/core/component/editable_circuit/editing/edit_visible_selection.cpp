#include "core/component/editable_circuit/editing/edit_visible_selection.h"

#include "core/component/editable_circuit/circuit_data.h"

namespace logicsim {

namespace editable_circuit {

namespace editing {

namespace {

auto _store_history_visible_selection_set(History& history,
                                          const VisibleSelection& visible_selection,
                                          const KeyIndex& key_index,
                                          const Selection& new_selection) -> void {
    if (const auto stack = history.get_stack()) {
        for (const auto& operation :
             visible_selection.operations() | std::ranges::views::reverse) {
            stack->push_visible_selection_add_operation(operation);
        }

        const auto& selection = visible_selection.initial_selection();

        if (selection == new_selection) {
            return;
        }

        // remove selection
        if (selection.empty()) {
            // remove single entry
            if (const auto logicitem_id = get_single_logicitem(new_selection)) {
                const auto logicitem_key = key_index.get(logicitem_id);
                stack->push_logicitem_remove_visible_selection(logicitem_key);
                return;
            }
            if (const auto decoration_id = get_single_decoration(new_selection)) {
                const auto decoration_key = key_index.get(decoration_id);
                stack->push_decoration_remove_visible_selection(decoration_key);
                return;
            }

            stack->push_visible_selection_clear();
            return;
        }

        if (new_selection.empty()) {
            // add single entry
            if (const auto logicitem_id = get_single_logicitem(selection)) {
                const auto logicitem_key = key_index.get(logicitem_id);
                stack->push_logicitem_add_visible_selection(logicitem_key);
                return;
            }
            if (const auto decoration_id = get_single_decoration(selection)) {
                const auto decoration_key = key_index.get(decoration_id);
                stack->push_decoration_add_visible_selection(decoration_key);
                return;
            }
        }

        // set to previous selection
        stack->push_visible_selection_set(to_stable_selection(selection, key_index));
    }
}

auto _store_history_visible_selection_pop_last(History& history) -> void {
    if (const auto stack = history.get_stack()) {
        stack->push_visible_selection_pop_last();
    }
}

auto _store_history_visible_selection_add_operation(CircuitData& circuit_data) -> void {
    static_cast<void>(circuit_data);
    if (const auto stack = circuit_data.history.get_stack()) {
        const auto operation = last_operation(circuit_data.visible_selection).value();
        stack->push_visible_selection_add_operation(operation);
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

auto set_visible_selection(CircuitData& circuit_data, Selection&& selection) -> void {
    if (!is_valid_selection(selection, circuit_data.layout)) {
        throw std::runtime_error("Selection contains elements not in layout");
    }

    _store_history_visible_selection_set(circuit_data.history,
                                         circuit_data.visible_selection,
                                         circuit_data.index.key_index(), selection);

    circuit_data.visible_selection = VisibleSelection {std::move(selection)};
}

auto add_visible_selection_rect(CircuitData& circuit_data, SelectionFunction function,
                                rect_fine_t rect) -> void {
    _store_history_visible_selection_pop_last(circuit_data.history);

    circuit_data.visible_selection.add(function, rect);
}

auto pop_last_visible_selection_rect(CircuitData& circuit_data) -> void {
    _store_history_visible_selection_add_operation(circuit_data);

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
