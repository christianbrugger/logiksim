#include "component/circuit_widget/circuit_store.h"

#include "logging.h"
#include "timer.h"

namespace logicsim {

namespace circuit_widget {

auto CircuitStore::set_circuit_state(CircuitWidgetState new_state) -> void {
    Expects(is_simulation(circuit_state_) || interactive_simulation_.layout().empty());
    if (new_state == circuit_state_) {
        return;
    }

    if (is_simulation(circuit_state_)) {
        Expects(!is_simulation(new_state));
        interactive_simulation_ = InteractiveSimulation {};
    }

    if (is_simulation(new_state)) {
        Expects(!is_simulation(circuit_state_));
        const auto _ [[maybe_unused]] = Timer {"Generate simulation", Timer::Unit::ms, 3};

        interactive_simulation_ = InteractiveSimulation {
            Layout {editable_circuit_.layout()},
            simulation_config_.wire_delay_per_distance(),
            simulation_config_.simulation_time_rate,
        };
    }

    // update
    circuit_state_ = new_state;
    Ensures(is_simulation(circuit_state_) || interactive_simulation_.layout().empty());
}

auto CircuitStore::set_simulation_config(SimulationConfig new_config) -> void {
    Expects(is_simulation(circuit_state_) || interactive_simulation_.layout().empty());

    if (new_config == simulation_config_) {
        return;
    }

    // use_wire_delay
    if (is_simulation(circuit_state_) &&
        new_config.use_wire_delay != simulation_config_.use_wire_delay) {
        throw std::runtime_error(
            "Cannot change 'use_wire_delay' with running simulation");
    }
    // simulation_time_rate
    interactive_simulation_.set_simulation_time_rate(new_config.simulation_time_rate);

    // update
    simulation_config_ = new_config;
    Ensures(is_simulation(circuit_state_) || interactive_simulation_.layout().empty());
}

auto CircuitStore::layout() const -> const Layout& {
    Expects(is_simulation(circuit_state_) || interactive_simulation_.layout().empty());

    if (is_simulation(circuit_state_)) {
        return interactive_simulation_.layout();
    }
    return editable_circuit_.layout();
}

auto CircuitStore::editable_circuit() -> EditableCircuit* {
    Expects(is_simulation(circuit_state_) || interactive_simulation_.layout().empty());

    if (is_editing_state(circuit_state_)) {
        return &editable_circuit_;
    }
    return nullptr;
}

auto CircuitStore::editable_circuit() const -> const EditableCircuit* {
    Expects(is_simulation(circuit_state_) || interactive_simulation_.layout().empty());

    if (is_editing_state(circuit_state_)) {
        return &editable_circuit_;
    }
    return nullptr;
}

auto CircuitStore::interactive_simulation() -> InteractiveSimulation* {
    Expects(is_simulation(circuit_state_) || interactive_simulation_.layout().empty());

    if (is_simulation(circuit_state_)) {
        return &interactive_simulation_;
    }
    return nullptr;
}

auto CircuitStore::interactive_simulation() const -> const InteractiveSimulation* {
    Expects(is_simulation(circuit_state_) || interactive_simulation_.layout().empty());

    if (is_simulation(circuit_state_)) {
        return &interactive_simulation_;
    }
    return nullptr;
}

auto CircuitStore::spatial_simulation() const -> const SpatialSimulation* {
    Expects(is_simulation(circuit_state_) || interactive_simulation_.layout().empty());

    if (is_simulation(circuit_state_)) {
        return &interactive_simulation_.spatial_simulation();
    }
    return nullptr;
}

auto CircuitStore::simulation_events_per_second() const -> std::optional<double> {
    Expects(is_simulation(circuit_state_) || interactive_simulation_.layout().empty());

    if (is_simulation(circuit_state_)) {
        return interactive_simulation_.events_per_second();
    }
    return std::nullopt;
}

}  // namespace circuit_widget

}  // namespace logicsim
