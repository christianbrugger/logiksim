#include "component/circuit_widget/circuit_store.h"

#include "component/circuit_widget/circuit_example.h"
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

auto generate_simulation(const CheckedEditableCircuit& checked_editable_circuit,
                         const SimulationConfig& simulation_config)
    -> InteractiveSimulation {
    return generate_simulation(checked_editable_circuit.editable_circuit(),
                               simulation_config);
}

auto has_same_config(const InteractiveSimulation& interactive_simulation,
                     const SimulationConfig& simulation_config) -> bool {
    return interactive_simulation.wire_delay_per_distance() ==
               simulation_config.wire_delay_per_distance() &&
           interactive_simulation.simulation_time_rate() ==
               simulation_config.simulation_time_rate;
}

}  // namespace

auto CircuitStore::set_circuit_state(CircuitWidgetState new_state) -> void {
    Expects(class_invariant_holds());

    if (new_state == circuit_state_) {
        return;
    }

    if (is_simulation(circuit_state_)) {
        interactive_simulation_ = std::nullopt;
    }

    if (is_simulation(new_state)) {
        interactive_simulation_ =
            generate_simulation(checked_editable_circuit_, simulation_config_);
    }

    // update
    checked_editable_circuit_.set_circuit_state(new_state);
    circuit_state_ = new_state;

    Expects(class_invariant_holds());
}

auto CircuitStore::set_simulation_config(SimulationConfig new_config) -> void {
    Expects(class_invariant_holds());

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

    Ensures(class_invariant_holds());
}

auto CircuitStore::set_editable_circuit(EditableCircuit&& editable_circuit__,
                                        std::optional<SimulationConfig> new_config)
    -> void {
    Expects(class_invariant_holds());

    checked_editable_circuit_.set_editable_circuit(std::move(editable_circuit__));
    if (new_config) {
        simulation_config_ = *new_config;
    }

    if (is_simulation(circuit_state_)) {
        interactive_simulation_ =
            generate_simulation(checked_editable_circuit_, simulation_config_);
    }

    if (const auto count = layout().size(); 0 < count && count < 30) {
        print(layout());
    }

    Ensures(class_invariant_holds());
}

auto CircuitStore::circuit_state() const -> CircuitWidgetState {
    Expects(class_invariant_holds());

    return circuit_state_;
}

auto CircuitStore::simulation_config() const -> SimulationConfig {
    Expects(class_invariant_holds());

    return simulation_config_;
}

auto CircuitStore::layout() const -> const Layout& {
    Expects(class_invariant_holds());

    return checked_editable_circuit_.editable_circuit().layout();
}

auto CircuitStore::editable_circuit() -> EditableCircuit& {
    Expects(class_invariant_holds());

    if (!is_editing_state(circuit_state_)) {
        throw std::runtime_error("Editable Circuit is only available in editing state");
    }
    return checked_editable_circuit_.editable_circuit();
}

auto CircuitStore::editable_circuit() const -> const EditableCircuit& {
    Expects(class_invariant_holds());

    if (!is_editing_state(circuit_state_)) {
        throw std::runtime_error("Editable Circuit is only available in editing state");
    }
    return checked_editable_circuit_.editable_circuit();
}

auto CircuitStore::interactive_simulation() -> InteractiveSimulation& {
    Expects(class_invariant_holds());

    if (!is_simulation(circuit_state_)) {
        throw std::runtime_error("Simulation is only available in editing state");
    }
    return interactive_simulation_.value();
}

auto CircuitStore::interactive_simulation() const -> const InteractiveSimulation& {
    Expects(class_invariant_holds());

    if (!is_simulation(circuit_state_)) {
        throw std::runtime_error("Simulation is only available in editing state");
    }
    return interactive_simulation_.value();
}

auto CircuitStore::simulation_events_per_second() const -> std::optional<double> {
    Expects(class_invariant_holds());

    if (is_simulation(circuit_state_)) {
        return interactive_simulation_.value().events_per_second();
    }
    return std::nullopt;
}

auto circuit_widget::CircuitStore::class_invariant_holds() const -> bool {
    Expects(is_simulation(circuit_state_) == interactive_simulation_.has_value());

    // this is expensive, so we only check in Debug
    assert(!is_simulation(circuit_state_) ||
           interactive_simulation_.value().layout() ==
               checked_editable_circuit_.editable_circuit().layout());

    Expects(!is_simulation(circuit_state_) ||
            has_same_config(interactive_simulation_.value(), simulation_config_));

    Expects(circuit_state_ == checked_editable_circuit_.circuit_state());

    return true;
}

//
// Free Function
//


auto editable_circuit_pointer(CircuitStore& store) -> EditableCircuit* {
    if (!is_editing_state(store.circuit_state())) {
        return nullptr;
    }
    return &store.editable_circuit();
}

auto editable_circuit_pointer(const CircuitStore& store) -> const EditableCircuit* {
    if (!is_editing_state(store.circuit_state())) {
        return nullptr;
    }
    return &store.editable_circuit();
}

auto spatial_simulation_pointer(const CircuitStore& store) -> const SpatialSimulation* {
    if (is_simulation(store.circuit_state())) {
        return &store.interactive_simulation().spatial_simulation();
    }
    return nullptr;
}

auto set_layout(CircuitStore& store, Layout&& layout__,
                std::optional<SimulationConfig> new_config) -> void {
    // clear to free memory
    store.set_editable_circuit(EditableCircuit {});
    // load new
    store.set_editable_circuit(EditableCircuit {std::move(layout__)}, new_config);
}

auto load_from_file(CircuitStore& store, std::string filename)
    -> circuit_store::LoadFileResult {
    const auto load_result = load_layout(load_file(filename));
    if (!load_result) {
        return circuit_store::LoadFileResult {};
    }

    // clear to free memory
    store.set_editable_circuit(EditableCircuit {});
    // load new
    auto editable_circuit = EditableCircuit {Layout {}};
    load_result->add(editable_circuit, {InsertionMode::insert_or_discard});
    store.set_editable_circuit(EditableCircuit {std::move(editable_circuit)},
                               load_result->simulation_config());

    return circuit_store::LoadFileResult {
        .success = true,
        .view_point = load_result->view_point(),
        .simulation_config = load_result->simulation_config(),
    };
}

auto load_circuit_example(CircuitStore& store, int number,
                          std::optional<SimulationConfig> new_config) -> void {
    // clear to free memory
    store.set_editable_circuit(EditableCircuit {});
    // load new
    auto editable_circuit = EditableCircuit {Layout {}};
    load_example_with_logging(editable_circuit, number);
    store.set_editable_circuit(EditableCircuit {std::move(editable_circuit)}, new_config);
}

auto save_circuit(const CircuitStore& store, std::string filename, ViewPoint view_point)
    -> bool {
    const auto binary =
        serialize_inserted(store.layout(), view_point, store.simulation_config());
    return save_file(filename, binary);
}

auto serialize_circuit(const CircuitStore& store) -> std::string {
    auto relevant_config = SimulationConfig {
        .use_wire_delay = store.simulation_config().use_wire_delay,
    };
    return serialize_inserted(store.layout(), {}, relevant_config);
}

}  // namespace circuit_widget

}  // namespace logicsim