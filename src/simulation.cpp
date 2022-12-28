
#include "simulation.h"

#include "algorithm.h"
#include "exceptions.h"
#include "format.h"
#include "range.h"
#include "timer.h"

#include <gsl/assert>
#include <gsl/gsl>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>

namespace logicsim {

//
// Simulation Event
//

SimulationEvent make_event(Circuit::ConstInput input, time_t time, bool value) {
    return {.time = time,
            .element_id = input.element_id(),
            .input_index = input.input_index(),
            .value = value};
}

std::string SimulationEvent::format() const {
    auto time_us = std::chrono::duration<double, std::micro> {time};
    return fmt::format(  // std::locale("en_US.UTF-8"),
        "<SimulationEvent: at {:L}us set Element_{}[{}] = {}>", time_us.count(),
        element_id, input_index, value);
}

bool SimulationEvent::operator==(const SimulationEvent &other) const {
    return this->element_id == other.element_id && this->time == other.time;
}

bool SimulationEvent::operator<(const SimulationEvent &other) const {
    if (this->time == other.time) {
        return this->element_id < other.element_id;
    }
    return this->time < other.time;
}

bool SimulationEvent::operator!=(const SimulationEvent &other) const {
    return !(this->operator==(other));
}

bool SimulationEvent::operator>(const SimulationEvent &other) const {
    return other.operator<(*this);
}

bool SimulationEvent::operator<=(const SimulationEvent &other) const {
    return !(this->operator>(other));
}

bool SimulationEvent::operator>=(const SimulationEvent &other) const {
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
    //{ranges::views::drop(events, 1)};  // TODO use tail

    if (head.element_id == null_element) {
        throw_exception("Event element cannot be null.");
    }

    if (!tail.empty()) {
        if (!std::ranges::all_of(tail, [head](const SimulationEvent &event) {
                return event.time == head.time;
            })) {
            throw_exception("All events in the group need to have the same time.");
        }

        if (!std::ranges::all_of(tail, [head](const SimulationEvent &event) {
                return event.element_id == head.element_id;
            })) {
            throw_exception("All events in the group need to have the same time.");
        }

        boost::container::small_vector<element_id_t, 2> event_ids;
        std::ranges::transform(
            events, std::back_inserter(event_ids),
            [](const SimulationEvent &event) { return event.input_index; });
        if (has_duplicates_quadratic(event_ids.begin(), event_ids.end())) {
            throw_exception(
                "Cannot have two events for the same input at the same time.");
        }
    }
}

//
// SimulationQueue
//

time_t SimulationQueue::time() const noexcept { return time_; }

void SimulationQueue::set_time(time_t time) {
    if (time < time_) {
        throw_exception("Cannot set new time to the past.");
    }
    if (time > next_event_time()) {
        throw_exception("New time would be greater than next event.");
    }

    time_ = time;
}

time_t SimulationQueue::next_event_time() const noexcept {
    return events_.empty() ? time_t::max() : events_.top().time;
}

bool SimulationQueue::empty() const noexcept { return events_.empty(); }

void SimulationQueue::submit_event(SimulationEvent event) {
    if (event.time <= time_) {
        throw_exception("Event time needs to be in the future.");
    }
    //    if (!std::isfinite(event.time)) {
    //        throw_exception("Event time needs to be finite.");
    //    }

    events_.push(event);
}

event_group_t SimulationQueue::pop_event_group() {
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
// SimulationState
//

SimulationState::SimulationState(connection_id_t total_inputs,
                                 connection_id_t total_outputs)
    : input_values(total_inputs, false),
      queue {},
      output_delays(total_outputs, standard_delay) {}

SimulationState::SimulationState(const Circuit &circuit)
    : SimulationState(circuit.total_input_count(), circuit.total_output_count()) {}

void check_input_size(const SimulationState &state, const Circuit &circuit) {
    const auto circuit_inputs
        = static_cast<logic_vector_t::size_type>(circuit.total_input_count());
    if (state.input_values.size() != circuit_inputs) [[unlikely]] {
        throw_exception(
            "input_values in state needs to match total inputs of the circuit.");
    }
}

//
// Simulation
//

using con_index_small_vector_t = boost::container::small_vector<connection_size_t, 8>;

logic_small_vector_t calculate_outputs(const logic_small_vector_t &input,
                                       connection_size_t output_count,
                                       const ElementType type) {
    if (input.empty()) [[unlikely]] {
        throw_exception("Input size cannot be zero.");
    }
    if (output_count <= 0) [[unlikely]] {
        throw_exception("Output count cannot be zero or negative.");
    }

    switch (type) {
        case ElementType::wire:
            return {logic_small_vector_t(output_count, input.at(0))};

        case ElementType::inverter_element:
            return {!input.at(0)};

        case ElementType::and_element:
            return {std::ranges::all_of(input, std::identity {})};

        case ElementType::or_element:
            return {std::ranges::any_of(input, std::identity {})};

        case ElementType::xor_element:
            return {std::ranges::count_if(input, std::identity {}) == 1};

        default:
            [[unlikely]] throw_exception(
                "Unknown type encountered in calculate_outputs.");
    }
}

logic_small_vector_t get_input_values(const Circuit::ConstElement element,
                                      const logic_vector_t &input_values) {
    const auto begin = input_values.begin() + element.first_input_id();
    const auto end = begin + element.input_count();

    if (!(begin >= input_values.begin() && begin < input_values.end()
          && end >= input_values.begin() && end <= input_values.end())) {
        throw_exception("Invalid begin or end iterator in get_input_values.");
    }

    return logic_small_vector_t {begin, end};
}

logic_small_vector_t get_output_values(const Circuit::ConstElement element,
                                       const logic_vector_t &input_values,
                                       const bool raise_missing) {
    logic_small_vector_t result;
    result.reserve(element.output_count());

    // for (const auto output : element.outputs()) {
    //     result.push_back(get_output_value(output, input_values, raise_missing));
    // }

    std::ranges::transform(
        element.outputs(), std::back_inserter(result), [&](const auto output) {
            return get_output_value(output, input_values, raise_missing);
        });

    return result;
}

logic_small_vector_t get_input_values(const Circuit::ConstElement element,
                                      const SimulationState &state) {
    return get_input_values(element, state.input_values);
}

logic_small_vector_t get_output_values(const Circuit::ConstElement element,
                                       const SimulationState &state,
                                       const bool raise_missing) {
    return get_output_values(element, state.input_values, raise_missing);
}

void set_input(logic_vector_t &input_values, const Circuit::ConstElement element,
               connection_size_t input_index, bool value) {
    const auto input_id
        = static_cast<logic_vector_t::size_type>(element.input_id(input_index));
    input_values.at(input_id) = value;
}

void apply_events(logic_vector_t &input_values, const Circuit::ConstElement element,
                  const event_group_t &group) {
    for (const auto &event : group) {
        set_input(input_values, element, event.input_index, event.value);
    }
}

con_index_small_vector_t get_changed_outputs(const logic_small_vector_t &old_outputs,
                                             const logic_small_vector_t &new_outputs) {
    if (std::size(old_outputs) != std::size(new_outputs)) [[unlikely]] {
        throw_exception("old_outputs and new_outputs need to have the same size.");
    }

    con_index_small_vector_t result;
    result.reserve(std::size(old_outputs));

    for (auto i : range(std::size(old_outputs))) {
        if (old_outputs[i] != new_outputs[i]) {
            result.push_back(gsl::narrow<connection_size_t>(i));
        }
    }

    return result;
}

void create_event(SimulationQueue &queue, const Circuit::ConstOutput output,
                  const logic_small_vector_t &output_values,
                  const delay_vector_t &output_delays) {
    const auto delay = get_output_delay(output, output_delays);

    if (output.has_connected_element()) {
        queue.submit_event({.time = queue.time() + delay,
                            .element_id = output.connected_element_id(),
                            .input_index = output.connected_input_index(),
                            .value = output_values.at(output.output_index())});
    }
}

void process_event_group(SimulationState &state, const Circuit &circuit,
                         event_group_t &&events, const bool print_events = false) {
    if (print_events) {
        fmt::print("events: {:n}\n", events);
    }
    if (events.empty()) {
        return;
    }
    validate(events);

    const Circuit::ConstElement element {circuit.element(events.front().element_id)};

    // short-circuit placeholders, as they don't have logic
    if (element.element_type() == ElementType::placeholder) {
        apply_events(state.input_values, element, events);
        return;
    }

    // update inputs
    const auto old_inputs = get_input_values(element, state);
    apply_events(state.input_values, element, events);
    const auto new_inputs = get_input_values(element, state);

    // find changing outputs
    const auto old_outputs
        = calculate_outputs(old_inputs, element.output_count(), element.element_type());
    const auto new_outputs
        = calculate_outputs(new_inputs, element.output_count(), element.element_type());
    const auto changes = get_changed_outputs(old_outputs, new_outputs);

    // submit events
    for (auto output_index : changes) {
        create_event(state.queue, element.output(output_index), new_outputs,
                     state.output_delays);
    }
}

class SimulationTimer {
   public:
    using time_point = timeout_clock::time_point;

    explicit SimulationTimer(timeout_t timeout = defaults::no_timeout) noexcept
        : timeout_(timeout), start_time_(timeout_clock::now()) {};

    [[nodiscard]] bool reached_timeout() const noexcept {
        return (timeout_ != defaults::no_timeout)
               && (std::chrono::steady_clock::now() - start_time_) > timeout_;
    }

   private:
    timeout_t timeout_;
    time_point start_time_;
};

int64_t advance_simulation(SimulationState &state, const Circuit &circuit,
                           const time_t simultation_time, const timeout_t timeout,
                           const int64_t max_events, const bool print_events) {
    // TODO test expects
    Expects(simultation_time >= 0us);
    if (simultation_time < 0us) [[unlikely]] {
        throw_exception("simultation_time needs to be positive.");
    }
    if (max_events < 0) [[unlikely]] {
        throw_exception("max events needs to be positive or zero.");
    }
    check_input_size(state, circuit);

    if (simultation_time == 0us) {
        return 0;
    }

    const SimulationTimer timer {timeout};
    const auto queue_end_time = simultation_time == defaults::infinite_simulation_time
                                    ? time_t::max()
                                    : state.queue.time() + simultation_time;
    int64_t event_count = 0;

    // TODO use ranges
    while (!state.queue.empty() && state.queue.next_event_time() < queue_end_time) {
        auto event_group = state.queue.pop_event_group();
        event_count += std::ssize(event_group);

        process_event_group(state, circuit, std::move(event_group), print_events);

        // at least one group
        if (timer.reached_timeout() || (event_count >= max_events)) [[unlikely]] {
            return event_count;
        }
    }

    if (simultation_time != defaults::infinite_simulation_time) {
        state.queue.set_time(queue_end_time);
    }
    return event_count;
}

void initialize_simulation(SimulationState &state, const Circuit &circuit) {
    check_input_size(state, circuit);

    for (auto &&element : circuit.elements()) {
        // short-circuit placeholders, as they don't have logic
        if (element.element_type() == ElementType::placeholder) {
            continue;
        }

        // find outputs that need an update
        const auto old_outputs {get_output_values(element, state, true)};
        const auto curr_inputs {get_input_values(element, state)};
        const auto new_outputs {calculate_outputs(curr_inputs, element.output_count(),
                                                  element.element_type())};
        const auto changes {get_changed_outputs(old_outputs, new_outputs)};

        // submit new events
        for (auto output_index : changes) {
            create_event(state.queue, element.output(output_index), new_outputs,
                         state.output_delays);
        }
    }
}

SimulationState get_uninitialized_state(Circuit &circuit) {
    add_output_placeholders(circuit);
    circuit.validate(true);

    return SimulationState {circuit};
}

SimulationState get_initialized_state(Circuit &circuit) {
    add_output_placeholders(circuit);
    circuit.validate(true);

    SimulationState state {circuit};
    initialize_simulation(state, circuit);
    return state;
}

SimulationState simulate_circuit(Circuit &circuit, time_t simultation_time,
                                 timeout_t timeout, bool print_events) {
    add_output_placeholders(circuit);
    circuit.validate(true);

    SimulationState state {circuit};
    initialize_simulation(state, circuit);

    advance_simulation(state, circuit, simultation_time, timeout, defaults::no_max_events,
                       print_events);
    return state;
}

bool get_input_value(const Circuit::ConstInput input,
                     const logic_vector_t &input_values) {
    return input_values.at(input.input_id());
}

bool get_input_value(const Circuit::ConstInput input, const SimulationState &state) {
    return get_input_value(input, state.input_values);
}

bool get_output_value(const Circuit::ConstOutput output,
                      const logic_vector_t &input_values, const bool raise_missing) {
    if (raise_missing || output.has_connected_element()) {
        return input_values.at(output.connected_input().input_id());
    }
    return false;
}

bool get_output_value(const Circuit::ConstOutput output, const SimulationState &state,
                      const bool raise_missing) {
    return get_output_value(output, state.input_values, raise_missing);
}

logic_vector_t get_all_output_values(const logic_vector_t &input_values,
                                     const Circuit &circuit, const bool raise_missing) {
    logic_vector_t output_values(circuit.total_output_count());

    for (auto element : circuit.elements()) {
        for (auto output : element.outputs()) {
            output_values.at(output.output_id())
                = get_output_value(output, input_values, raise_missing);
        }
    }

    return output_values;
}

time_t get_output_delay(const Circuit::ConstOutput output,
                        const delay_vector_t &output_delays) {
    return output_delays.at(output.output_id());
}

time_t get_output_delay(const Circuit::ConstOutput output, const SimulationState &state) {
    return get_output_delay(output, state.output_delays);
}

void set_output_delay(const Circuit::ConstOutput output, SimulationState &state,
                      const time_t delay) {
    if (!state.queue.empty()) {
        throw_exception("Cannot set output delay for state with scheduled events.");
    }

    state.output_delays.at(output.output_id()) = delay;
}

//
// Benchmark
//

template <std::uniform_random_bit_generator G>
void _generate_random_events(G &rng, const Circuit &circuit, SimulationState &state) {
    boost::random::uniform_int_distribution<int32_t> trigger_distribution {0, 1};

    for (auto element : circuit.elements()) {
        for (auto input : element.inputs()) {
            if (trigger_distribution(rng) == 0) {
                state.queue.submit_event(make_event(input, state.queue.time() + 1us,
                                                    !get_input_value(input, state)));
            }
        }
    }
}

template <std::uniform_random_bit_generator G>
auto benchmark_simulation(G &rng, const Circuit &circuit, const int n_events,
                          const bool print) -> int64_t {
    SimulationState state {circuit};

    // set custom delays
    std::ranges::generate(state.output_delays, [&rng]() {
        boost::random::uniform_int_distribution<time_t::rep> nanosecond_dist {5, 500};
        return 1us * nanosecond_dist(rng);
    });

    initialize_simulation(state, circuit);

    int64_t simulated_event_count {0};
    while (true) {
        simulated_event_count += advance_simulation(
            state, circuit, defaults::infinite_simulation_time, defaults::no_timeout,
            n_events - simulated_event_count, print);

        if (simulated_event_count >= n_events) {
            break;
        }

        _generate_random_events(rng, circuit, state);
    }

    if (print) {
        auto output_values {get_all_output_values(state.input_values, circuit)};

        fmt::print("events simulated = {}\n", simulated_event_count);
        fmt::print("input_values = {}\n", fmt_join("{:b}", state.input_values, ""));
        fmt::print("output_values = {}\n", fmt_join("{:b}", output_values, ""));
    }

    Ensures(simulated_event_count >= n_events);
    return simulated_event_count;
}

template auto benchmark_simulation(boost::random::mt19937 &rng, const Circuit &circuit,
                                   const int n_events, const bool print) -> int64_t;

auto benchmark_simulation(const int n_elements, const int n_events, const bool print)
    -> int64_t {
    boost::random::mt19937 rng {0};

    auto circuit = create_random_circuit(rng, n_elements, 0.75);
    if (print) {
        fmt::print("{}\n", circuit);
    }
    add_output_placeholders(circuit);
    circuit.validate(true);

    return benchmark_simulation(rng, circuit, n_events, print);
}

}  // namespace logicsim
