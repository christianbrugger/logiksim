#include "circuit_widget.h"

#include "logging.h"
#include "vocabulary/simulation_config.h"
#include "vocabulary/widget_render_config.h"

namespace logicsim {

CircuitWidget::CircuitWidget(QWidget* parent) : CircuitWidgetBase(parent) {}

auto CircuitWidget::set_render_config(WidgetRenderConfig new_config) -> void {
    if (render_config_ == new_config) {
        return;
    }

    render_config_ = new_config;
    emit_render_config_changed(new_config);
    print(render_config_);
}

auto CircuitWidget::set_simulation_config(SimulationConfig new_config) -> void {
    if (simulation_config_ == new_config) {
        return;
    }

    simulation_config_ = new_config;
    emit_simulation_config_changed(new_config);
    print(simulation_config_);
}

auto CircuitWidget::set_circuit_state(CircuitWidgetState new_state) -> void {
    if (circuit_state_ == new_state) {
        return;
    }

    circuit_state_ = new_state;
    emit_circuit_state_changed(new_state);
    print(circuit_state_);
}

auto CircuitWidget::render_config() const -> WidgetRenderConfig {
    return render_config_;
}

auto CircuitWidget::simulation_config() const -> SimulationConfig {
    return simulation_config_;
}

auto CircuitWidget::circuit_state() const -> CircuitWidgetState {
    return circuit_state_;
}

auto CircuitWidget::serialized_circuit() const -> std::string {
    // TODO implement
    return std::string();
}

auto CircuitWidget::load_new_circuit() -> void {
    // TODO implement
}

auto CircuitWidget::load_circuit_example(int) -> void {
    // TODO implement
}

auto CircuitWidget::load_circuit(std::string filename) -> bool {
    // TODO implement
    return true;
}

auto CircuitWidget::save_circuit(std::string filename) -> bool {
    // TODO implement
    return true;
}

auto CircuitWidget::reload_circuit() -> void {
    // TODO implement
}

auto CircuitWidget::statistics() const -> Statistics {
    // TODO implement
    return Statistics {};
}

auto CircuitWidget::submit_user_action(UserAction action) -> void {
    // TODO implement
}

//
// Free Functions
//

auto set_do_benchmark(CircuitWidget& circuit_widget, bool value) -> void {
    auto config = circuit_widget.render_config();
    config.do_benchmark = value;
    circuit_widget.set_render_config(config);
}

auto set_show_circuit(CircuitWidget& circuit_widget, bool value) -> void {
    auto config = circuit_widget.render_config();
    config.show_circuit = value;
    circuit_widget.set_render_config(config);
}

auto set_show_collision_cache(CircuitWidget& circuit_widget, bool value) -> void {
    auto config = circuit_widget.render_config();
    config.show_collision_cache = value;
    circuit_widget.set_render_config(config);
}

auto set_show_connection_cache(CircuitWidget& circuit_widget, bool value) -> void {
    auto config = circuit_widget.render_config();
    config.show_connection_cache = value;
    circuit_widget.set_render_config(config);
}

auto set_show_selection_cache(CircuitWidget& circuit_widget, bool value) -> void {
    auto config = circuit_widget.render_config();
    config.show_selection_cache = value;
    circuit_widget.set_render_config(config);
}

auto set_thread_count(CircuitWidget& circuit_widget, int new_count) -> void {
    auto config = circuit_widget.render_config();
    config.thread_count = new_count;
    circuit_widget.set_render_config(config);
}

auto set_direct_rendering(CircuitWidget& circuit_widget, bool use_store) -> void {
    auto config = circuit_widget.render_config();
    config.direct_rendering = use_store;
    circuit_widget.set_render_config(config);
}

auto set_simulation_time_rate(CircuitWidget& circuit_widget, time_rate_t new_rate)
    -> void {
    auto config = circuit_widget.simulation_config();
    config.simulation_time_rate = new_rate;
    circuit_widget.set_simulation_config(config);
}

auto set_use_wire_delay(CircuitWidget& circuit_widget, bool value) -> void {
    auto config = circuit_widget.simulation_config();
    config.use_wire_delay = value;
    circuit_widget.set_simulation_config(config);
}

}  // namespace logicsim
