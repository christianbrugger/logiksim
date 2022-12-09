
#include "simulation.h"

#include "algorithms.h"

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <range/v3/all.hpp>

#include <algorithm>
#include <functional>
#include <random>

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
    auto element_id_str {(element_id == null_element) ? "NULL"
                                                      : fmt::format("{}", element_id)};
    return fmt::format("<SimulationEvent: at {}s set Element_{}[{}] = {}>", time,
                       element_id_str, input_index, (value ? "true" : "false"));
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
    const auto tail {ranges::views::drop(events, 1)};  // TODO use tail

    if (head.element_id == null_element) {
        throw_exception("Event element cannot be null.");
    }
    if (!std::isfinite(head.time)) {
        throw_exception("Event time needs to be finite.");
    }

    if (!tail.empty()) {
        if (!ranges::all_of(
                tail, [head](const auto &event) { return event.time == head.time; })) {
            throw_exception("All events in the group need to have the same time.");
        }

        if (!ranges::all_of(tail, [head](const auto &event) {
                return event.element_id == head.element_id;
            })) {
            throw_exception("All events in the group need to have the same time.");
        }

        if (has_duplicates_quadratic(ranges::views::transform(
                events, [](const auto &event) { return event.input_index; }))) {
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
    if (!std::isfinite(time)) {
        throw_exception("New time needs to be finite.");
    }
    if (time < time_) {
        throw_exception("Cannot set new time to the past.");
    }
    if (time > next_event_time()) {
        throw_exception("New time would be greater than next event.");
    }

    time_ = time;
}

time_t SimulationQueue::next_event_time() const noexcept {
    return events_.empty() ? std::numeric_limits<time_t>::infinity() : events_.top().time;
}

bool SimulationQueue::empty() const noexcept { return events_.empty(); }

void SimulationQueue::submit_event(SimulationEvent event) {
    if (event.time <= time_) {
        throw_exception("Event time needs to be in the future.");
    }
    if (!std::isfinite(event.time)) {
        throw_exception("Event time needs to be finite.");
    }

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
    : input_values(total_inputs), queue {}, output_delays(total_outputs) {}

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
            return ranges::views::repeat_n(input.at(0), output_count)
                   | ranges::to<logic_small_vector_t>();

        case ElementType::inverter_element:
            return {!input.at(0)};

        case ElementType::and_element:
            return {ranges::all_of(input, std::identity {})};

        case ElementType::or_element:
            return {ranges::any_of(input, std::identity {})};

        case ElementType::xor_element:
            return {ranges::count_if(input, std::identity {}) == 1};

        default:
            return {};
    }
}

logic_small_vector_t get_input_values(const Circuit::ConstElement element,
                                      const logic_vector_t &input_values) {
    return ranges::views::all(input_values)
           | ranges::views::drop_exactly(element.first_input_id())
           | ranges::views::take_exactly(element.input_count())
           | ranges::to<logic_small_vector_t>();
}

logic_small_vector_t get_output_values(const Circuit::ConstElement element,
                                       const logic_vector_t &input_values,
                                       const bool raise_missing) {
    return element.outputs()
           | ranges::views::transform([&](const Circuit::ConstOutput output) {
                 return get_output_value(output, input_values, raise_missing);
             })
           | ranges::to<logic_small_vector_t>();
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
    ranges::for_each(group, [&](const SimulationEvent &event) {
        set_input(input_values, element, event.input_index, event.value);
    });
}

con_index_small_vector_t get_changed_outputs(const logic_small_vector_t &old_outputs,
                                             const logic_small_vector_t &new_outputs) {
    if (std::size(old_outputs) != std::size(new_outputs)) [[unlikely]] {
        throw_exception("old_outputs and new_outputs need to have the same size.");
    }

    con_index_small_vector_t result;
    for (const auto &&[index, old_value, new_value] :
         ranges::views::zip(ranges::views::iota(0), old_outputs, new_outputs)) {
        if (old_value != new_value) {
            result.push_back(static_cast<connection_size_t>(index));
        }
    }
    return result;
}

namespace defaults {
constexpr time_t standard_delay = 0.1;
}

void create_event(SimulationQueue &queue, Circuit::ConstOutput output,
                  const logic_small_vector_t &output_values,
                  const delay_vector_t &output_delays) {
    time_t delay {get_output_delay(output, output_delays)};
    if (delay == 0) {
        delay = defaults::standard_delay;
    }

    const time_t time {queue.time() + delay};

    if (output.has_connected_element()) {
        queue.submit_event({.time = time,
                            .element_id = output.connected_element_id(),
                            .input_index = output.connected_input_index(),
                            .value = output_values.at(output.output_index())});
    }
}

void process_event_group(SimulationState &state, const Circuit &circuit,
                         event_group_t &&events, bool print_events = false) {
    if (events.empty()) {
        return;
    }
    validate(events);

    if (print_events) {
        fmt::print("events: {:n:}\n", events);
    }

    const Circuit::ConstElement element {circuit.element(events.front().element_id)};

    // short-circuit placeholders, as they don't have logic
    if (element.element_type() == ElementType::placeholder) {
        apply_events(state.input_values, element, events);
        return;
    }

    // update inputs
    const auto old_inputs {get_input_values(element, state)};
    apply_events(state.input_values, element, events);
    const auto new_inputs {get_input_values(element, state)};

    // find changing outputs
    const auto old_outputs {
        calculate_outputs(old_inputs, element.output_count(), element.element_type())};
    const auto new_outputs {
        calculate_outputs(new_inputs, element.output_count(), element.element_type())};
    const auto changes {get_changed_outputs(old_outputs, new_outputs)};

    // submit events
    ranges::for_each(changes, [&, element](auto output_index) {
        create_event(state.queue, element.output(output_index), new_outputs,
                     state.output_delays);
    });
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

void advance_simulation(SimulationState &state, const Circuit &circuit, time_t time_delta,
                        timeout_t timeout, bool print_events) {
    if (time_delta <= 0) [[unlikely]] {
        throw_exception("time_delta needs to be positive.");
    }
    check_input_size(state, circuit);

    const SimulationTimer timer {timeout};
    const auto queue_end_time {state.queue.time() + time_delta};

    while (!state.queue.empty() && state.queue.next_event_time() < queue_end_time) {
        process_event_group(state, circuit, state.queue.pop_event_group(), print_events);

        // we check here, so we process at least one group
        if (timer.reached_timeout()) {
            return;
        }
    }

    if (std::isfinite(time_delta)) {
        state.queue.set_time(queue_end_time);
    }
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
        ranges::for_each(changes, [&, element](auto output_index) {
            create_event(state.queue, element.output(output_index), new_outputs,
                         state.output_delays);
        });
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

SimulationState simulate_circuit(Circuit &circuit, time_t time_delta, timeout_t timeout,
                                 bool print_events) {
    add_output_placeholders(circuit);
    circuit.validate(true);

    SimulationState state {circuit};
    initialize_simulation(state, circuit);

    advance_simulation(state, circuit, time_delta, timeout, print_events);
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

void set_output_delay(const Circuit::ConstOutput output, delay_vector_t &output_delays,
                      const time_t delay) {
    output_delays.at(output.output_id()) = delay;
}

void set_output_delay(const Circuit::ConstOutput output, SimulationState &state,
                      const time_t delay) {
    if (!state.queue.empty()) {
        throw_exception("Cannot set output delay for state with scheduled events.");
    }

    return set_output_delay(output, state.output_delays, delay);
}

//
// Benchmark
//

void add_random_element(Circuit &circuit, std::mt19937 &rng) {
    std::uniform_int_distribution<> element_dist {0, 1};
    std::uniform_int_distribution<> connection_dist {1, 8};

    const auto element_type {element_dist(rng) == 0 ? ElementType::xor_element
                                                    : ElementType::wire};

    const auto input_count {static_cast<connection_size_t>(
        element_type == ElementType::wire ? 1 : connection_dist(rng))};
    const auto output_count {static_cast<connection_size_t>(
        element_type == ElementType::wire ? connection_dist(rng) : 1)};

    circuit.add_element(element_type, input_count, output_count);
}

auto try_add_random_connection(Circuit &circuit, std::mt19937 &rng) -> bool {
    if (circuit.element_count() == 0) {
        return false;
    }

    auto element_id_dist
        = std::uniform_int_distribution<element_id_t> {0, circuit.element_count() - 1};

    auto element_1 = circuit.element(element_id_dist(rng));
    auto element_2 = circuit.element(element_id_dist(rng));

    auto input = element_1.input(static_cast<connection_size_t>(
        std::uniform_int_distribution<> {0, element_1.input_count() - 1}(rng)));
    auto output = element_2.output(static_cast<connection_size_t>(
        std::uniform_int_distribution<> {0, element_2.output_count() - 1}(rng)));

    if (input.has_connected_element() || output.has_connected_element()) {
        return false;
    }
    input.connect(output);
    return true;
}

int benchmark_simulation(const int n_elements, const int n_events, const bool print) {
    std::mt19937 rng {0};

    Circuit circuit;
    // create elements
    ranges::for_each(ranges::views::iota(0, n_elements),
                     [&](auto) { add_random_element(circuit, rng); });

    auto all_inputs
        = circuit.elements()
          | ranges::views::transform([](auto element) { return element.inputs(); })
          | ranges::views::join  //
          | ranges::to_vector;
    auto all_outputs
        = circuit.elements()
          | ranges::views::transform([](auto element) { return element.outputs(); })
          | ranges::views::join  //
          | ranges::to_vector;
    ;
    fmt::print("{}\n", all_inputs);
    fmt::print("{} x {}\n", std::size(all_inputs), std::size(all_outputs));
    // for (const auto &element : all_inputs) {
    //     fmt::print("{}\n", element);
    // }
    //| ranges::views::join  //
    //| ranges::to_vector;
    //   for (const auto &a : all_inputs) {
    //      for (const auto &i : a) {
    //          fmt::print("{}\n", i);
    //      }
    //  }

    //   add connections
    ranges::for_each(ranges::views::iota(0, n_elements), [&](auto) {
        while (!try_add_random_connection(circuit, rng)) {
        }
    });

    add_output_placeholders(circuit);
    SimulationState state {circuit};

    advance_simulation(state, circuit, defaults::until_steady, defaults::no_timeout,
                       print);
    auto output_values {get_all_output_values(state.input_values, circuit)};

    if (print) {
        std::cout << n_events;
        fmt::print("input_values = {::b}\n", state.input_values);
        fmt::print("output_values = {::b}\n", output_values);
    }

    return (state.input_values.empty() ? 0 : static_cast<int>(state.input_values.front()))
           + (output_values.empty() ? 0 : static_cast<int>(output_values.front()))
           + n_elements;
}
}  // namespace logicsim
