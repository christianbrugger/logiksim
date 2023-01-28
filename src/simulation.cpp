
#include "simulation.h"

#include "algorithm.h"
#include "exceptions.h"
#include "format.h"
#include "range.h"
#include "timer.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <gsl/assert>
#include <gsl/gsl>

#include <algorithm>
#include <cmath>
#include <iterator>

namespace logicsim {

//
// Simulation Event
//

auto make_event(Circuit::ConstInput input, time_t time, bool value) -> SimulationEvent {
    return {.time = time,
            .element_id = input.element_id(),
            .input_index = input.input_index(),
            .value = value};
}

auto SimulationEvent::format() const -> std::string {
    auto time_us = std::chrono::duration<double, std::micro> {time};
    return fmt::format(  // std::locale("en_US.UTF-8"),
        "<SimulationEvent: at {:L}us set Element_{}[{}] = {}>", time_us.count(),
        element_id, input_index, value);
}

auto SimulationEvent::operator==(const SimulationEvent &other) const -> bool {
    return this->element_id == other.element_id && this->time == other.time;
}

auto SimulationEvent::operator<(const SimulationEvent &other) const -> bool {
    if (this->time == other.time) {
        return this->element_id < other.element_id;
    }
    return this->time < other.time;
}

auto SimulationEvent::operator!=(const SimulationEvent &other) const -> bool {
    return !(this->operator==(other));
}

auto SimulationEvent::operator>(const SimulationEvent &other) const -> bool {
    return other.operator<(*this);
}

auto SimulationEvent::operator<=(const SimulationEvent &other) const -> bool {
    return !(this->operator>(other));
}

auto SimulationEvent::operator>=(const SimulationEvent &other) const -> bool {
    return !(this->operator<(other));
}

//
// EventGroup
//

void validate(const event_group_t &events) {
    if (events.empty()) {
        return;
    }

    const auto &head {events.front()};
    const auto tail = std::ranges::subrange(events.begin() + 1, events.end());

    if (head.element_id == null_element) {
        throw_exception("Event element cannot be null.");
    }

    if (!tail.empty()) {
        if (!std::ranges::all_of(tail, [time = head.time](const SimulationEvent &event) {
                return event.time == time;
            })) {
            throw_exception("All events in the group need to have the same time.");
        }

        if (!std::ranges::all_of(
                tail, [element_id = head.element_id](const SimulationEvent &event) {
                    return event.element_id == element_id;
                })) {
            throw_exception("All events in the group need to have the same time.");
        }

        const auto pred = [](const SimulationEvent &event) { return event.input_index; };
        if (has_duplicates_quadratic(events.begin(), events.end(), pred)) {
            throw_exception(
                "Cannot have two events for the same input at the same time.");
        }
    }
}

//
// SimulationQueue
//

auto SimulationQueue::time() const noexcept -> time_t {
    return time_;
}

void SimulationQueue::set_time(time_t time) {
    if (time < time_) {
        throw_exception("Cannot set new time to the past.");
    }
    if (time > next_event_time()) {
        throw_exception("New time would be greater than next event.");
    }

    time_ = time;
}

auto SimulationQueue::next_event_time() const noexcept -> time_t {
    return events_.empty() ? time_t::max() : events_.top().time;
}

auto SimulationQueue::empty() const noexcept -> bool {
    return events_.empty();
}

void SimulationQueue::submit_event(SimulationEvent event) {
    if (event.time <= time_) {
        throw_exception("Event time needs to be in the future.");
    }

    events_.push(event);
}

auto SimulationQueue::pop_event_group() -> event_group_t {
    event_group_t group;
    pop_while(
        events_, [&group](const SimulationEvent &event) { group.push_back(event); },
        [&group](const SimulationEvent &event) {
            return group.empty() || group.front() == event;
        });
    if (!group.empty()) {
        this->set_time(group.front().time);
    }
    return group;
}

//
// Simulation
//

[[nodiscard]] constexpr auto internal_state_size(const ElementType type) noexcept
    -> connection_size_t {
    switch (type) {
        using enum ElementType;

        case placeholder:
        case wire:
        case inverter_element:
        case and_element:
        case or_element:
        case xor_element:
            return 0;
        case clock_generator:
        case flipflop_jk:
            return 1;
        case shift_register:
            return 8;
    }
    throw_exception("Dont know internal state of given type.");
}

[[nodiscard]] constexpr auto has_internal_state(const ElementType type) noexcept -> bool {
    return internal_state_size(type) != 0;
}

Simulation::Simulation(const Circuit &circuit)
    : circuit_ {&circuit}, states_(circuit.element_count()), queue_ {} {
    for (auto element : circuit.elements()) {
        auto &state = get_state(element);

        state.input_values.resize(element.input_count(), false);
        state.input_inverters.resize(element.input_count(), false);
        state.output_delays.resize(element.output_count(), defaults::standard_delay);
        state.internal_state.resize(internal_state_size(element.element_type()), false);
    }
}

auto Simulation::get_state(ElementOrConnection auto obj) -> ElementState & {
    return states_.at(obj.element_id());
}

auto Simulation::get_state(ElementOrConnection auto obj) const -> const ElementState & {
    return states_.at(obj.element_id());
}

auto Simulation::circuit() const noexcept -> const Circuit & {
    return *circuit_;
}

auto Simulation::time() const noexcept -> time_t {
    return queue_.time();
}

auto Simulation::submit_event(Circuit::ConstInput input, time_t offset, bool value)
    -> void {
    queue_.submit_event(make_event(input, queue_.time() + offset, value));
}

auto Simulation::submit_events(Circuit::ConstElement element, time_t offset,
                               logic_small_vector_t values) -> void {
    if (std::ssize(values) != element.input_count()) [[unlikely]] {
        throw_exception("Need to provide number of input values.");
    }
    for (auto input : element.inputs()) {
        submit_event(input, offset, values.at(input.input_index()));
    }
}

auto Simulation::check_state_valid() const -> void {
    const auto n_elements
        = static_cast<logic_vector_t::size_type>(circuit_->element_count());

    if (states_.size() != n_elements) [[unlikely]] {
        throw_exception("number of state needs to match number of elements.");
    }
}

auto update_internal_state(const Simulation::logic_small_vector_t &old_input,
                           const Simulation::logic_small_vector_t &new_input,
                           const ElementType type,
                           Simulation::logic_small_vector_t &state) {
    switch (type) {
        using enum ElementType;

        case clock_generator: {
            bool rise_cycle = !new_input.at(0) && old_input.at(0);
            bool rise_start = new_input.at(1) && !old_input.at(1);

            bool in_second_phase = !new_input.at(0);
            bool enabled = new_input.at(1);

            if ((rise_cycle && enabled) || (rise_start && in_second_phase)) {
                state.at(0) = true;
            } else if (new_input.at(0) && !old_input.at(0)) {
                state.at(0) = false;
            }
            return;
        }

        case flipflop_jk: {
            // rising edge
            if (new_input.at(0) && !old_input.at(0)) {
                bool input_j = new_input.at(1);
                bool input_k = new_input.at(2);

                if (input_j && input_k) {
                    state.at(0) = !state.at(0);
                } else if (input_j && !input_k) {
                    state.at(0) = true;
                } else if (!input_j && input_k) {
                    state.at(0) = false;
                }
            }
            return;
        }

        case shift_register: {
            // rising edge
            if (new_input.at(0) && !old_input.at(0)) {
                auto n_inputs = std::ssize(new_input) - 1;

                if (std::ssize(state) < n_inputs) [[unlikely]] {
                    throw_exception(
                        "need at least as many internal states "
                        "as inputs for shift register");
                }

                std::shift_right(state.begin(), state.end(), n_inputs);
                std::copy(std::next(new_input.begin()), new_input.end(), state.begin());
            }
            return;
        }

        default:
            [[unlikely]] throw_exception(
                "Unexpected type encountered in calculate_new_state.");
    }
}

auto calculate_outputs_from_state(const Simulation::logic_small_vector_t &state,
                                  connection_size_t output_count, const ElementType type)
    -> Simulation::logic_small_vector_t {
    switch (type) {
        using enum ElementType;

        case clock_generator: {
            bool enabled = state.at(0);
            return {enabled, enabled};
        }

        case flipflop_jk: {
            bool enabled = state.at(0);
            return {enabled, !enabled};
        }

        case shift_register: {
            if (std::ssize(state) < output_count) [[unlikely]] {
                throw_exception(
                    "need at least output count internal state for shift register");
            }
            return Simulation::logic_small_vector_t(std::prev(state.end(), output_count),
                                                    state.end());
        }

        default:
            [[unlikely]] throw_exception(
                "Unexpected type encountered in calculate_new_state.");
    }
}

auto calculate_outputs_from_inputs(const Simulation::logic_small_vector_t &input,
                                   connection_size_t output_count, const ElementType type)
    -> Simulation::logic_small_vector_t {
    if (input.empty()) [[unlikely]] {
        throw_exception("Input size cannot be zero.");
    }
    if (output_count <= 0) [[unlikely]] {
        throw_exception("Output count cannot be zero or negative.");
    }

    switch (type) {
        using enum ElementType;

        case wire:
            return {Simulation::logic_small_vector_t(output_count, input.at(0))};

        case inverter_element:
            return {!input.at(0)};

        case and_element:
            return {std::ranges::all_of(input, std::identity {})};

        case or_element:
            return {std::ranges::any_of(input, std::identity {})};

        case xor_element:
            return {std::ranges::count_if(input, std::identity {}) == 1};

        default:
            [[unlikely]] throw_exception(
                "Unexpected type encountered in calculate_outputs.");
    }
}

auto Simulation::apply_events(const Circuit::ConstElement element,
                              const event_group_t &group) -> void {
    for (const auto &event : group) {
        set_input(element.input(event.input_index), event.value);
    }
}

auto get_changed_outputs(const Simulation::logic_small_vector_t &old_outputs,
                         const Simulation::logic_small_vector_t &new_outputs)
    -> Simulation::con_index_small_vector_t {
    if (std::size(old_outputs) != std::size(new_outputs)) [[unlikely]] {
        throw_exception("old_outputs and new_outputs need to have the same size.");
    }

    Simulation::con_index_small_vector_t result;
    for (auto index : range(gsl::narrow<connection_size_t>(std::size(old_outputs)))) {
        if (old_outputs[index] != new_outputs[index]) {
            result.push_back(index);
        }
    }
    return result;
}

void Simulation::create_event(const Circuit::ConstOutput output,
                              const logic_small_vector_t &output_values) {
    if (output.has_connected_element()) {
        const auto delay = output_delay(output);
        queue_.submit_event({.time = queue_.time() + delay.value,
                             .element_id = output.connected_element_id(),
                             .input_index = output.connected_input_index(),
                             .value = output_values.at(output.output_index())});
    }
}

auto Simulation::submit_events_for_changed_outputs(
    const Circuit::ConstElement element, const logic_small_vector_t &old_outputs,
    const logic_small_vector_t &new_outputs) -> void {
    const auto changes = get_changed_outputs(old_outputs, new_outputs);
    for (auto output_index : changes) {
        create_event(element.output(output_index), new_outputs);
    }
}

auto invert_inputs(Simulation::logic_small_vector_t &values,
                   const Simulation::logic_small_vector_t &inverters) -> void {
    if (std::size(values) != std::size(inverters)) [[unlikely]] {
        throw_exception("Inputs and inverters need to have same size.");
    }
    for (auto i : range(std::ssize(values))) {
        values[i] ^= inverters[i];
    }
}

auto Simulation::process_event_group(event_group_t &&events) -> void {
    if (print_events) {
        fmt::print("events: {:n}\n", events);
    }
    if (events.empty()) {
        return;
    }
    validate(events);
    const auto element
        = Circuit::ConstElement {circuit_->element(events.front().element_id)};
    const auto element_type = element.element_type();

    // short-circuit placeholders, as they don't have logic
    if (element_type == ElementType::placeholder) {
        apply_events(element, events);
        return;
    }

    // update inputs
    auto old_inputs = input_values(element);
    apply_events(element, events);
    auto new_inputs = input_values(element);

    const auto inverters = has_input_inverters(element);
    if (std::ranges::any_of(inverters, std::identity {})) {
        invert_inputs(old_inputs, inverters);
        invert_inputs(new_inputs, inverters);
    }

    if (has_internal_state(element_type)) {
        auto &internal_state = get_state(element).internal_state;

        const auto old_outputs = calculate_outputs_from_state(
            internal_state, element.output_count(), element_type);
        update_internal_state(old_inputs, new_inputs, element_type, internal_state);
        const auto new_outputs = calculate_outputs_from_state(
            internal_state, element.output_count(), element_type);

        submit_events_for_changed_outputs(element, old_outputs, new_outputs);
    } else {
        // find changing outputs
        const auto old_outputs = calculate_outputs_from_inputs(
            old_inputs, element.output_count(), element_type);
        const auto new_outputs = calculate_outputs_from_inputs(
            new_inputs, element.output_count(), element_type);

        submit_events_for_changed_outputs(element, old_outputs, new_outputs);
    }
}

class Simulation::Timer {
   public:
    using time_point = timeout_clock::time_point;

    explicit Timer(timeout_t timeout = defaults::no_timeout) noexcept
        : timeout_(timeout), start_time_(timeout_clock::now()) {};

    [[nodiscard]] auto reached_timeout() const noexcept -> bool {
        return (timeout_ != defaults::no_timeout)
               && (std::chrono::steady_clock::now() - start_time_) > timeout_;
    }

   private:
    timeout_t timeout_;
    time_point start_time_;
};

auto Simulation::run(const time_t simulation_time, const timeout_t timeout,
                     const int64_t max_events) -> int64_t {
    if (!is_initialized_) {
        throw_exception("Simulation first needs to be initialized.");
    }
    if (simulation_time <= 0us) [[unlikely]] {
        throw_exception("simulation_time needs to be positive.");
    }
    if (max_events < 0) [[unlikely]] {
        throw_exception("max events needs to be positive or zero.");
    }
    check_state_valid();

    if (simulation_time == 0us) {
        return 0;
    }

    const Simulation::Timer timer {timeout};
    const auto queue_end_time = simulation_time == defaults::infinite_simulation_time
                                    ? time_t::max()
                                    : queue_.time() + simulation_time;
    int64_t event_count = 0;

    // TODO refactor loop
    while (!queue_.empty() && queue_.next_event_time() < queue_end_time) {
        auto event_group = queue_.pop_event_group();
        event_count += std::ssize(event_group);

        process_event_group(std::move(event_group));

        // at least one group
        if (timer.reached_timeout() || (event_count >= max_events)) [[unlikely]] {
            return event_count;
        }
    }

    if (simulation_time != defaults::infinite_simulation_time) {
        queue_.set_time(queue_end_time);
    }
    return event_count;
}

auto Simulation::initialize() -> void {
    if (!queue_.empty()) [[unlikely]] {
        throw_exception("Cannot initialize simulation with scheduled events.");
    }

    check_state_valid();

    for (auto &&element : circuit_->elements()) {
        auto element_type = element.element_type();

        // short-circuit placeholders, as they don't have logic
        if (element_type == ElementType::placeholder) {
            continue;
        }

        const auto old_outputs {output_values(element)};

        if (has_internal_state(element_type)) {
            const auto new_outputs = calculate_outputs_from_state(
                internal_state(element), element.output_count(), element_type);

            submit_events_for_changed_outputs(element, old_outputs, new_outputs);

        } else {
            auto curr_inputs = input_values(element);
            invert_inputs(curr_inputs, has_input_inverters(element));
            const auto new_outputs = calculate_outputs_from_inputs(
                curr_inputs, element.output_count(), element_type);

            submit_events_for_changed_outputs(element, old_outputs, new_outputs);
        }
    }

    is_initialized_ = true;
}

auto Simulation::record_input_history(const Circuit::ConstInput input,
                                      const bool new_value) -> void {
    // we only record the first input, as we only need a history for wires
    if (input.input_index() != 0) {
        return;
    }
    auto &state = get_state(input);
    if (state.max_history <= history_t {0ns}) {
        return;
    }
    if (new_value == input_value(input)) {
        return;
    }
    auto &history = state.first_input_history;
    if (!history.empty() && history.front() == time()) {
        throw_exception("Cannot have two transitions recorded at the same time.");
    }

    // remove old values
    clean_history(history, state.max_history);

    // add new entry
    history.push_front(time());
}

auto Simulation::clean_history(history_vector_t &history, history_t max_history) -> void {
    while (!history.empty() && history.back() < time() - max_history.value) {
        history.pop_back();
    }
}

auto Simulation::input_value(const Circuit::ConstInput input) const -> bool {
    return states_.at(input.element_id()).input_values.at(input.input_index());
}

auto Simulation::input_values(const Circuit::ConstElement element) const
    -> logic_small_vector_t {
    return states_.at(element.element_id()).input_values;
}

auto Simulation::input_values() const -> const logic_vector_t {
    auto result = logic_vector_t {};

    for (auto element : circuit_->elements()) {
        std::ranges::copy(input_values(element), std::back_inserter(result));
    }

    return result;
}

auto Simulation::set_input(const Circuit::ConstInput input, bool value) -> void {
    record_input_history(input, value);

    states_.at(input.element_id()).input_values.at(input.input_index()) = value;
}

auto Simulation::output_value(const Circuit::ConstOutput output,
                              const bool raise_missing) const -> bool {
    if (raise_missing || output.has_connected_element()) {
        return input_value(output.connected_input());
    }
    return false;
}

auto Simulation::output_values(const Circuit::ConstElement element,
                               const bool raise_missing) const -> logic_small_vector_t {
    return transform_to_container<logic_small_vector_t>(
        element.outputs(),
        [=, this](auto output) { return output_value(output, raise_missing); });
}

auto Simulation::output_values(const bool raise_missing) const -> logic_vector_t {
    logic_vector_t result(circuit_->output_count());

    for (auto element : circuit_->elements()) {
        std::ranges::copy(output_values(element, raise_missing),
                          std::back_inserter(result));
    }

    return result;
}

[[nodiscard]] auto Simulation::has_input_inverter(Circuit::ConstInput input) const
    -> bool {
    return get_state(input).input_inverters.at(input.input_index());
}

[[nodiscard]] auto Simulation::has_input_inverters(Circuit::ConstElement element) const
    -> logic_small_vector_t {
    return get_state(element).input_inverters;
}

auto Simulation::set_input_inverter(Circuit::ConstInput input, bool value) -> void {
    if (!queue_.empty()) {
        throw_exception("Cannot set input inverters for state with scheduled events.");
    }
    is_initialized_ = false;

    get_state(input).input_inverters.at(input.input_index()) = value;
}

auto Simulation::set_input_inverters(Circuit::ConstElement element,
                                     logic_small_vector_t values) -> void {
    if (!queue_.empty()) {
        throw_exception("Cannot set input inverters for state with scheduled events.");
    }
    is_initialized_ = false;

    if (std::ssize(values) != element.input_count()) {
        throw_exception("Need as many values for has_inverters as inputs.");
    }
    get_state(element).input_inverters.assign(std::begin(values), std::end(values));
}

auto Simulation::output_delay(const Circuit::ConstOutput output) const -> delay_t {
    return get_state(output).output_delays.at(output.output_index());
}

auto Simulation::set_output_delay(const Circuit::ConstOutput output, const delay_t delay)
    -> void {
    if (!queue_.empty()) {
        throw_exception("Cannot set output delay for state with scheduled events.");
    }
    get_state(output).output_delays.at(output.output_index()) = delay;
}

auto Simulation::internal_state(Circuit::ConstElement element) const
    -> const logic_small_vector_t & {
    return get_state(element).internal_state;
}

// auto Simulation::set_internal_state(const Circuit::ConstElement element,
//                                     const logic_small_vector_t &new_state) -> void
//                                     {
//     auto &state = get_state(element);
//
//     if (std::size(new_state) != std::size(state.internal_state)) {
//         throw_exception("New internal state has wrong size.");
//     }
//
//     state.internal_state.assign(new_state.begin(), new_state.end());
// }

auto Simulation::get_input_history(const Circuit::ConstElement element) const
    -> const history_vector_t & {
    return get_state(element).first_input_history;
}

auto Simulation::max_history(const Circuit::ConstElement element) const -> history_t {
    return get_state(element).max_history;
}

auto Simulation::set_max_history(const Circuit::ConstElement element,
                                 const history_t max_history) -> void {
    if (max_history < history_t {0ns}) [[unlikely]] {
        throw_exception("Max history cannot be negative.");
    }
    get_state(element).max_history = max_history;
}

//
// Benchmark
//

template <std::uniform_random_bit_generator G>
void _generate_random_events(G &rng, Simulation &simulation) {
    boost::random::uniform_int_distribution<int32_t> trigger_distribution {0, 1};

    for (auto element : simulation.circuit().elements()) {
        for (auto input : element.inputs()) {
            if (trigger_distribution(rng) == 0) {
                simulation.submit_event(input, 1us, !simulation.input_value(input));
            }
        }
    }
}

template <std::uniform_random_bit_generator G>
auto benchmark_simulation(G &rng, const Circuit &circuit, const int n_events,
                          const bool print) -> int64_t {
    Simulation simulation {circuit};
    simulation.print_events = print;

    // set custom delays
    for (const auto element : circuit.elements()) {
        for (const auto output : element.outputs()) {
            boost::random::uniform_int_distribution<time_t::rep> delay_dist {5, 500};
            simulation.set_output_delay(output, delay_t {1us * delay_dist(rng)});
        }
    }

    // set history for wires
    for (const auto element : circuit.elements()) {
        if (element.element_type() == ElementType::wire) {
            const auto delay = simulation.output_delay(element.output(0));
            simulation.set_max_history(element, history_t {delay.value * 10});
        }
    }

    simulation.initialize();

    int64_t simulated_event_count {0};
    while (true) {
        simulated_event_count += simulation.run(
            Simulation::defaults::infinite_simulation_time,
            Simulation::defaults::no_timeout, n_events - simulated_event_count);

        if (simulated_event_count >= n_events) {
            break;
        }

        _generate_random_events(rng, simulation);
    }

    if (print) {
        auto output_values {simulation.output_values()};

        fmt::print("events simulated = {}\n", simulated_event_count);
        fmt::print("input_values = {}\n",
                   fmt_join("{:b}", simulation.input_values(), ""));
        fmt::print("output_values = {}\n", fmt_join("{:b}", output_values, ""));
        for (auto element : circuit.elements()) {
            if (element.element_type() == ElementType::wire) {
                auto &hist = simulation.get_input_history(element);
                fmt::print("{} {}\n", element, hist);
            }
        }
    }

    Ensures(simulated_event_count >= n_events);
    return simulated_event_count;
}

template auto benchmark_simulation(boost::random::mt19937 &rng, const Circuit &circuit,
                                   const int n_events, const bool print) -> int64_t;

auto benchmark_simulation(const int n_elements, const int n_events, const bool print)
    -> int64_t {
    boost::random::mt19937 rng {0};

    auto circuit = create_random_circuit(rng, n_elements);
    if (print) {
        fmt::print("{}\n", circuit);
    }
    add_output_placeholders(circuit);
    circuit.validate(true);

    return benchmark_simulation(rng, circuit, n_events, print);
}

}  // namespace logicsim
