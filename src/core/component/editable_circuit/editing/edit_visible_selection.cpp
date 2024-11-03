#include "core/component/editable_circuit/editing/edit_visible_selection.h"

#include "core/component/editable_circuit/circuit_data.h"

namespace logicsim {

namespace editable_circuit {

namespace editing {

auto clear_visible_selection(CircuitData& circuit_data) -> void {
    circuit_data.visible_selection.clear();
}

auto set_visible_selection(CircuitData& circuit_data, Selection selection_) -> void {
    if (!is_valid_selection(selection_, circuit_data.layout)) {
        throw std::runtime_error("Selection contains elements not in layout");
    }

    circuit_data.visible_selection.set_selection(std::move(selection_));
}

auto add_visible_selection_rect(CircuitData& circuit_data, SelectionFunction function,
                                rect_fine_t rect) -> void {
    circuit_data.visible_selection.add(function, rect);
}

auto try_pop_last_visible_selection_rect(CircuitData& circuit_data) -> bool {
    if (circuit_data.visible_selection.operation_count() == std::size_t {0}) {
        return false;
    }
    circuit_data.visible_selection.pop_last();

    return true;
}

auto try_update_last_visible_selection_rect(CircuitData& circuit_data,
                                            rect_fine_t rect) -> bool {
    if (circuit_data.visible_selection.operation_count() == std::size_t {0}) {
        return false;
    }
    circuit_data.visible_selection.update_last(rect);

    return true;
}

auto apply_all_visible_selection_operations(CircuitData& circuit_data) -> void {
    circuit_data.visible_selection.apply_all_operations(circuit_data.layout,
                                                        circuit_data.index);
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
