#include "circuit_widget_base.h"

#include <fmt/core.h>

#include <exception>

namespace logicsim {

namespace circuit_widget {

auto SimulationState::format() const -> std::string {
    return "SimulationState";
}

auto NonInteractiveState::format() const -> std::string {
    return "NonInteractiveState";
}

auto EditingState::format() const -> std::string {
    return fmt::format("EditingState({})", default_mouse_action);
}

auto is_simulation(const CircuitState& state) -> bool {
    return std::holds_alternative<SimulationState>(state);
}

}  // namespace circuit_widget

template <>
auto format(circuit_widget::DefaultMouseAction action) -> std::string {
    switch (action) {
        using enum circuit_widget::DefaultMouseAction;

        case selection:
            return "selection";
        case insert_wire:
            return "insert_wire";

        case insert_button:
            return "insert_button";
        case insert_led:
            return "insert_led";
        case insert_display_number:
            return "insert_display_number";
        case insert_display_ascii:
            return "insert_display_ascii";

        case insert_and_element:
            return "insert_and_element";
        case insert_or_element:
            return "insert_or_element";
        case insert_xor_element:
            return "insert_xor_element";
        case insert_nand_element:
            return "insert_nand_element";
        case insert_nor_element:
            return "insert_nor_element";

        case insert_buffer_element:
            return "insert_buffer_element";
        case insert_inverter_element:
            return "insert_inverter_element";
        case insert_flipflop_jk:
            return "insert_flipflop_jk";
        case insert_latch_d:
            return "insert_latch_d";
        case insert_flipflop_d:
            return "insert_flipflop_d";
        case insert_flipflop_ms_d:
            return "insert_flipflop_ms_d";

        case insert_clock_generator:
            return "insert_clock_generator";
        case insert_shift_register:
            return "insert_shift_register";
    };

    std::terminate();
}

//
// Configs
//

namespace circuit_widget {

auto RenderConfig::format() const -> std::string {
    return fmt::format(
        "RenderConfig(\n"
        "  do_benchmark = {},\n"
        "  show_circuit = {},\n"
        "  show_collision_cache = {},\n"
        "  show_connection_cache = {},\n"
        "  show_selection_cache = {},\n"
        "  \n"
        "  zoom_level = {},\n"
        "  \n"
        "  thread_count = {},\n"
        "  direct_rendering = {},\n"
        ")",
        do_benchmark, show_circuit, show_collision_cache, show_connection_cache,
        show_selection_cache, zoom_level, thread_count, direct_rendering);
}

auto SimulationConfig::format() const -> std::string {
    return fmt::format(
        "SimulationConfig(\n"
        "  simulation_time_rate = {},\n"
        "  use_wire_delay = {},\n"
        ")",
        simulation_time_rate, use_wire_delay);
}

}  // namespace circuit_widget

//
// Circuit Widget Base
//

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
