#include "core/element/logicitem/simulation_info.h"

#include "core/element/logicitem/schematic_info.h"
#include "core/schematic.h"
#include "core/vocabulary/connection.h"
#include "core/vocabulary/connection_count.h"

#include <stdexcept>

namespace logicsim {

auto has_no_logic(const ElementType type) noexcept -> bool {
    using enum ElementType;
    return type == placeholder || type == led || type == display_ascii ||
           type == display_number;
}

auto internal_state_size(const ElementType type) -> std::size_t {
    switch (type) {
        using enum ElementType;

        case unused:
        case placeholder:
        case wire:

        case buffer_element:
        case and_element:
        case or_element:
        case xor_element:
            return 0;

        case button:
            return 1;

        case led:
        case display_ascii:
        case display_number:
            return 0;

        case clock_generator:
            return 4;
        case flipflop_jk:
            return 2;
        case shift_register:
            return 10;
        case latch_d:
            return 1;
        case flipflop_d:
            return 1;
        case flipflop_ms_d:
            return 2;

        case sub_circuit:
            return 0;
    }
    std::terminate();
}

auto has_internal_state(const ElementType type) -> bool {
    return internal_state_size(type) != 0;
}

auto is_internal_state_user_writable(const ElementType type) -> bool {
    return !has_internal_connections(type);
}

//
// Initialization
//

// TODO remove Schematic dependency
auto initialize_input_values(const Schematic &schematic,
                             std::vector<logic_small_vector_t> &input_values) -> void {
    auto set_input = [&](input_t input, bool value) {
        input_values.at(std::size_t {input.element_id})
            .at(std::size_t {input.connection_id}) = value;
    };

    for (const auto element_id : element_ids(schematic)) {
        // unconnected enable inputs

        if (const auto enable_id =
                element_enable_input_id(schematic.element_type(element_id))) {
            const auto input = input_t {element_id, *enable_id};

            if (!schematic.output(input) && !schematic.input_inverted(input)) {
                set_input(input, true);
            }
        }

        // unconnected j&k flipflops
        if (schematic.element_type(element_id) == ElementType::flipflop_jk) {
            const auto input_1 = input_t {element_id, connection_id_t {1}};
            const auto input_2 = input_t {element_id, connection_id_t {2}};

            if (!schematic.output(input_1) && !schematic.input_inverted(input_1) &&
                !schematic.output(input_2) && !schematic.input_inverted(input_2)) {
                set_input(input_1, true);
                set_input(input_2, true);
            }
        }
    }
}

//
// State Mappings
//

namespace {

template <bool Const>
struct state_mapping_clock_generator {
    using vector_ref = std::conditional_t<!Const, logic_small_vector_t &,  //
                                          const logic_small_vector_t &>;

    using bool_ref = std::conditional_t<!Const, bool &, const bool &>;

    explicit state_mapping_clock_generator(vector_ref state);

    bool_ref enabled;
    bool_ref output_value;
    bool_ref on_finish_event;
    bool_ref off_finish_event;
};

template <bool Const>
state_mapping_clock_generator<Const>::state_mapping_clock_generator(vector_ref state)
    : enabled {state.at(0)},
      output_value {state.at(1)},
      on_finish_event {state.at(2)},
      off_finish_event {state.at(3)} {
    Expects(state.size() == 4);
}

}  // namespace

//
// Simulation Behavior
//

auto update_internal_state(const logic_small_vector_t &old_input,
                           const logic_small_vector_t &new_input, const ElementType type,
                           logic_small_vector_t &state) -> void {
    switch (type) {
        using enum ElementType;

        case button:
            return;

        case clock_generator: {
            // first input is enable signal
            // second input & output are internal signals to loop the enalbe phase
            // third input & output are internal signals to loop the disable phase

            const auto state_map = state_mapping_clock_generator<false> {state};

            bool input_enabled = new_input.at(0);
            bool on_finished = new_input.at(1) ^ old_input.at(1);
            bool off_finished = new_input.at(2) ^ old_input.at(2);

            if (!state_map.enabled) {
                if (input_enabled) {
                    state_map.enabled = true;

                    state_map.output_value = true;
                    state_map.on_finish_event = !state_map.on_finish_event;
                }
            } else {
                if (on_finished) {
                    state_map.output_value = false;
                    state_map.off_finish_event = !state_map.off_finish_event;
                } else if (off_finished) {
                    if (input_enabled) {
                        state_map.output_value = true;
                        state_map.on_finish_event = !state_map.on_finish_event;
                    } else {
                        state_map.enabled = false;
                    }
                }
            }

            return;
        }

        case flipflop_jk: {
            bool input_j = new_input.at(1);
            bool input_k = new_input.at(2);
            bool input_set = new_input.at(3);
            bool input_reset = new_input.at(4);

            if (input_reset) {
                state.at(0) = false;
                state.at(1) = false;
            } else if (input_set) {
                state.at(0) = true;
                state.at(1) = true;
            } else if (new_input.at(0) && !old_input.at(0)) {  // rising edge
                if (input_j && input_k) {
                    state.at(0) = !state.at(1);
                } else if (input_j && !input_k) {
                    state.at(0) = true;
                } else if (!input_j && input_k) {
                    state.at(0) = false;
                }
            } else if (!new_input.at(0) && old_input.at(0)) {  // faling edge
                state.at(1) = state.at(0);
            }
            return;
        }

        case shift_register: {
            auto n_inputs = std::ssize(new_input) - 1;
            if (std::ssize(state) < n_inputs) [[unlikely]] {
                throw std::runtime_error(
                    "need at least as many internal states "
                    "as inputs for shift register");
            }

            // rising edge - store new value
            if (new_input.at(0) && !old_input.at(0)) {
                std::copy(std::next(new_input.begin()), new_input.end(), state.begin());
            }
            // falling edge - shift register
            if (!new_input.at(0) && old_input.at(0)) {
                std::shift_right(state.begin(), state.end(), n_inputs);
            }
            return;
        }

        case latch_d: {
            bool input_clk = new_input.at(0);
            bool input_d = new_input.at(1);

            if (input_clk) {
                state.at(0) = input_d;
            }
            return;
        }

        case flipflop_d: {
            bool input_d = new_input.at(1);
            bool input_set = new_input.at(2);
            bool input_reset = new_input.at(3);

            if (input_reset) {
                state.at(0) = false;
            } else if (input_set) {
                state.at(0) = true;
            } else if (new_input.at(0) && !old_input.at(0)) {  // rising edge
                state.at(0) = input_d;
            }
            return;
        }

        case flipflop_ms_d: {
            bool input_d = new_input.at(1);
            bool input_set = new_input.at(2);
            bool input_reset = new_input.at(3);

            if (input_reset) {
                state.at(0) = false;
                state.at(1) = false;
            } else if (input_set) {
                state.at(0) = true;
                state.at(1) = true;
            } else if (new_input.at(0) && !old_input.at(0)) {  // rising edge
                state.at(0) = input_d;
            } else if (!new_input.at(0) && old_input.at(0)) {  // faling edge
                state.at(1) = state.at(0);
            }
            return;
        }

        default:
            throw std::runtime_error("Element type has no state.");
    }
}

auto calculate_outputs_from_state(const logic_small_vector_t &state,
                                  connection_count_t output_count, const ElementType type)
    -> logic_small_vector_t {
    switch (type) {
        using enum ElementType;

        case button: {
            return {state.at(0)};
        }

        case clock_generator: {
            const auto state_map = state_mapping_clock_generator<true> {state};

            return {state_map.output_value, state_map.on_finish_event,
                    state_map.off_finish_event};
        }

        case flipflop_jk: {
            const bool enabled = state.at(1);
            return {enabled, !enabled};
        }

        case shift_register: {
            if (std::size(state) < std::size_t {output_count}) [[unlikely]] {
                throw std::runtime_error(
                    "need at least output count internal state for shift register");
            }
            return logic_small_vector_t(std::prev(state.end(), output_count.count()),
                                        state.end());
        }

        case latch_d: {
            const bool data = state.at(0);
            return {data};
        }

        case flipflop_d: {
            const bool data = state.at(0);
            return {data};
        }

        case flipflop_ms_d: {
            const bool data = state.at(1);
            return {data};
        }

        default:
            throw std::runtime_error("Element type has no state.");
    }
}

auto calculate_outputs_from_inputs(const logic_small_vector_t &input,
                                   connection_count_t output_count,
                                   const ElementType type) -> logic_small_vector_t {
    if (input.empty()) [[unlikely]] {
        throw std::runtime_error("Input size cannot be zero.");
    }
    if (output_count <= connection_count_t {0}) [[unlikely]] {
        throw std::runtime_error("Output count cannot be zero or negative.");
    }

    switch (type) {
        using enum ElementType;

        case wire:
            return {logic_small_vector_t(output_count.count(), input.at(0))};

        case buffer_element:
            return {input.at(0)};

        case and_element:
            return {std::ranges::all_of(input, std::identity {})};

        case or_element:
            return {std::ranges::any_of(input, std::identity {})};

        case xor_element:
            return {std::ranges::count_if(input, std::identity {}) == 1};

        default:
            throw std::runtime_error("Unexpected type encountered in calculate_outputs.");
    }
}

}  // namespace logicsim
