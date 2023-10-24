
#include "simulation.h"

#include "algorithm/range.h"
#include "algorithm/transform_to_container.h"
#include "component/simulation/history_view.h"
#include "component/simulation/simulation_event.h"
#include "component/simulation/simulation_event_group.h"
#include "exception.h"
#include "layout_info.h"
#include "logging.h"
#include "logic_item/simulation_info.h"

#include <gsl/gsl>

#include <algorithm>
#include <cmath>

namespace logicsim {

auto set_default_outputs(Simulation &simulation) -> void {
    if (simulation.is_initialized()) [[unlikely]] {
        throw_exception("cannot set outputs for initialized simulation");
    }
    const auto &schematic = simulation.schematic();

    for (const auto element_id : element_ids(schematic)) {
        if (is_logic_item(schematic.element_type(element_id))) {
            for (auto output : outputs(schematic, element_id)) {
                if (schematic.input(output)) {
                    simulation.set_output_value(output, false);
                }
            }
        }
    }
}

auto set_default_inputs(Simulation &simulation) -> void {
    if (simulation.is_initialized()) [[unlikely]] {
        throw_exception("cannot set inputs for initialized simulation");
    }
    const auto &schematic = simulation.schematic();

    for (const auto element_id : element_ids(schematic)) {
        // activate unconnected enable inputs
        if (const auto enable_id =
                element_enable_input_id(schematic.element_type(element_id))) {
            const auto input = input_t {element_id, *enable_id};

            if (!schematic.output(input) && !schematic.input_inverted(input)) {
                simulation.set_input_value(input, true);
            }
        }

        // activate unconnected j&k flipflops
        if (schematic.element_type(element_id) == ElementType::flipflop_jk) {
            const auto input_1 = input_t {element_id, connection_id_t {1}};
            const auto input_2 = input_t {element_id, connection_id_t {2}};

            if (!schematic.output(input_1) && !schematic.input_inverted(input_1) &&
                !schematic.output(input_2) && !schematic.input_inverted(input_2)) {
                simulation.set_input_value(input_1, true);
                simulation.set_input_value(input_2, true);
            }
        }
    }
}

//
// Simulation
//

Simulation::Simulation(Schematic &&schematic__, const PrintEvents print_events)
    : schematic_ {std::move(schematic__)},
      queue_ {},
      largest_history_event_ {queue_.time()},
      is_initialized_ {false},
      print_events_ {print_events == PrintEvents::yes} {
    input_values_.reserve(schematic_.size());
    internal_states_.reserve(schematic_.size());

    for (auto element_id : element_ids(schematic_)) {
        input_values_.emplace_back(schematic_.input_count(element_id).count(), false);
        internal_states_.emplace_back(
            internal_state_size(schematic_.element_type(element_id)), false);
    }
    first_input_histories_.resize(schematic_.size());

    assert(schematic_.size() == input_values_.size());
    assert(schematic_.size() == internal_states_.size());
    assert(schematic_.size() == first_input_histories_.size());
}

auto Simulation::schematic() const noexcept -> const Schematic & {
    return schematic_;
}

auto Simulation::time() const noexcept -> time_t {
    return queue_.time();
}

auto Simulation::submit_event(input_t input, delay_t offset, bool value) -> void {
    queue_.submit_event(simulation::simulation_event_t {
        .time = queue_.time() + offset,
        .element_id = input.element_id,
        .input_id = input.connection_id,
        .value = value,
    });
}

auto Simulation::submit_events(element_id_t element_id, delay_t offset,
                               logic_small_vector_t values) -> void {
    if (connection_count_t {std::size(values)} != schematic_.input_count(element_id))
        [[unlikely]] {
        throw_exception("Need to provide number of input values.");
    }
    for (auto input : inputs(schematic_, element_id)) {
        const auto value = values.at(input.connection_id.value);
        submit_event(input, offset, value);
    }
}

auto Simulation::apply_events(element_id_t element_id,
                              const simulation::SimulationEventGroup &group) -> void {
    for (const auto &event : group) {
        const auto input = input_t {element_id, event.input_id};
        set_input_internal(input, event.value);
    }
}

namespace {
using policy = folly::small_vector_policy::policy_size_type<uint32_t>;

using con_index_small_vector_t = folly::small_vector<connection_id_t, 10, policy>;

static_assert(sizeof(con_index_small_vector_t) == 24);

static_assert(con_index_small_vector_t::max_size() >=
              std::size_t {connection_count_t::max()});
}  // namespace

auto get_changed_outputs(const logic_small_vector_t &old_outputs,
                         const logic_small_vector_t &new_outputs)
    -> con_index_small_vector_t {
    if (std::size(old_outputs) != std::size(new_outputs)) [[unlikely]] {
        throw_exception("old_outputs and new_outputs need to have the same size.");
    }

    auto result = con_index_small_vector_t {};
    for (auto index :
         range(gsl::narrow<connection_id_t::value_type>(std::size(old_outputs)))) {
        if (old_outputs[index] != new_outputs[index]) {
            result.push_back(connection_id_t {index});
        }
    }
    return result;
}

void Simulation::create_event(output_t output,
                              const logic_small_vector_t &output_values) {
    if (const auto input = schematic_.input(output)) {
        queue_.submit_event(simulation::simulation_event_t {
            .time = queue_.time() + schematic_.output_delay(output),
            .element_id = input.element_id,
            .input_id = input.connection_id,
            .value = output_values.at(output.connection_id.value),
        });
    }
}

auto Simulation::submit_events_for_changed_outputs(
    element_id_t element_id, const logic_small_vector_t &old_outputs,
    const logic_small_vector_t &new_outputs) -> void {
    const auto changes = get_changed_outputs(old_outputs, new_outputs);
    for (auto output_index : changes) {
        const auto output = output_t {element_id, output_index};
        create_event(output, new_outputs);
    }
}

auto invert_inputs(logic_small_vector_t &values, const logic_small_vector_t &inverters)
    -> void {
    if (std::size(values) != std::size(inverters)) [[unlikely]] {
        throw_exception("Inputs and inverters need to have same size.");
    }
    for (auto i : range(std::ssize(values))) {
        values[i] ^= inverters[i];
    }
}

auto inverted_inputs(logic_small_vector_t values, const logic_small_vector_t &inverters)
    -> logic_small_vector_t {
    invert_inputs(values, inverters);
    return values;
}

auto Simulation::process_event_group(simulation::SimulationEventGroup &&events) -> void {
    if (print_events_) {
        print_fmt("events: {:n}\n", events);
    }
    if (events.empty()) {
        return;
    }
    const auto element_id = events.front().element_id;
    const auto element_type = schematic_.element_type(element_id);

    // short-circuit trivial elements
    if (has_no_logic(element_type)) {
        apply_events(element_id, events);
        return;
    }

    // update inputs
    auto old_inputs = input_values(element_id);
    apply_events(element_id, events);
    auto new_inputs = input_values(element_id);

    const auto &inverters = schematic_.input_inverters(element_id);
    if (std::ranges::any_of(inverters, std::identity {})) {
        invert_inputs(old_inputs, inverters);
        invert_inputs(new_inputs, inverters);
    }

    const auto output_count = schematic_.output_count(element_id);

    if (has_internal_state(element_type)) {
        auto &internal_state = internal_states_.at(element_id.value);

        const auto old_outputs =
            calculate_outputs_from_state(internal_state, output_count, element_type);
        update_internal_state(old_inputs, new_inputs, element_type, internal_state);
        const auto new_outputs =
            calculate_outputs_from_state(internal_state, output_count, element_type);

        submit_events_for_changed_outputs(element_id, old_outputs, new_outputs);
    } else {
        // find changing outputs
        const auto old_outputs =
            calculate_outputs_from_inputs(old_inputs, output_count, element_type);
        const auto new_outputs =
            calculate_outputs_from_inputs(new_inputs, output_count, element_type);

        submit_events_for_changed_outputs(element_id, old_outputs, new_outputs);
    }
}

auto Simulation::run(const delay_t simulation_time,
                     const simulation::realtime_timeout_t timeout,
                     const int64_t max_events) -> int64_t {
    if (!is_initialized_) {
        throw_exception("Simulation first needs to be initialized.");
    }
    if (simulation_time < delay_t {0us}) [[unlikely]] {
        throw_exception("simulation_time needs to be positive.");
    }
    if (max_events < 0) [[unlikely]] {
        throw_exception("max events needs to be positive or zero.");
    }

    if (simulation_time == delay_t {0us}) {
        return 0;
    }

    const auto timer = TimeoutTimer {timeout};
    const auto queue_end_time =
        simulation_time == simulation::defaults::infinite_simulation_time
            ? time_t::max()
            : queue_.time() + simulation_time;
    int64_t event_count = 0;

    // only check time after this many events
    constexpr int64_t check_interval = 1'000;
    int64_t next_check =
        std::min(max_events, timeout == simulation::defaults::no_realtime_timeout
                                 ? std::numeric_limits<int64_t>::max()
                                 : check_interval);

    while (!queue_.empty() && queue_.next_event_time() < queue_end_time) {
        auto event_group = queue_.pop_event_group();
        event_count += std::ssize(event_group);

        process_event_group(std::move(event_group));

        if (event_count >= next_check) {
            // process all events at this time-point
            if (queue_.next_event_time() == queue_.time()) {
                continue;
            }

            // we check timeout after we process at least one group
            if (timer.reached_timeout() || event_count >= max_events) {
                return event_count;
            }
            next_check = std::min(max_events, next_check + check_interval);
        }
    }

    if (simulation_time != simulation::defaults::infinite_simulation_time) {
        queue_.set_time(queue_end_time);
    }
    return event_count;
}

auto Simulation::run_infinitesimal() -> int64_t {
    return run(delay_t::epsilon());
}

auto Simulation::finished() const -> bool {
    return queue_.empty() && time() >= largest_history_event_;
}

auto Simulation::initialize() -> void {
    if (is_initialized()) [[unlikely]] {
        throw std::runtime_error("simulation is already initialized");
    }
    if (!queue_.empty()) [[unlikely]] {
        throw_exception("Cannot initialize simulation with scheduled events.");
    }

    for (const auto element_id : element_ids(schematic_)) {
        const auto element_type = schematic_.element_type(element_id);

        if (element_type == ElementType::wire) {
            continue;
        }

        const auto output_count = schematic_.output_count(element_id);

        if (output_count == connection_count_t {0}) {
            continue;
        }

        // output values without inverters
        const auto old_outputs = transform_to_container<logic_small_vector_t>(
            outputs(schematic_, element_id), [&](const output_t output) -> bool {
                return input_value(schematic_.input(output));
            });

        if (has_internal_state(element_type)) {
            const auto new_inputs = input_values(element_id);

            if (std::ranges::any_of(new_inputs, std::identity {})) {
                const auto old_inputs = logic_small_vector_t(new_inputs.size(), false);
                auto &internal_state = internal_states_.at(element_id.value);

                update_internal_state(old_inputs, new_inputs, element_type,
                                      internal_state);
            }

            const auto new_outputs = calculate_outputs_from_state(
                internal_state(element_id), output_count, element_type);
            submit_events_for_changed_outputs(element_id, old_outputs, new_outputs);

        } else {
            auto curr_inputs = logic_small_vector_t {input_values(element_id)};
            invert_inputs(curr_inputs, schematic_.input_inverters(element_id));
            const auto new_outputs =
                calculate_outputs_from_inputs(curr_inputs, output_count, element_type);

            submit_events_for_changed_outputs(element_id, old_outputs, new_outputs);
        }
    }

    is_initialized_ = true;
}

auto Simulation::is_initialized() const -> bool {
    return is_initialized_;
}

auto Simulation::record_input_history(input_t input, const bool new_value) -> void {
    // we only record the first input, as we only need a history for wires
    if (input.connection_id != connection_id_t {0}) {
        return;
    }
    const auto history_length = schematic_.history_length(input.element_id);

    if (history_length <= delay_t {0ns}) {
        return;
    }
    if (new_value == input_value(input)) {
        return;
    }
    auto &history = first_input_histories_.at(input.element_id.value);

    // remove old values
    clean_history(history, history_length);

    // add new entry
    history.push_back(time());

    // update largest history event
    largest_history_event_ = std::max(largest_history_event_, time() + history_length);
}

auto Simulation::clean_history(simulation::HistoryBuffer &history, delay_t history_length)
    -> void {
    const auto min_time = time() - history_length;
    while (!history.empty() && history.front() < min_time) {
        history.pop_front();
    }
}

auto Simulation::input_value(input_t input) const -> bool {
    return input_values_.at(input.element_id.value).at(input.connection_id.value);
}

auto Simulation::input_values(element_id_t element_id) const
    -> const logic_small_vector_t & {
    return input_values_.at(element_id.value);
}

auto Simulation::set_input_internal(input_t input, bool value) -> void {
    record_input_history(input, value);

    input_values_.at(input.element_id.value).at(input.connection_id.value) = value;
}

auto Simulation::set_output_value(output_t output, bool value) -> void {
    if (is_initialized_) {
        throw_exception("can only set outputs at the start of the simulation");
    }

    const auto input = schematic_.input(output);

    auto &input_value =
        input_values_.at(input.element_id.value).at(input.connection_id.value);
    input_value = value ^ schematic_.input_inverted(input);
}

auto Simulation::output_value(output_t output) const -> bool {
    const auto input = schematic_.input(output);
    return input_value(input) ^ schematic_.input_inverted(input);
}

auto Simulation::output_values(element_id_t element_id) const -> logic_small_vector_t {
    return transform_to_container<logic_small_vector_t>(
        outputs(schematic_, element_id),
        [&](output_t output) { return output_value(output); });
}

auto Simulation::set_input_value(input_t input, bool value) -> void {
    if (schematic_.output(input)) [[unlikely]] {
        throw_exception("cannot set input values for connected inputs");
    }
    auto &input_value =
        input_values_.at(input.element_id.value).at(input.connection_id.value);

    if (!is_initialized_) {
        input_value = value;
    } else {
        if (input_value != value) {
            submit_event(input, delay_t::epsilon(), value);
            run_infinitesimal();
        }
    }
}

auto Simulation::set_internal_state(element_id_t element_id, std::size_t index,
                                    bool value) -> void {
    auto &state = internal_states_.at(element_id.value).at(index);

    if (!is_initialized_) {
        state = value;
    } else {
        const auto output_count = schematic_.output_count(element_id);
        const auto element_type = schematic_.element_type(element_id);

        const auto old_outputs = calculate_outputs_from_state(internal_state(element_id),
                                                              output_count, element_type);
        state = value;
        const auto new_outputs = calculate_outputs_from_state(internal_state(element_id),
                                                              output_count, element_type);

        submit_events_for_changed_outputs(element_id, old_outputs, new_outputs);
        run_infinitesimal();
    }
}

auto Simulation::internal_state(element_id_t element_id) const
    -> const logic_small_vector_t & {
    return internal_states_.at(element_id.value);
}

auto Simulation::internal_state(element_id_t element_id, std::size_t index) const
    -> bool {
    return internal_state(element_id).at(index);
}

auto Simulation::input_history(element_id_t element_id) const -> simulation::HistoryView {
    const auto &input_values = this->input_values(element_id);

    if (input_values.empty()) {
        return simulation::HistoryView {};
    }

    const auto last_value = static_cast<bool>(
        input_values.at(0) ^ schematic_.input_inverters(element_id).at(0));

    return simulation::HistoryView {
        first_input_histories_.at(element_id.value),
        this->time(),
        last_value,
        schematic_.history_length(element_id),
    };
}

}  // namespace logicsim
