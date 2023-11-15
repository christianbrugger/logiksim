#include "circuit_widget_base.h"

namespace logicsim {

auto CircuitWidgetBase::emit_render_config_changed(WidgetRenderConfig new_config)
    -> void {
    Q_EMIT render_config_changed(new_config);
}

auto CircuitWidgetBase::emit_simulation_config_changed(SimulationConfig new_config)
    -> void {
    Q_EMIT simulation_config_changed(new_config);
}

auto CircuitWidgetBase::emit_circuit_state_changed(CircuitWidgetState new_state) -> void {
    Q_EMIT circuit_state_changed(new_state);
}

}  // namespace logicsim
