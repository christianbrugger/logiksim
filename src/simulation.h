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


namespace logicsim {

    // using event_id_t = int32_t;
    using time_t = double;


    struct SimulationEvent
    {
        // event_id_t event_id;
        time_t time;
        element_size_t element;
        connection_size_t output;
        bool value;

        bool operator==(const SimulationEvent& other) const;
        bool operator<(const SimulationEvent& other) const;

        bool operator!=(const SimulationEvent& other) const;
        bool operator>(const SimulationEvent& other) const;
        bool operator<=(const SimulationEvent& other) const;
        bool operator>=(const SimulationEvent& other) const;

        std::string format() const;
    };


    class SimulationQueue {
    public:
        
        time_t time() const noexcept {
            return time_;
        }

        void set_time(time_t time) {
            if (time < time_) {
                throw std::exception(std::format(
                    "Cannot set time to {}s, new time would be in the past. Current time is {}s.",
                    time, time_).c_str());
            }
            if (time > next_event_time()) {
                throw std::exception(std::format(
                    "Cannot set time to {}s, new time would be too far into the future. Next event is at {}s.",
                    time, next_event_time()).c_str());
            }
            time_ = time;
        }

        time_t next_event_time() const noexcept {
            return events_.empty() ? std::numeric_limits<time_t>::infinity() : events_.top().time;
        }

        bool empty() const noexcept {
            return events_.empty();
        }

        void add_event(SimulationEvent &&event) {
            if (event.time <= time_) {
                std::string msg = std::format("Cannot submit int the past or present event at {}s {}", time_, event.format());
                throw std::exception(msg.c_str());
            }
            events_.push(std::move(event));
        }

        using event_group_t = boost::container::small_vector<SimulationEvent, 1>;

        /* Return next events with the same time and element. */
        event_group_t get_event_group()
        {
            event_group_t group;
            pop_while(
                events_,
                [&group](const SimulationEvent& event) { group.push_back(event); },
                [&group](const SimulationEvent& event) { return group.size() == 0 || group.front() == event; }
            );
            if (group.size() > 0) {
                time_ = group.front().time;
            }
            return group;
        }
    private:
        time_t time_;
        std::priority_queue<SimulationEvent, std::vector<SimulationEvent>, std::greater<>> events_;
    };


    using logic_vector_t = boost::container::vector<bool>;


    struct SimulationState {
        logic_vector_t input_values;
        logic_vector_t output_values;
        SimulationQueue queue;

        SimulationState()
        {
        }

        SimulationState(SimulationState&& state, const CircuitGraph graph) :
            input_values(std::move(state.input_values)),
            output_values(std::move(state.output_values))
        {
            input_values.resize(graph.total_inputs());
            output_values.resize(graph.total_outputs());
        }
    };


    void process_event(SimulationState& state, [[maybe_unused]] SimulationQueue::event_group_t&& group, const CircuitGraph& graph);
    void test_sv();
    SimulationState advance_simulation(time_t time_delta, SimulationState&& old_state, const CircuitGraph& graph);
}

#endif