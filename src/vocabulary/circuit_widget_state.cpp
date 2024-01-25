#include "vocabulary/circuit_widget_state.h"

namespace logicsim {

auto SimulationState::format() const -> std::string {
    return "SimulationState";
}

auto NonInteractiveState::format() const -> std::string {
    return "NonInteractiveState";
}

auto EditingState::format() const -> std::string {
    return fmt::format("EditingState({})", default_mouse_action);
}

auto is_simulation(const CircuitWidgetState &state) -> bool {
    return std::holds_alternative<SimulationState>(state);
}

auto is_non_interactive(const CircuitWidgetState &state) -> bool {
    return std::holds_alternative<NonInteractiveState>(state);
}

auto is_editing_state(const CircuitWidgetState &state) -> bool {
    return std::holds_alternative<EditingState>(state);
}

//
// EditingState
//

auto is_insert_logic_item_state(const EditingState &editing_state) -> bool {
    return is_insert_logic_item_state(editing_state.default_mouse_action);
}

auto is_insert_wire_state(const EditingState &editing_state) -> bool {
    return editing_state.default_mouse_action == DefaultMouseAction::insert_wire;
}

auto is_selection_state(const EditingState &editing_state) -> bool {
    return editing_state.default_mouse_action == DefaultMouseAction::selection;
}

auto is_inserting_state(const EditingState &editing_state) -> bool {
    return is_inserting_state(editing_state.default_mouse_action);
}

//
// CircuitWidgetState
//

auto is_insert_logic_item_state(const CircuitWidgetState &state) -> bool {
    const auto editing_state = std::get_if<EditingState>(&state);

    return editing_state && is_insert_logic_item_state(*editing_state);
}

auto is_insert_wire_state(const CircuitWidgetState &state) -> bool {
    const auto editing_state = std::get_if<EditingState>(&state);

    return editing_state && is_insert_wire_state(*editing_state);
}

auto is_selection_state(const CircuitWidgetState &state) -> bool {
    const auto editing_state = std::get_if<EditingState>(&state);

    return editing_state && is_selection_state(*editing_state);
}

auto is_inserting_state(const CircuitWidgetState &state) -> bool {
    const auto editing_state = std::get_if<EditingState>(&state);

    return editing_state && is_inserting_state(*editing_state);
}

}  // namespace logicsim
