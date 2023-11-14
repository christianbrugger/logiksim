#include "circuit_widget_base.h"

namespace logicsim {

namespace circuit_widget {

auto is_simulation(const CircuitState& state) -> bool {
    return std::holds_alternative<SimulationState>(state);
}

}  // namespace circuit_widget

auto CircuitWidgetBase::emit_render_config_changed(RenderConfig new_config) -> void {
    emit render_config_changed(new_config);
}

auto CircuitWidgetBase::emit_simulation_config_changed(SimulationConfig new_config)
    -> void {
    emit simulation_config_changed(new_config);
}

auto CircuitWidgetBase::emit_circuit_state_changed(CircuitState new_state) -> void {
    emit circuit_state_changed(new_state);
}

}  // namespace logicsim
