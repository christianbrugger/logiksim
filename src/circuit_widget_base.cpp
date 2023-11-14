#include "circuit_widget_base.h"


namespace logicsim {

auto CircuitWidgetBase::emit_render_config_changed(WidgetRenderConfig new_config) -> void {
    emit render_config_changed(new_config);
}

auto CircuitWidgetBase::emit_simulation_config_changed(SimulationConfig new_config)
    -> void {
    emit simulation_config_changed(new_config);
}

auto CircuitWidgetBase::emit_circuit_state_changed(CircuitWidgetState new_state) -> void {
    emit circuit_state_changed(new_state);
}

}  // namespace logicsim
