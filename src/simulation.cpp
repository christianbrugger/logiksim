
#include "simulation.h"

#include <boost/range/combine.hpp>
#include <boost/range/adaptors.hpp>

#include <format>
#include <ranges>
#include <algorithm>
#include <functional>


namespace logicsim {

    std::string SimulationEvent::format() const
    {
        auto element_id_str = (element == null_element) ? "NULL" : std::format("{}", element);
        return std::format("<SimulationEvent: at {}s set Element_{}[{}] = {}>",
            time, element_id_str, input, (value ? "true" : "false"));
    }


    bool SimulationEvent::operator==(const SimulationEvent& other) const
    {
        return this->element == other.element && this->time == other.time;
    }
    bool SimulationEvent::operator<(const SimulationEvent& other) const
    {
        if (this->time == other.time)
            return this->element < other.element;
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


    void validate(const event_group_t& events, const ElementInputConfig config) {
        if (events.size() == 0)
            return;

        const auto& head = events.front();
        const auto tail = std::views::drop(events, 1);

        if (head.element == null_element)
            throw_exception("Event element cannot be null.");

        if (!(head.time < std::numeric_limits<time_t>::infinity()))
            throw_exception("Event time cannot be infinite.");

        if (tail.size() > 0) {
            if (!std::ranges::all_of(tail, [head](const auto& event) { return event.time == head.time;  }))
                throw_exception("All events in the group need to have the same time.");

            if (!std::ranges::all_of(tail, [head](const auto& event) { return event.element == head.element;  }))
                throw_exception("All events in the group need to have the same time.");
        }

        std::ranges::for_each(events, [count = config.input_count](const auto& event) {
            if (event.input < 0 || event.input >= count)
                throw_exception("Event input is out of range.");
            });
    }


    using logic_small_vector_t = boost::container::small_vector<bool, 8>;
    using con_index_small_vector_t = boost::container::small_vector<connection_size_t, 8>;


    logic_small_vector_t get_logic_output(const logic_small_vector_t& input, const ElementType type) {
        if (input.size() == 0)
            return {};

        switch (type) {
        case ElementType::wire:
            return { input.at(0) };

        case ElementType::inverter_element:
            return { !input.at(0) };

        case ElementType::and_element:
            return { std::ranges::all_of(input, std::identity{}) };

        case ElementType::or_element:
            return { std::ranges::any_of(input, std::identity{}) };

        case ElementType::xor_element:
            return { std::ranges::count_if(input, std::identity{}) == 1 };

        }
        return {};
    }


    logic_small_vector_t copy_inputs(const logic_vector_t &input_values, const ElementInputConfig& config) {
        const auto view = input_values | std::views::drop(config.input_index) | std::views::take(config.input_count);
        return { std::cbegin(view), std::cend(view) };
    }

    void apply_events(logic_vector_t& input_values, const event_group_t& group) {
        std::ranges::for_each(group, [&input_values](const SimulationEvent& event) {
            input_values.at(event.input) = event.value; });
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

    void create_event(SimulationQueue& queue, const CircuitGraph& graph, element_size_t element, connection_size_t output, const logic_small_vector_t& output_values) {
        const time_t time = queue.time() + STANDARD_DELAY;
        const auto con_element = graph.get_connected_element(element, output);
        if (con_element != null_element) {
            const auto con_input = graph.get_connected_input(element, output);
            if (con_input != null_connection) {
                queue.submit_event({ time, con_element, con_input, output_values.at(output) });
            }
        }
    }

    void process_event_group(SimulationState& state, const event_group_t& events, const CircuitGraph& graph, bool print_events = false) {
        if (events.size() == 0)
            return;

        if (print_events) {
            std::cout << std::format("events: {}\n", events);
        }

        const auto element{ events.front().element };
        const auto element_config{ graph.get_input_config(element) };
        validate(events, element_config);

        // update inputs
        const auto old_inputs = copy_inputs(state.input_values, element_config);
        apply_events(state.input_values, events);
        const auto new_inputs = copy_inputs(state.input_values, element_config);

        // find changing outputs
        const auto old_outputs = get_logic_output(old_inputs, element_config.type);
        const auto new_outputs = get_logic_output(new_inputs, element_config.type);
        const auto changes = get_changed_outputs(old_outputs, new_outputs);

        // submit events
        std::ranges::for_each(changes, [&, element](auto output) { create_event(state.queue, graph, element, output, new_outputs); });
    }
    

    SimulationState advance_simulation(SimulationState old_state, const CircuitGraph& graph, time_t time_delta, bool print_events) {
        if (time_delta < 0) [[unlikely]]
            throw_exception("time_delta needs to be zero or positive.");
        SimulationState state{ std::move(old_state), graph };

        const double end_time = (time_delta == 0) ?
            std::numeric_limits<time_t>::infinity() :
            state.queue.time() + time_delta;

        while (!state.queue.empty() && state.queue.next_event_time() < end_time) {
            process_event_group(state, state.queue.get_event_group(), graph, print_events);
        }

        if (time_delta > 0) {
            state.queue.set_time(end_time);
        }
        return state;
    }


    int benchmark_simulation(const int n_elements) {

        CircuitGraph graph;

        auto elem0 = graph.create_element(ElementType::or_element, 2, 1);
        graph.connect_output(elem0, 0, elem0, 0);

        SimulationState state;
        state.queue.submit_event({ 0.1, elem0, 1, true });
        state.queue.submit_event({ 0.5, elem0, 1, false });

        auto new_state = advance_simulation(state, graph, 0, false);

        return new_state.input_values.front() + n_elements;
    }
}

