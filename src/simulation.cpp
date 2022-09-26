
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




    bool is_valid(const event_group_t& group) {
        if (group.size() == 0)
            return true;

        // std::ranges::all_of(group, [&group](const auto& val) { return val.element == std::begin(group)->element; });
        return false;
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
            input_values[event.input] = event.value; });
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

    void process_event_group(SimulationState& state, const event_group_t& events, const CircuitGraph& graph) {
        if (events.size() == 0)
            return;

        const auto element{ events.front().element };
        const auto element_config{ graph.get_input_config(element) };

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
    

    SimulationState advance_simulation(time_t time_delta, SimulationState&& old_state, const CircuitGraph& graph) {
        if (time_delta < 0)
            throw std::exception("time_delta needs to be zero or positive.");
        SimulationState state(std::move(old_state), graph);

        const double end_time = (time_delta == 0) ?
            std::numeric_limits<time_t>::infinity() :
            state.queue.time() + time_delta;

        while (!state.queue.empty() && state.queue.next_event_time() < end_time) {
            process_event_group(state, state.queue.get_event_group(), graph);
        }

        if (time_delta > 0) {
            state.queue.set_time(end_time);
        }
        return state;
    }

}

