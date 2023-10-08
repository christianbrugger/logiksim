
#include "simulation.h"

#include "algorithm/has_duplicates_quadratic.h"
#include "algorithm/pop_while.h"
#include "algorithm/range.h"
#include "algorithm/transform_to_container.h"
#include "exception.h"
#include "logging.h"

#include <gsl/assert>
#include <gsl/gsl>

#include <algorithm>
#include <cmath>
#include <iterator>

namespace logicsim {

//
// Simulation Event
//

auto make_event(Schematic::ConstInput input, time_t time, bool value) -> SimulationEvent {
    return {.time = time,
            .element_id = input.element_id(),
            .input_index = input.input_index(),
            .value = value};
}

auto SimulationEvent::format() const -> std::string {
    return fmt::format("<SimulationEvent: at {} set Element_{}[{}] = {}>", time,
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

        const auto to_index = [](const SimulationEvent &event) {
            return event.input_index;
        };
        if (has_duplicates_quadratic(events, to_index)) {
            throw_exception(
                "Cannot have two events for the same input at the same time.");
        }
    }
}

auto set_default_outputs(Simulation &simulation) -> void {
    if (simulation.is_initialized()) [[unlikely]] {
        throw_exception("cannot set outputs for initialized simulation");
    }

    for (const auto element : simulation.schematic().elements()) {
        if (element.is_logic_item()) {
            for (auto output : element.outputs()) {
                if (output.has_connected_element()) {
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

    for (const auto element : simulation.schematic().elements()) {
        /*
        // and-elements
        if (element.element_type() == ElementType::and_element) {
            for (auto input : element.inputs()) {
                if (!input.has_connected_element()) {
                    simulation.set_input_value(input, true);
                }
            }
        }
        */

        // activate unconnected enable
        if (has_enable(element.element_type())) {
            const auto input = element.input(connection_id_t {0});
            if (!input.has_connected_element() && !input.is_inverted()) {
                simulation.set_input_value(input, true);
            }
        }

        // activate unconnected j&k flipflops
        if (element.element_type() == ElementType::flipflop_jk) {
            const auto input_1 = element.input(connection_id_t {1});
            const auto input_2 = element.input(connection_id_t {2});
            if (!input_1.has_connected_element() && !input_1.is_inverted() &&
                !input_2.has_connected_element() && !input_2.is_inverted()) {
                simulation.set_input_value(input_1, true);
                simulation.set_input_value(input_2, true);
            }
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

[[nodiscard]] constexpr auto has_no_logic(const ElementType type) noexcept -> bool {
    using enum ElementType;
    return type == placeholder || type == led || type == display_ascii ||
           type == display_number;
}

[[nodiscard]] constexpr auto internal_state_size(const ElementType type) noexcept
    -> std::size_t {
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
    throw_exception("Dont know internal state of given type.");
}

[[nodiscard]] constexpr auto has_internal_state(const ElementType type) noexcept -> bool {
    return internal_state_size(type) != 0;
}

Simulation::Simulation(const Schematic &schematic)
    : schematic_ {&schematic},
      queue_ {},
      largest_history_event_ {queue_.time()},
      is_initialized_ {false} {
    input_values_.reserve(schematic.element_count());
    internal_states_.reserve(schematic.element_count());

    for (auto element : schematic.elements()) {
        input_values_.emplace_back(element.input_count().count(), false);
        internal_states_.emplace_back(internal_state_size(element.element_type()), false);
    }
    first_input_histories_.resize(schematic.element_count());
}

auto Simulation::schematic() const noexcept -> const Schematic & {
    return *schematic_;
}

auto Simulation::time() const noexcept -> time_t {
    return queue_.time();
}

auto Simulation::submit_event(Schematic::ConstInput input, delay_t offset, bool value)
    -> void {
    queue_.submit_event(make_event(input, time_t {queue_.time() + offset}, value));
}

auto Simulation::submit_events(Schematic::ConstElement element, delay_t offset,
                               logic_small_vector_t values) -> void {
    if (connection_count_t {std::size(values)} != element.input_count()) [[unlikely]] {
        throw_exception("Need to provide number of input values.");
    }
    for (auto input : element.inputs()) {
        const auto value = values.at(std::size_t {input.input_index()});
        submit_event(input, offset, value);
    }
}

auto Simulation::check_counts_valid() const -> void {
    const auto n_elements =
        static_cast<logic_vector_t::size_type>(schematic_->element_count());

    if (input_values_.size() != n_elements || internal_states_.size() != n_elements ||
        first_input_histories_.size() != n_elements) [[unlikely]] {
        throw_exception("size of vector match schematic element count.");
    }
}

namespace {

template <bool Const>
struct state_mapping_clock_generator {
    using vector_ref =
        std::conditional_t<!Const, logic_small_vector_t &, const logic_small_vector_t &>;
    using bool_ref = std::conditional_t<!Const, bool &, const bool &>;

    explicit state_mapping_clock_generator(vector_ref state)
        : enabled {state.at(0)},
          output_value {state.at(1)},
          on_finish_event {state.at(2)},
          off_finish_event {state.at(3)} {
        if (state.size() != 4) {
            throw_exception("invalid state size");
        }
    }

    bool_ref enabled;
    bool_ref output_value;
    bool_ref on_finish_event;
    bool_ref off_finish_event;
};

}  // namespace

auto update_internal_state(const logic_small_vector_t &old_input,
                           const logic_small_vector_t &new_input, const ElementType type,
                           logic_small_vector_t &state) {
    switch (type) {
        using enum ElementType;

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
                throw_exception(
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
            [[unlikely]] throw_exception(
                "Unexpected type encountered in calculate_new_state.");
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
                throw_exception(
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
            [[unlikely]] throw_exception(
                "Unexpected type encountered in calculate_new_state.");
    }
}

auto calculate_outputs_from_inputs(const logic_small_vector_t &input,
                                   connection_count_t output_count,
                                   const ElementType type) -> logic_small_vector_t {
    if (input.empty()) [[unlikely]] {
        throw_exception("Input size cannot be zero.");
    }
    if (output_count <= connection_count_t {0}) [[unlikely]] {
        throw_exception("Output count cannot be zero or negative.");
    }

    switch (type) {
        using enum ElementType;

        case wire:
            return logic_small_vector_t(output_count.count(), input.at(0));

        case buffer_element:
            return {input.at(0)};

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

auto Simulation::apply_events(const Schematic::ConstElement element,
                              const event_group_t &group) -> void {
    for (const auto &event : group) {
        set_input_internal(element.input(event.input_index), event.value);
    }
}

auto get_changed_outputs(const logic_small_vector_t &old_outputs,
                         const logic_small_vector_t &new_outputs)
    -> Simulation::con_index_small_vector_t {
    if (std::size(old_outputs) != std::size(new_outputs)) [[unlikely]] {
        throw_exception("old_outputs and new_outputs need to have the same size.");
    }

    Simulation::con_index_small_vector_t result;
    for (auto index :
         range(gsl::narrow<connection_id_t::value_type>(std::size(old_outputs)))) {
        if (old_outputs[index] != new_outputs[index]) {
            result.push_back(connection_id_t {index});
        }
    }
    return result;
}

void Simulation::create_event(const Schematic::ConstOutput output,
                              const logic_small_vector_t &output_values) {
    if (output.has_connected_element()) {
        queue_.submit_event(
            {.time = queue_.time() + output.delay(),
             .element_id = output.connected_element_id(),
             .input_index = output.connected_input_index(),
             .value = output_values.at(std::size_t {output.output_index()})});
    }
}

auto Simulation::submit_events_for_changed_outputs(
    const Schematic::ConstElement element, const logic_small_vector_t &old_outputs,
    const logic_small_vector_t &new_outputs) -> void {
    const auto changes = get_changed_outputs(old_outputs, new_outputs);
    for (auto output_index : changes) {
        create_event(element.output(output_index), new_outputs);
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

auto Simulation::process_event_group(event_group_t &&events) -> void {
    if (print_events) {
        print_fmt("events: {:n}\n", events);
    }
    if (events.empty()) {
        return;
    }
    validate(events);
    const auto element =
        Schematic::ConstElement {schematic_->element(events.front().element_id)};
    const auto element_type = element.element_type();

    // short-circuit trivial elements
    if (has_no_logic(element_type)) {
        apply_events(element, events);
        return;
    }

    // update inputs
    auto old_inputs = input_values(element);
    apply_events(element, events);
    auto new_inputs = input_values(element);

    const auto inverters = element.input_inverters();
    if (std::ranges::any_of(inverters, std::identity {})) {
        invert_inputs(old_inputs, inverters);
        invert_inputs(new_inputs, inverters);
    }

    if (has_internal_state(element_type)) {
        auto &internal_state = internal_states_.at(element.element_id().value);

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

namespace simulation {

class Timer {
   public:
    using time_point = timeout_clock::time_point;

    explicit Timer(timeout_t timeout = Simulation::defaults::no_timeout) noexcept
        : timeout_(timeout), start_time_(timeout_clock::now()) {};

    [[nodiscard]] auto reached_timeout() const noexcept -> bool {
        return (timeout_ != Simulation::defaults::no_timeout) &&
               (std::chrono::steady_clock::now() - start_time_) > timeout_;
    }

   private:
    timeout_t timeout_;
    time_point start_time_;
};
}  // namespace simulation

auto Simulation::run(const delay_t simulation_time, const timeout_t timeout,
                     const int64_t max_events) -> int64_t {
    if (!is_initialized_) {
        throw_exception("Simulation first needs to be initialized.");
    }
    if (simulation_time <= delay_t {0us}) [[unlikely]] {
        throw_exception("simulation_time needs to be positive.");
    }
    if (max_events < 0) [[unlikely]] {
        throw_exception("max events needs to be positive or zero.");
    }
    check_counts_valid();

    if (simulation_time == delay_t {0us}) {
        return 0;
    }

    const auto timer = simulation::Timer {timeout};
    const auto queue_end_time = simulation_time == defaults::infinite_simulation_time
                                    ? time_t::max()
                                    : queue_.time() + simulation_time;
    int64_t event_count = 0;

    // only check time after this many events
    constexpr int64_t check_interval = 1'000;
    int64_t next_check = std::min(max_events, timeout == Simulation::defaults::no_timeout
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

    if (simulation_time != defaults::infinite_simulation_time) {
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
    if (!queue_.empty()) [[unlikely]] {
        throw_exception("Cannot initialize simulation with scheduled events.");
    }
    check_counts_valid();

    for (auto &&element : schematic_->elements()) {
        auto element_type = element.element_type();

        if (element.is_wire()) {
            continue;
        }

        if (element.output_count() == connection_count_t {0}) {
            continue;
        }

        // existing outputs without inverters
        const auto old_outputs = transform_to_container<logic_small_vector_t>(
            element.outputs(),
            [this](auto output) { return input_value(output.connected_input()); });

        if (has_internal_state(element_type)) {
            const auto new_inputs = input_values(element);
            if (std::ranges::any_of(new_inputs, std::identity {})) {
                const auto old_inputs = logic_small_vector_t(new_inputs.size(), false);
                auto &internal_state = internal_states_.at(element.element_id().value);
                update_internal_state(old_inputs, new_inputs, element_type,
                                      internal_state);
            }

            const auto new_outputs = calculate_outputs_from_state(
                internal_state(element), element.output_count(), element_type);
            submit_events_for_changed_outputs(element, old_outputs, new_outputs);

        } else {
            auto curr_inputs = input_values(element);
            invert_inputs(curr_inputs, element.input_inverters());
            const auto new_outputs = calculate_outputs_from_inputs(
                curr_inputs, element.output_count(), element_type);

            submit_events_for_changed_outputs(element, old_outputs, new_outputs);
        }
    }

    is_initialized_ = true;
}

auto Simulation::is_initialized() const -> bool {
    return is_initialized_;
}

auto Simulation::record_input_history(const Schematic::ConstInput input,
                                      const bool new_value) -> void {
    // we only record the first input, as we only need a history for wires
    if (input.input_index() != connection_id_t {0}) {
        return;
    }
    const auto history_length = input.element().history_length();

    if (history_length <= delay_t {0ns}) {
        return;
    }
    if (new_value == input_value(input)) {
        return;
    }
    auto &history = first_input_histories_.at(input.element_id().value);
    if (!history.empty() && history.back() == time()) {
        throw_exception("Cannot have two transitions recorded at the same time.");
    }

    // remove old values
    clean_history(history, history_length);

    // add new entry
    history.push_back(time());

    // update largest history event
    largest_history_event_ = std::max(largest_history_event_, time() + history_length);
}

auto Simulation::clean_history(history_buffer_t &history, delay_t history_length)
    -> void {
    const auto min_time = time() - history_length;
    while (!history.empty() && history.front() < min_time) {
        history.pop_front();
    }
}

auto Simulation::input_value(element_id_t element_id, connection_id_t index) const
    -> bool {
    return input_values_.at(element_id.value).at(std::size_t {index});
}

auto Simulation::input_value(const Schematic::ConstInput input) const -> bool {
    return input_value(input.element_id(), input.input_index());
}

auto Simulation::input_values(element_id_t element_id) const
    -> const logic_small_vector_t & {
    return input_values_.at(element_id.value);
}

auto Simulation::input_values(Schematic::Element element) const
    -> const logic_small_vector_t & {
    return input_values(element.element_id());
}

auto Simulation::input_values(const Schematic::ConstElement element) const
    -> const logic_small_vector_t & {
    return input_values(element.element_id());
}

auto Simulation::input_values() const -> const logic_vector_t {
    auto result = logic_vector_t {};

    for (auto element : schematic_->elements()) {
        std::ranges::copy(input_values(element), std::back_inserter(result));
    }

    return result;
}

auto Simulation::set_input_internal(const Schematic::ConstInput input, bool value)
    -> void {
    record_input_history(input, value);

    input_values_.at(input.element_id().value).at(std::size_t {input.input_index()}) =
        value;
}

auto Simulation::set_output_value(Schematic::ConstOutput output, bool value) -> void {
    if (is_initialized_) {
        throw_exception("can only set outputs at the start of the simulation");
    }

    const auto input = output.connected_input();

    auto &input_value =
        input_values_.at(input.element_id().value).at(std::size_t {input.input_index()});
    input_value = value ^ input.is_inverted();
}

auto Simulation::output_value(element_id_t element_id, connection_id_t index) const
    -> bool {
    return output_value(schematic_->element(element_id).output(index));
}

auto Simulation::output_value(const Schematic::ConstOutput output) const -> bool {
    const auto input = output.connected_input();
    return input_value(input) ^ input.is_inverted();
}

auto Simulation::output_values(element_id_t element_id) const -> logic_small_vector_t {
    return output_values(schematic_->element(element_id));
}

auto Simulation::output_values(Schematic::Element element) const -> logic_small_vector_t {
    return output_values(Schematic::ConstElement {element});
}

auto Simulation::output_values(const Schematic::ConstElement element) const
    -> logic_small_vector_t {
    return transform_to_container<logic_small_vector_t>(
        element.outputs(), [=, this](auto output) { return output_value(output); });
}

auto Simulation::output_values() const -> logic_vector_t {
    logic_vector_t result(schematic_->total_output_count());

    for (auto element : schematic_->elements()) {
        std::ranges::copy(output_values(element), std::back_inserter(result));
    }

    return result;
}

auto Simulation::set_input_value(Schematic::ConstInput input, bool value) -> void {
    if (input.has_connected_element()) [[unlikely]] {
        throw_exception("cannot input-value for connected inputs");
    }
    auto &input_value =
        input_values_.at(input.element_id().value).at(std::size_t {input.input_index()});

    if (!is_initialized_) {
        input_value = value;
    } else {
        if (input_value != value) {
            submit_event(input, delay_t::epsilon(), value);
            run_infinitesimal();
        }
    }
}

auto Simulation::set_internal_state(Schematic::ConstElement element, std::size_t index,
                                    bool value) -> void {
    auto &state = internal_states_.at(element.element_id().value).at(index);

    if (!is_initialized_) {
        state = value;
    } else {
        const auto old_outputs = calculate_outputs_from_state(
            internal_state(element), element.output_count(), element.element_type());
        state = value;
        const auto new_outputs = calculate_outputs_from_state(
            internal_state(element), element.output_count(), element.element_type());

        submit_events_for_changed_outputs(element, old_outputs, new_outputs);
        run_infinitesimal();
    }
}

auto Simulation::internal_state(element_id_t element_id) const
    -> const logic_small_vector_t & {
    return internal_states_.at(element_id.value);
}

auto Simulation::internal_state(Schematic::Element element) const
    -> const logic_small_vector_t & {
    return internal_state(element.element_id());
}

auto Simulation::internal_state(Schematic::ConstElement element) const
    -> const logic_small_vector_t & {
    return internal_state(element.element_id());
}

auto Simulation::internal_state(Schematic::ConstElement element, std::size_t index) const
    -> bool {
    return internal_state(element).at(index);
}

auto Simulation::input_history(element_id_t element_id) const -> HistoryView {
    return input_history(schematic_->element(element_id));
}

auto Simulation::input_history(Schematic::Element element) const -> HistoryView {
    return input_history(Schematic::ConstElement {element});
}

auto Simulation::input_history(Schematic::ConstElement element) const -> HistoryView {
    const auto &input_values = this->input_values(element);

    if (input_values.empty()) {
        return HistoryView {};
    }

    const auto last_value =
        static_cast<bool>(input_values.at(0) ^ element.input_inverters().at(0));

    return HistoryView {
        first_input_histories_.at(element.element_id().value),
        this->time(),
        last_value,
        element.history_length(),
    };
}

//
// History View
//

namespace simulation {

HistoryView::HistoryView(const history_buffer_t &history, time_t simulation_time,
                         bool last_value, delay_t history_length)
    : history_ {&history}, simulation_time_ {simulation_time}, last_value_ {last_value} {
    // ascending without duplicates
    assert(std::ranges::is_sorted(history, std::ranges::less_equal {}));
    // calculate first valid index
    const auto first_time = simulation_time - history_length;
    const auto first_index = find_index(first_time);
    min_index_ = gsl::narrow<decltype(min_index_)>(first_index);

    assert(size() >= 1);
}

auto HistoryView::size() const -> std::size_t {
    if (history_ == nullptr) {
        return 1;
    }
    return history_->size() + 1 - min_index_;
}

auto HistoryView::ssize() const -> std::ptrdiff_t {
    if (history_ == nullptr) {
        return 1;
    }
    return history_->size() + 1 - min_index_;
}

auto HistoryView::begin() const -> HistoryIterator {
    return HistoryIterator {*this, min_index_};
}

auto HistoryView::end() const -> HistoryIterator {
    return HistoryIterator {*this, size() + min_index_};
}

auto HistoryView::from(time_t value) const -> HistoryIterator {
    if (value > simulation_time_) [[unlikely]] {
        throw_exception("cannot query times in the future");
    }
    const auto index = find_index(value);
    return HistoryIterator {*this, index};
}

auto HistoryView::until(time_t value) const -> HistoryIterator {
    if (value > simulation_time_) [[unlikely]] {
        throw_exception("cannot query times in the future");
    }

    const auto last_time = value > time_t::min()  //
                               ? value - delay_t::epsilon()
                               : value;
    const auto index = find_index(last_time) + 1;
    return HistoryIterator {*this, index};
}

auto HistoryView::value(time_t value) const -> bool {
    if (value > simulation_time_) [[unlikely]] {
        throw_exception("cannot query times in the future");
    }
    const auto index = find_index(value);
    return get_value(index);
}

auto HistoryView::last_value() const -> bool {
    return last_value_;
}

auto HistoryView::get_value(std::size_t history_index) const -> bool {
    if (history_ == nullptr) {
        if (history_index != 0) [[unlikely]] {
            throw_exception("invalid history index");
        }
        return false;
    }

    auto number = history_->size() - history_index;
    return static_cast<bool>(number % 2) ^ last_value_;
}

// Returns the index to the first element that is greater to the value,
// or the history.size() if no such element is found.
auto HistoryView::find_index(time_t value) const -> std::size_t {
    if (history_ == nullptr) {
        return 0;
    }

    const auto it =
        std::ranges::lower_bound(history_->begin() + min_index_, history_->end(), value,
                                 std::ranges::less_equal {});
    const auto index = it - history_->begin();

    assert(index >= min_index_);
    assert(index <= std::ssize(*history_));
    assert(index == std::ssize(*history_) || history_->at(index) > value);
    assert(index == min_index_ || history_->at(index - 1) <= value);

    return gsl::narrow_cast<std::size_t>(index);
}

auto HistoryView::get_time(std::ptrdiff_t index) const -> time_t {
    if (history_ == nullptr) {
        return index < 0 ? time_t::min() : simulation_time_;
    }

    if (index < min_index_) {
        return time_t::min();
    }
    if (index >= std::ssize(*history_)) {
        return simulation_time_;
    }
    return history_->at(index);
}
}  // namespace simulation

//
// History Iterator
//

namespace simulation {

auto history_entry_t::format() const -> std::string {
    return fmt::format("HistoryEntry({}, {}, {})", first_time, last_time, value);
}

HistoryIterator::HistoryIterator(HistoryView view, std::size_t index) noexcept
    : view_ {std::move(view)}, index_ {index} {}

auto HistoryIterator::operator*() const -> value_type {
    return history_entry_t {
        .first_time = view_.get_time(static_cast<std::ptrdiff_t>(index_) - 1),
        .last_time = view_.get_time(index_),
        .value = view_.get_value(index_),
    };
}

auto HistoryIterator::operator++() noexcept -> HistoryIterator & {
    ++index_;
    return *this;
}

auto HistoryIterator::operator++(int) noexcept -> HistoryIterator {
    const auto tmp = *this;
    ++(*this);
    return tmp;
}

auto HistoryIterator::operator==(const HistoryIterator &right) const noexcept -> bool {
    return index_ >= right.index_;
}

auto HistoryIterator::operator-(const HistoryIterator &right) const noexcept
    -> difference_type {
    return static_cast<std::ptrdiff_t>(index_) -
           static_cast<std::ptrdiff_t>(right.index_);
}

}  // namespace simulation

}  // namespace logicsim
