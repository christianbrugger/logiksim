
#include "simulation.h"

#include <boost/range/combine.hpp>
#include <boost/range/adaptors.hpp>

#include <format>
#include <algorithm>
#include <functional>


namespace logicsim {

    std::string SimulationEvent::format() const
    {
        auto element_id_str { (element_id == null_element) ? "NULL" : std::format("{}", element_id) };
        return std::format("<SimulationEvent: at {}s set Element_{}[{}] = {}>",
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

            if (has_duplicates_quadratic(ranges::views::transform(events, [](const auto& event){ return event.input_index; })))
                throw_exception("Cannot have two events for the same input at the same time.");
        }

    }


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
        // TODO use to vector
        const auto view { input_values | ranges::views::drop(element.first_input_id()) | ranges::views::take(element.input_count()) };
        return { std::cbegin(view), std::cend(view) };
    }

    void set_input(logic_vector_t& input_values, const Circuit::ConstElement element, connection_size_t input_index, bool value) {
        input_values.at(static_cast<logic_vector_t::size_type>(element.input_id(input_index))) = value;
    }

    void apply_events(logic_vector_t& input_values, const Circuit::ConstElement element, const event_group_t& group) {
        ranges::for_each(group, [&input_values, element](const SimulationEvent& event) {
            set_input(input_values, element, event.input_index, event.value);
        });
    }

    con_index_small_vector_t get_changed_outputs(const logic_small_vector_t& old_outputs, const logic_small_vector_t& new_outputs) {
        boost::container::small_vector<connection_size_t, 8> result;

        for (auto&& element : boost::range::combine(old_outputs, new_outputs) | boost::adaptors::indexed()) {
            if (element.value().get<0>() != element.value().get<1>()) {
                result.push_back(static_cast<connection_size_t>(element.index()));
            }
        }
        return result;
    }

    constexpr time_t STANDARD_DELAY = 0.1;

    void create_event(SimulationQueue& queue, Circuit::ConstOutputConnection output, const logic_small_vector_t& output_values) {
        const time_t time { queue.time() + STANDARD_DELAY };

        if (output.has_connected_element()) {
            queue.submit_event({ 
                time, 
                output.connected_element_id(), 
                output.connected_input_index(), 
                output_values.at(output.output_index())
            });
        }
    }

    void process_event_group(SimulationState& state, const event_group_t& events, const Circuit& circuit, bool print_events = false) {
        if (events.size() == 0)
            return;
        validate(events);

        if (print_events) {
            std::cout << std::format("events: {}\n", events);
        }

        const auto element { circuit.element(events.front().element_id) };

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
    

    SimulationState advance_simulation(SimulationState old_state, const Circuit& circuit, time_t time_delta, bool print_events) {
        if (time_delta < 0) [[unlikely]]
            throw_exception("time_delta needs to be zero or positive.");
        SimulationState state { std::move(old_state), circuit.total_input_count() };

        const double end_time { (time_delta == 0) ? std::numeric_limits<time_t>::infinity(): state.queue.time() + time_delta };

        while (!state.queue.empty() && state.queue.next_event_time() < end_time) {
            process_event_group(state, state.queue.get_event_group(), circuit, print_events);
        }

        if (time_delta > 0) {
            state.queue.set_time(end_time);
        }
        return state;
    }

    logic_vector_t collect_output_values(const logic_vector_t& input_values, const Circuit& circuit) {
        logic_vector_t output_values(circuit.total_output_count());

        // TODO refactor loops
        for (auto element : circuit.elements()) {
            for (auto output : element.outputs()) {
                if (output.has_connected_element()) {
                    output_values.at(output.output_id()) = input_values.at(output.connected_input().input_id());
                }
            }
        }

        return output_values;
    }

    int benchmark_simulation(const int n_elements, bool print) {
        Circuit circuit;

        const auto elem0 { circuit.create_element(ElementType::or_element, 2, 1) };
        elem0.output(0).connect(elem0.input(0));
        

        SimulationState state;
        state.queue.submit_event({ 0.1, elem0.element_id(), 0, true});
        // state.queue.submit_event({ 0.1, elem0, 0, true });
        state.queue.submit_event({ 0.5, elem0.element_id(), 1, false });

        create_output_placeholders(circuit);
        auto new_state { advance_simulation(state, circuit, 0, print) };
        auto output_values { collect_output_values(new_state.input_values, circuit) };

        if (print) {
            for (bool input : new_state.input_values) {
                std::cout << std::format("input_values = {}\n", input);
            }
            for (bool output : output_values) {
                std::cout << std::format("output_values = {}\n", output);
            }
            // std::cout << std::format("input_values = {}", std::vector<bool>(std::begin(new_state.input_values), std::end(new_state.input_values)));
            // std::cout << std::format("output_values = {}", std::vector<bool>(std::begin(output_values), std::end(output_values)));
        }

        return new_state.input_values.front() + output_values.front() + n_elements;
    }
}

