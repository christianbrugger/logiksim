#include "component/circuit_widget/circuit_store.h"

#include "file.h"
#include "logging.h"
#include "serialize.h"
#include "timer.h"
#include "vocabulary/insertion_mode.h"

#include <fmt/core.h>

namespace logicsim {

namespace circuit_widget {

namespace circuit_store {

auto LoadFileResult::format() const -> std::string {
    return fmt::format(
        "LoadFileResult(success = {}, view_point = {}, simulation_config = {})", success,
        view_point, simulation_config);
}

}  // namespace circuit_store

namespace {

auto generate_simulation(const EditableCircuit& editable_circuit,
                         const SimulationConfig& simulation_config)
    -> InteractiveSimulation {
    const auto _ [[maybe_unused]] = Timer {"Generate simulation", Timer::Unit::ms, 3};

    return InteractiveSimulation {
        Layout {editable_circuit.layout()},
        simulation_config.wire_delay_per_distance(),
        simulation_config.simulation_time_rate,
    };
}

}  // namespace

auto CircuitStore::set_circuit_state(CircuitWidgetState new_state) -> void {
    Expects(is_simulation(circuit_state_) == interactive_simulation_.has_value());
    if (new_state == circuit_state_) {
        return;
    }

    if (is_simulation(circuit_state_)) {
        interactive_simulation_ = std::nullopt;
    }

    if (is_simulation(new_state)) {
        interactive_simulation_ =
            generate_simulation(editable_circuit_, simulation_config_);
    }

    // update
    circuit_state_ = new_state;
    Ensures(is_simulation(circuit_state_) == interactive_simulation_.has_value());
}

auto CircuitStore::set_simulation_config(SimulationConfig new_config) -> void {
    Expects(is_simulation(circuit_state_) == interactive_simulation_.has_value());

    if (new_config == simulation_config_) {
        return;
    }

    if (is_simulation(circuit_state_)) {
        // use_wire_delay
        if (new_config.use_wire_delay != simulation_config_.use_wire_delay) {
            throw std::runtime_error(
                "Cannot change 'use_wire_delay' with running simulation");
        }
        // simulation_time_rate
        interactive_simulation_.value().set_simulation_time_rate(
            new_config.simulation_time_rate);
    }

    // update
    simulation_config_ = new_config;
    Ensures(is_simulation(circuit_state_) == interactive_simulation_.has_value());
}

auto CircuitStore::set_layout(Layout&& layout) -> void {
    Expects(is_simulation(circuit_state_) == interactive_simulation_.has_value());

    editable_circuit_ = EditableCircuit {Layout {}};

    if (is_simulation(circuit_state_)) {
        interactive_simulation_ =
            generate_simulation(editable_circuit_, simulation_config_);
    }

    Ensures(is_simulation(circuit_state_) == interactive_simulation_.has_value());
}

auto CircuitStore::load_from_file(std::string filename) -> LoadFileResult {
    Expects(is_simulation(circuit_state_) == interactive_simulation_.has_value());

    const auto load_result = load_layout(load_file(filename));

    if (!load_result) {
        return LoadFileResult {};
    }

    // load into editable_circuit
    editable_circuit_ = EditableCircuit {Layout {}};
    load_result->add(editable_circuit_, {InsertionMode::insert_or_discard});

    // load simulation config
    simulation_config_ = load_result->simulation_config();

    // generate simulation
    if (is_simulation(circuit_state_)) {
        interactive_simulation_ =
            generate_simulation(editable_circuit_, simulation_config_);
    }

    return LoadFileResult {
        .success = true,
        .view_point = load_result->view_point(),
        .simulation_config = load_result->simulation_config(),
    };

    Ensures(is_simulation(circuit_state_) == interactive_simulation_.has_value());
}

auto CircuitStore::layout() const -> const Layout& {
    Expects(is_simulation(circuit_state_) == interactive_simulation_.has_value());

    if (is_simulation(circuit_state_)) {
        return interactive_simulation_.value().layout();
    }
    return editable_circuit_.layout();
}

auto CircuitStore::editable_circuit_pointer() const -> const EditableCircuit* {
    Expects(is_simulation(circuit_state_) == interactive_simulation_.has_value());
    if (!is_editing_state(circuit_state_)) {
        return nullptr;
    }
    return &editable_circuit_;
}

auto CircuitStore::editable_circuit() -> EditableCircuit& {
    Expects(is_simulation(circuit_state_) == interactive_simulation_.has_value());
    if (!is_editing_state(circuit_state_)) {
        throw std::runtime_error("Editable Circuit is only available in editing state");
    }
    return editable_circuit_;
}

auto CircuitStore::editable_circuit() const -> const EditableCircuit& {
    Expects(is_simulation(circuit_state_) == interactive_simulation_.has_value());
    if (!is_editing_state(circuit_state_)) {
        throw std::runtime_error("Editable Circuit is only available in editing state");
    }
    return editable_circuit_;
}

auto CircuitStore::interactive_simulation() -> InteractiveSimulation& {
    Expects(is_simulation(circuit_state_) == interactive_simulation_.has_value());

    if (!is_simulation(circuit_state_)) {
        throw std::runtime_error("Simulation is only available in editing state");
    }
    return interactive_simulation_.value();
}

auto CircuitStore::interactive_simulation() const -> const InteractiveSimulation& {
    Expects(is_simulation(circuit_state_) == interactive_simulation_.has_value());

    if (!is_simulation(circuit_state_)) {
        throw std::runtime_error("Simulation is only available in editing state");
    }
    return interactive_simulation_.value();
}

auto CircuitStore::spatial_simulation_pointer() const -> const SpatialSimulation* {
    Expects(is_simulation(circuit_state_) == interactive_simulation_.has_value());

    if (is_simulation(circuit_state_)) {
        return &interactive_simulation_.value().spatial_simulation();
    }
    return nullptr;
}

auto CircuitStore::simulation_events_per_second() const -> std::optional<double> {
    Expects(is_simulation(circuit_state_) == interactive_simulation_.has_value());

    if (is_simulation(circuit_state_)) {
        return interactive_simulation_.value().events_per_second();
    }
    return std::nullopt;
}

}  // namespace circuit_widget

}  // namespace logicsim
