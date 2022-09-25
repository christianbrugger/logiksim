#ifndef LOGIKSIM_SIMULATION_H
#define LOGIKSIM_SIMULATION_H

#include <queue>

#include "algorithms.h"
#include "circuit.h"


namespace logicsim {

    using event_id_t = int32_t;
    using time_t = double;


    struct LogicEvent
    {
        event_id_t event_id;
        time_t time;
        element_size_t element;
        connection_size_t output;
        bool value;

        bool operator==(const LogicEvent& other) const;
        bool operator<(const LogicEvent& other) const;

        bool operator!=(const LogicEvent& other) const;
        bool operator>(const LogicEvent& other) const;
        bool operator<=(const LogicEvent& other) const;
        bool operator>=(const LogicEvent& other) const;

        std::string format() const;
    };

    using event_queue_t = std::priority_queue<LogicEvent, std::vector<LogicEvent>, std::greater<>>;


    class SimulationState {
    public:

        SimulationState(time_t time = 0) :
            time_(time)
        {}

        SimulationState(time_t time, event_queue_t &&events) :
            time_(time),
            events_(std::move(events))
        {}

        event_queue_t get_events() const {
            return events_;
        }

        time_t time() const {
            return time_;
        }

    private:
        time_t time_;
        event_queue_t events_;
    };


    SimulationState advance_simulation(time_t time_delta, const SimulationState &state) {
        if (time_delta < 0)
            throw std::exception("time_delta needs to be zero or positive.");

        const double end_time = (time_delta == 0) ? 
            std::numeric_limits<time_t>::infinity() : 
            state.time() + time_delta;

        event_queue_t events = state.get_events();
        queue_apply_while(
            events,
            []([[maybe_unused]] const LogicEvent& event) {},
            [end_time](const LogicEvent& event) { return end_time >= event.time; });

        return SimulationState(end_time, std::move(events));
    }


    SimulationState advance_simulation(time_t time_delta) {
        SimulationState state;
        return advance_simulation(time_delta, state);
    }
}

#endif