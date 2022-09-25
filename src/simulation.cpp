
#include "simulation.h"

#include <format>

namespace logicsim {

    std::string SimulationEvent::format() const
    {
        auto element_id_str = (element == null_element) ? "NULL" : std::format("{}", element);
        return std::format("<SimulationEvent: at {}s set Element_{}[{}] = {}>",
            time, element_id_str, output, (value ? "true" : "false"));
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








    void process_event(SimulationState& state, [[maybe_unused]] SimulationQueue::event_group_t&& group, const CircuitGraph& graph) {
        if (group.size() == 0)
            return;
        if (state.queue.empty() == 0)
            return;
        if (graph.total_inputs() == 0)
            return;

        const auto element = group.front().element;

        boost::container::small_vector<bool, 16> input_values(graph.get_input_count(element));
    }



    SimulationState advance_simulation(time_t time_delta, SimulationState&& old_state, const CircuitGraph& graph) {
        if (time_delta < 0)
            throw std::exception("time_delta needs to be zero or positive.");
        SimulationState state(std::move(old_state), graph);

        const double end_time = (time_delta == 0) ?
            std::numeric_limits<time_t>::infinity() :
            state.queue.time() + time_delta;

        while (!state.queue.empty() && state.queue.next_event_time() < end_time) {
            process_event(state, state.queue.get_event_group(), graph);
        }

        if (time_delta > 0) {
            state.queue.set_time(end_time);
        }
        return state;
    }

}

