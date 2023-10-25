#include "simulation.h"

#include "algorithm/contains.h"
#include "algorithm/fmt_join.h"
#include "algorithm/range.h"
#include "algorithm/transform_to_container.h"
#include "component/simulation/history_view.h"
#include "component/simulation/simulation_event.h"
#include "component/simulation/simulation_event_group.h"
#include "layout_info.h"
#include "logging.h"
#include "logic_item/simulation_info.h"
#include "vocabulary/connection_ids.h"
#include "vocabulary/internal_state.h"

#include <fmt/core.h>
#include <gsl/gsl>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace logicsim {

namespace {
/**
 * @brief: Sets all outputs to zero, considering input inverters.
 */
auto set_outputs_to_zero(const Schematic &schematic,
                         std::vector<logic_small_vector_t> &input_values) -> void {
    auto set_input = [&](input_t input, bool value) {
        input_values.at(input.element_id.value).at(input.connection_id.value) = value;
    };

    for (const auto element_id : element_ids(schematic)) {
        if (is_logic_item(schematic.element_type(element_id))) {
            for (auto output : outputs(schematic, element_id)) {
                if (const auto input = schematic.input(output);
                    input && schematic.input_inverted(input)) {
                    set_input(input, true);
                }
            }
        }
    }
}

}  // namespace

//
// Simulation
//

auto Simulation::resize_vectors() -> void {
    assert(input_values_.empty());
    assert(internal_states_.empty());
    assert(first_input_histories_.empty());

    // input_values
    input_values_.reserve(schematic_.size());
    for (auto element_id : element_ids(schematic_)) {
        input_values_.emplace_back(schematic_.input_count(element_id).count(), false);
    }

    // internal states
    internal_states_.reserve(schematic_.size());
    for (auto element_id : element_ids(schematic_)) {
        internal_states_.emplace_back(
            internal_state_size(schematic_.element_type(element_id)), false);
    }

    // first input histories
    first_input_histories_.resize(schematic_.size());

    assert(schematic_.size() == input_values_.size());
    assert(schematic_.size() == internal_states_.size());
    assert(schematic_.size() == first_input_histories_.size());
}

Simulation::Simulation(Schematic &&schematic__, PrintEvents do_print)
    : schematic_ {std::move(schematic__)},
      queue_ {},
      largest_history_event_ {queue_.time()},
      print_events_ {do_print == PrintEvents::yes},
      event_count_ {0} {
    resize_vectors();

    initialize_input_values(schematic_, input_values_);
    set_outputs_to_zero(schematic_, input_values_);

    schedule_initial_events();
}

auto Simulation::format_element(element_id_t element_id) const -> std::string {
    const auto element_type = schematic_.element_type(element_id);

    if (element_type == ElementType::wire) {
        return fmt::format("{{{}-{}, inputs: {}, history: {}}}", element_id, element_type,
                           input_values(element_id), input_history(element_id));
    }

    const auto formatted_state = [&] {
        if (internal_state(element_id).empty()) {
            return std::string {};
        }
        return fmt::format(", internal_state: {}", internal_state(element_id));
    }();

    return fmt::format("{{{}-{}, inputs: {}, outputs: {}{}}},", element_id, element_type,
                       input_values(element_id), output_values(element_id),
                       formatted_state);
}

auto Simulation::format() const -> std::string {
    const auto inner = fmt_join("\n  ", element_ids(schematic()), "{}",
                                [&](auto id) { return format_element(id); });

    return fmt::format("<Simulation at {} with {} processed events\n  {}\n>", time(),
                       processed_event_count(), inner);
}

auto Simulation::time() const noexcept -> time_t {
    return queue_.time();
}

auto Simulation::schematic() const noexcept -> const Schematic & {
    return schematic_;
}

auto Simulation::processed_event_count() const noexcept -> event_count_t {
    return event_count_;
}

auto Simulation::apply_events(element_id_t element_id,
                              const simulation::SimulationEventGroup &group) -> void {
    for (const auto &event : group) {
        const auto input = input_t {element_id, event.input_id};
        set_input_internal(input, event.value);
    }
}

namespace {}  // namespace

auto get_changed_outputs(const logic_small_vector_t &old_outputs,
                         const logic_small_vector_t &new_outputs) -> connection_ids_t {
    if (std::size(old_outputs) != std::size(new_outputs)) [[unlikely]] {
        throw std::runtime_error(
            "old_outputs and new_outputs need to have the same size.");
    }

    auto result = connection_ids_t {};
    for (auto index :
         range(gsl::narrow<connection_id_t::value_type>(std::size(old_outputs)))) {
        if (old_outputs[index] != new_outputs[index]) {
            result.push_back(connection_id_t {index});
        }
    }
    return result;
}

void Simulation::submit_event(output_t output,
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
        submit_event(output, new_outputs);
    }
}

auto invert_inputs(logic_small_vector_t &values, const logic_small_vector_t &inverters)
    -> void {
    if (std::size(values) != std::size(inverters)) [[unlikely]] {
        throw std::runtime_error("Inputs and inverters need to have same size.");
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

/**
 * @brief: process all events at the current time
 */
auto Simulation::process_all_current_events() -> void {
    while (queue_.next_event_time() == queue_.time()) {
        auto event_group = queue_.pop_event_group();
        event_count_ += std::ssize(event_group);

        process_event_group(std::move(event_group));
    }
}

namespace {

using event_count_t = Simulation::event_count_t;
using RunConfig = Simulation::RunConfig;

auto validate(RunConfig config, event_count_t current_event_count) -> void {
    if (config.simulate_for < delay_t {0us}) [[unlikely]] {
        throw std::runtime_error("simulation_time needs to be positive.");
    }
    if (config.realtime_timeout < simulation::realtime_timeout_t::zero()) [[unlikely]] {
        throw std::runtime_error("realtime-timeout needs to be positive.");
    }
    if (config.max_events < 0) [[unlikely]] {
        throw std::runtime_error("max events needs to be positive or zero.");
    }
    if (current_event_count > std::numeric_limits<Simulation::event_count_t>::max() -
                                  config.max_events) [[unlikely]] {
        throw std::runtime_error("max events to large, overflows.");
    }
}

auto simulation_end_time(RunConfig config, time_t current_time) -> time_t {
    if (config.simulate_for == simulation::defaults::infinite_simulation) {
        return time_t::max();
    }
    return current_time + config.simulate_for;
}

auto stop_event_count(RunConfig config, event_count_t current_event_count)
    -> event_count_t {
    if (config.max_events == simulation::defaults::no_max_events) {
        return std::numeric_limits<event_count_t>::max();
    }
    return current_event_count + config.max_events;
}

/**
 * @brief: Checking the realtime timeout is expensive, so we only check
 *         it after processing batches of this many events.
 */
constexpr inline auto timer_check_interval = event_count_t {1'000};

auto first_check_count(RunConfig config, event_count_t current_event_count)
    -> event_count_t {
    if (config.realtime_timeout == simulation::defaults::no_realtime_timeout) {
        return std::numeric_limits<event_count_t>::max();
    }
    return current_event_count + timer_check_interval;
}

}  // namespace

auto Simulation::run(RunConfig config) -> void {
    Expects(queue_.next_event_time() > time());
    validate(config, event_count_);

    const auto timer = TimeoutTimer {config.realtime_timeout};
    const auto queue_end_time = simulation_end_time(config, time());
    const auto max_count = stop_event_count(config, event_count_);

    auto next_check = std::min(max_count, first_check_count(config, event_count_));

    while (!queue_.empty() && queue_.next_event_time() <= queue_end_time) {
        queue_.set_time(queue_.next_event_time());
        process_all_current_events();

        if (event_count_ >= next_check) {
            if (timer.reached_timeout() || event_count_ >= max_count) {
                Ensures(queue_.next_event_time() > time());
                return;
            }
            next_check = std::min(max_count, next_check + timer_check_interval);
        }
    }

    // advance simulation time (when not interrupted)
    if (config.simulate_for == simulation::defaults::infinite_simulation) {
        queue_.set_time(std::max(time(), largest_history_event_));
        Ensures(is_finished());
    } else {
        queue_.set_time(queue_end_time);
    }
    Ensures(queue_.next_event_time() > time());
}

auto Simulation::run() -> void {
    run(RunConfig {});
}

auto Simulation::is_finished() const -> bool {
    return queue_.empty() && time() >= largest_history_event_;
}

auto Simulation::schedule_initial_events() -> void {
    assert(queue_.empty());

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

auto Simulation::output_value(output_t output) const -> OptionalLogicValue {
    const auto input = schematic_.input(output);
    if (!input) {
        return std::nullopt;
    }
    return input_value(input) ^ schematic_.input_inverted(input);
}

auto Simulation::output_values(element_id_t element_id) const -> optional_logic_values_t {
    return transform_to_container<optional_logic_values_t>(
        outputs(schematic_, element_id),
        [&](output_t output) { return output_value(output); });
}

auto Simulation::try_set_internal_state(internal_state_t index, bool value) -> bool {
    const auto element_id = index.element_id;
    const auto element_type = schematic_.element_type(element_id);
    const auto output_count = schematic_.output_count(element_id);

    if (!is_internal_state_user_writable(element_type)) {
        throw std::runtime_error("internal state cannot be written to");
    }

    // find time-slot where state is not changed
    constexpr static auto max_tries = 10;
    const auto tries = std::ranges::views::iota(0, max_tries);
    if (!contains(tries, true, [&](auto try_ [[maybe_unused]]) {
            Expects(queue_.next_event_time() > time());
            queue_.set_time(queue_.time() + delay_t::epsilon());

            auto start_state = logic_small_vector_t {internal_state(element_id)};
            process_all_current_events();
            auto end_state = logic_small_vector_t {internal_state(element_id)};

            Ensures(queue_.next_event_time() > time());
            return start_state == end_state;
        })) {
        // give up, inputs are too busy
        return false;
    }

    // change state and schedule resulting events
    const auto old_outputs = calculate_outputs_from_state(internal_state(element_id),
                                                          output_count, element_type);
    internal_states_.at(element_id.value).at(index.internal_state_index.value) = value;
    const auto new_outputs = calculate_outputs_from_state(internal_state(element_id),
                                                          output_count, element_type);
    submit_events_for_changed_outputs(element_id, old_outputs, new_outputs);
    return true;
}

auto Simulation::set_unconnected_input(input_t input, bool value) -> void {
    if (schematic_.output(input)) [[unlikely]] {
        throw std::runtime_error("input is connected");
    }

    if (value == input_value(input)) {
        return;
    }

    // we run the simulation for a very short time
    // this makes sure we are the only one submitting an event at this time and input.
    run({.simulate_for = delay_t::epsilon()});

    // we submit an event, so it will become part of the event group, in case
    // other inputs of the same element are changed.
    queue_.submit_event(simulation::simulation_event_t {
        .time = queue_.time() + delay_t::epsilon(),
        .element_id = input.element_id,
        .input_id = input.connection_id,
        .value = value,
    });
}

auto Simulation::internal_state(element_id_t element_id) const
    -> const logic_small_vector_t & {
    return internal_states_.at(element_id.value);
}

auto Simulation::internal_state(internal_state_t index) const -> bool {
    return internal_state(index.element_id).at(index.internal_state_index.value);
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
