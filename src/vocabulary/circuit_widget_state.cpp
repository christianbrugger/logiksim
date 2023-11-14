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

auto is_simulation(const CircuitWidgetState& state) -> bool {
    return std::holds_alternative<SimulationState>(state);
}

}  // namespace logicsim
