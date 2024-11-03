#ifndef LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_VISIBLE_SELECTION_H
#define LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_VISIBLE_SELECTION_H

#include "core/vocabulary/selection_function.h"

namespace logicsim {

class Selection;
class VisibleSelection;
struct rect_fine_t;

namespace editable_circuit {

struct CircuitData;

namespace editing {

auto clear_visible_selection(CircuitData& circuit_data) -> void;

auto set_visible_selection(CircuitData& circuit_data, Selection&& selection_) -> void;

auto add_visible_selection_rect(CircuitData& circuit_data, SelectionFunction function,
                                rect_fine_t rect) -> void;

auto pop_last_visible_selection_rect(CircuitData& circuit_data) -> void;

auto update_last_visible_selection_rect(CircuitData& circuit_data,
                                        rect_fine_t rect) -> void;

auto apply_all_visible_selection_operations(CircuitData& circuit_data) -> void;

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim

#endif
