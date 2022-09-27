#ifndef LOGIKSIM_SIMULATION_H
#define LOGIKSIM_SIMULATION_H

#include "algorithms.h"
#include "circuit.h"

#include <boost/container/small_vector.hpp>
#include <boost/container/vector.hpp>

#include <queue>
#include <string>
#include <format>
#include <ranges>
#include <iostream>
#include <format>
#include <cmath>


namespace logicsim {

    // using event_id_t = int32_t;
    using time_t = double;


    struct SimulationEvent
    {
        // event_id_t event_id;
        time_t time;
        element_size_t element;
        connection_size_t input;
        bool value;

        bool operator==(const SimulationEvent& other) const;
        bool operator<(const SimulationEvent& other) const;

        bool operator!=(const SimulationEvent& other) const;
        bool operator>(const SimulationEvent& other) const;
        bool operator<=(const SimulationEvent& other) const;
        bool operator>=(const SimulationEvent& other) const;

        std::string format() const;
    };
}

template <>
struct std::formatter<logicsim::SimulationEvent> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const logicsim::SimulationEvent& obj, std::format_context& ctx) {
        return std::format_to(ctx.out(), "{}", obj.format());
    }
};


namespace logicsim {

    using event_group_t = boost::container::small_vector<SimulationEvent, 1>;

    class SimulationQueue {
    public:
        
        time_t time() const noexcept {
            return time_;
        }

        void set_time(time_t time) {
            if (!std::isfinite(time))
                throw_exception("New time needs to be finite.");
            if (time < time_)
                throw_exception("Cannot set new time to the past.");
            if (time > next_event_time())
                throw_exception("New time would be greater than next event.");

            time_ = time;
        }

        time_t next_event_time() const noexcept {
            return events_.empty() ? std::numeric_limits<time_t>::infinity() : events_.top().time;
        }

        bool empty() const noexcept {
            return events_.empty();
        }

        void submit_event(SimulationEvent &&event) {
            if (event.time <= time_)
                throw_exception("Event time needs to be in the future.");
            if (!std::isfinite(event.time))
                throw_exception("Event time needs to be finite.");

            events_.push(std::move(event));
        }

        /* Return next events for the same time and element. */
        event_group_t get_event_group()
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
    private:
        time_t time_{ 0 };
        std::priority_queue<SimulationEvent, std::vector<SimulationEvent>, std::greater<>> events_;
    };

    using logic_vector_t = boost::container::vector<bool>;


    struct SimulationState {
        logic_vector_t input_values;
        SimulationQueue queue;

        SimulationState()
        {
        }
        
        SimulationState(SimulationState&& state, const CircuitGraph graph) :
            input_values{ std::move(state.input_values) },
            queue { std::move(state.queue) }
        {
            input_values.resize(graph.total_inputs());
        }
    };


    SimulationState advance_simulation(SimulationState old_state, const CircuitGraph& graph, time_t time_delta = 0, bool print_events = false);

    logic_vector_t collect_output_values(const logic_vector_t& input_values, const CircuitGraph& graph);

    int benchmark_simulation(const int n_elements = 100);
}

#endif
