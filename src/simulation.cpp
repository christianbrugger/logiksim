
#include "simulation.h"

#include <range/v3/all.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <functional>


namespace logicsim {

    //
    // Simulation Event
    //

    std::string SimulationEvent::format() const
    {
        auto element_id_str { (element_id == null_element) ? "NULL" : fmt::format("{}", element_id) };
        return fmt::format("<SimulationEvent: at {}s set Element_{}[{}] = {}>",
            time, element_id_str, input_index, (value ? "true" : "false"));
    }


    bool SimulationEvent::operator==(const SimulationEvent& other) const
    {
        return this->element_id == other.element_id && this->time == other.time;
    }
    bool SimulationEvent::operator<(const SimulationEvent& other) const
    {
        if (this->time == other.time)
            return this->element_id < other.element_id;
        else
            return this->time < other.time;
    }


    bool SimulationEvent::operator!=(const SimulationEvent& other) const
    {
        return !(this->operator==(other));
    }
    bool SimulationEvent::operator>(const SimulationEvent& other) const
    {
        return other.operator<(*this);
    }
    bool SimulationEvent::operator<=(const SimulationEvent& other) const
    {
        return !(this->operator>(other));
    }
    bool SimulationEvent::operator>=(const SimulationEvent& other) const
    {
        return !(this->operator<(other));
    }

    //
    // EventGroup
    //

    void validate(const event_group_t& events) {
        if (events.size() == 0)
            return;

        const auto& head { events.front() };
        const auto tail { ranges::views::drop(events, 1) };  // TODO use tail

        if (head.element_id == null_element)
            throw_exception("Event element cannot be null.");

        if (!std::isfinite(head.time))
            throw_exception("Event time needs to be finite.");

        if (tail.size() > 0) {
            if (!ranges::all_of(tail, [head](const auto& event) { return event.time == head.time;  }))
                throw_exception("All events in the group need to have the same time.");

            if (!ranges::all_of(tail, [head](const auto& event) { return event.element_id == head.element_id;  }))
                throw_exception("All events in the group need to have the same time.");

            if (has_duplicates_quadratic(ranges::views::transform(events, [](const auto& event) { return event.input_index; })))
                throw_exception("Cannot have two events for the same input at the same time.");
        }

    }

    //
    // SimulationQueue
    //


    time_t SimulationQueue::time() const noexcept {
        return time_;
    }

    void SimulationQueue::set_time(time_t time) {
        if (!std::isfinite(time))
            throw_exception("New time needs to be finite.");
        if (time < time_)
            throw_exception("Cannot set new time to the past.");
        if (time > next_event_time())
            throw_exception("New time would be greater than next event.");

        time_ = time;
    }

    time_t SimulationQueue::next_event_time() const noexcept {
        return events_.empty()?std::numeric_limits<time_t>::infinity():events_.top().time;
    }

    bool SimulationQueue::empty() const noexcept {
        return events_.empty();
    }

    void SimulationQueue::submit_event(SimulationEvent&& event) {
        if (event.time <= time_)
            throw_exception("Event time needs to be in the future.");
        if (!std::isfinite(event.time))
            throw_exception("Event time needs to be finite.");

        events_.push(std::move(event));
    }

    event_group_t SimulationQueue::pop_event_group()
    {
        event_group_t group;
        pop_while(
            events_,
            [&group](const SimulationEvent& event) { group.push_back(event); },
            [&group](const SimulationEvent& event) { return group.size() == 0 || group.front() == event; }
        );
        if (group.size() > 0) {
            set_time(group.front().time);
        }
        return group;
    }

    //
    // SimulationState
    //


    SimulationState::SimulationState(connection_id_t total_inputs) : 
        input_values(total_inputs), 
        queue {} 
    {
    }

    //
    // Simulation
    //

    using logic_small_vector_t = boost::container::small_vector<bool, 8>;
    using con_index_small_vector_t = boost::container::small_vector<connection_size_t, 8>;


    logic_small_vector_t calculate_outputs(const logic_small_vector_t& input, const ElementType type) {
        if (input.size() == 0)
            return {};

        switch (type) {
        case ElementType::wire:
            return { input.at(0) };

        case ElementType::inverter_element:
            return { !input.at(0) };

        case ElementType::and_element:
            return { ranges::all_of(input, std::identity{}) };

        case ElementType::or_element:
            return { ranges::any_of(input, std::identity{}) };

        case ElementType::xor_element:
            return { ranges::count_if(input, std::identity{}) == 1 };

        default:
            return {};

        }
    }

    logic_small_vector_t copy_inputs(const logic_vector_t &input_values, const Circuit::ConstElement element) {
        return ranges::views::all(input_values)
            | ranges::views::drop(element.first_input_id())
            | ranges::views::take(element.input_count())
            | ranges::to<logic_small_vector_t>();
    }

    void set_input(logic_vector_t& input_values, const Circuit::ConstElement element, connection_size_t input_index, bool value) {
        const auto input_id = static_cast<logic_vector_t::size_type>(element.input_id(input_index));
        input_values.at(input_id) = value;
    }

    void apply_events(logic_vector_t& input_values, const Circuit::ConstElement element, const event_group_t& group) {
        ranges::for_each(group, [&](const SimulationEvent& event) {
            set_input(input_values, element, event.input_index, event.value);
        });
    }

    con_index_small_vector_t get_changed_outputs(const logic_small_vector_t& old_outputs, const logic_small_vector_t& new_outputs) {
        if (std::size(old_outputs) != std::size(new_outputs)) [[unlikely]]
            throw_exception("old_outputs and new_outputs need to have the same size.");

        con_index_small_vector_t result;
        for (const auto&& [index, old_value, new_value] : ranges::views::zip(ranges::views::iota(0), old_outputs, new_outputs)) {
            if (old_value != new_value) {
                result.push_back(static_cast<connection_size_t>(index));
            }
        }
        return result;
    }

    constexpr time_t STANDARD_DELAY = 0.1;

    void create_event(SimulationQueue& queue, Circuit::ConstOutput output, const logic_small_vector_t& output_values) {
        const time_t time { queue.time() + STANDARD_DELAY };

        if (output.has_connected_element()) {
            queue.submit_event({ 
                .time=time, 
                .element_id=output.connected_element_id(), 
                .input_index=output.connected_input_index(), 
                .value=output_values.at(output.output_index())
            });
        }
    }

    void process_event_group(
        SimulationState& state, const Circuit& circuit, 
        event_group_t&& events, bool print_events = false) 
    {
        if (events.size() == 0)
            return;
        validate(events);

        if (print_events) {
            fmt::print("events: {:n:}\n", events);
        }

        const Circuit::ConstElement element { circuit.element(events.front().element_id) };

        // short-circuit input placeholders
        if (element.element_type() == ElementType::placeholder) {
            apply_events(state.input_values, element, events);
            return;
        }

        // update inputs
        const auto old_inputs { copy_inputs(state.input_values, element) };
        apply_events(state.input_values, element, events);
        const auto new_inputs { copy_inputs(state.input_values, element) };

        // find changing outputs
        const auto old_outputs { calculate_outputs(old_inputs, element.element_type()) };
        const auto new_outputs { calculate_outputs(new_inputs, element.element_type()) };
        const auto changes { get_changed_outputs(old_outputs, new_outputs) };

        // submit events
        ranges::for_each(changes, [&, element](auto output_index) { 
            create_event(state.queue, element.output(output_index), new_outputs); 
        });
    }
    

    void advance_simulation(
        SimulationState &state, const Circuit& circuit, 
        time_t time_delta, bool print_events) 
    {
        if (time_delta < 0) [[unlikely]]
            throw_exception("time_delta needs to be zero or positive.");
        if (state.input_values.size() != 
                static_cast<logic_vector_t::size_type>(circuit.total_input_count())) [[unlikely]]
            throw_exception("input_values in state needs to match total inputs of the circuit.");

        const double end_time { (time_delta == 0) ? 
            std::numeric_limits<time_t>::infinity() : state.queue.time() + time_delta };

        while (!state.queue.empty() && state.queue.next_event_time() < end_time) {
            process_event_group(state, circuit, state.queue.pop_event_group(), print_events);
        }

        if (time_delta > 0) {
            state.queue.set_time(end_time);
        }
    }

    bool get_output_value(
        const Circuit::ConstOutput output, 
        const logic_vector_t& input_values, 
        const bool raise_missing
    ) {
        if (raise_missing || output.has_connected_element()) {
            return input_values.at(output.connected_input().input_id());
        }
        return false;
    }

    logic_vector_t output_value_vector(const logic_vector_t& input_values, const Circuit& circuit, 
        const bool raise_missing)
    {
        logic_vector_t output_values(circuit.total_output_count());

        for (auto element : circuit.elements()) {
            for (auto output : element.outputs()) {
                output_values.at(output.output_id()) = get_output_value(output, input_values, raise_missing);
            }
        }

        return output_values;
    }

    //
    // Benchmark
    //

    int benchmark_simulation(const int n_elements, bool print) {
        Circuit circuit;

        const auto elem0 { circuit.create_element(ElementType::or_element, 2, 1) };
        elem0.output(0).connect(elem0.input(0));
        

        SimulationState state { circuit.total_input_count() };
        state.queue.submit_event({ 0.1, elem0.element_id(), 0, true});
        state.queue.submit_event({ 0.1, elem0.element_id(), 1, true});
        state.queue.submit_event({ 0.5, elem0.element_id(), 1, false });

        create_output_placeholders(circuit);
        advance_simulation(state, circuit, 0, print);
        auto output_values { output_value_vector(state.input_values, circuit) };

        if (print) {
            fmt::print("input_values = {::b}\n", state.input_values);
            fmt::print("output_values = {::b}\n", output_values);
        }

        return state.input_values.front() + output_values.front() + n_elements;
    }
}

