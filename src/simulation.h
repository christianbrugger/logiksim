#ifndef LOGIKSIM_SIMULATION_H
#define LOGIKSIM_SIMULATION_H

#include "circuit.h"
#include "exceptions.h"
#include "algorithms.h"

#include <boost/container/small_vector.hpp>
#include <boost/container/vector.hpp>

#include <range/v3/core.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/algorithm/count_if.hpp>

#include <fmt/format.h>

#include <queue>
#include <string>
#include <iostream>
#include <cmath>
#include <ostream>


namespace logicsim {

    using time_t = double;


    struct SimulationEvent
    {
        time_t time;
        element_id_t element_id;
        connection_size_t input_index;
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
struct fmt::formatter<logicsim::SimulationEvent> {
    constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const logicsim::SimulationEvent& obj, fmt::format_context& ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

namespace std {

    template<class CharT>
    std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const logicsim::SimulationEvent& dt)
    {
        os << dt.format();
        return os;
    }
}


namespace logicsim {

    /// groups of events for the same element and time  but different inputs
    using event_group_t = boost::container::small_vector<SimulationEvent, 1>;
    void validate(const event_group_t& events);


    class SimulationQueue {
    public:
        
        time_t time() const noexcept;
        time_t next_event_time() const noexcept;
        bool empty() const noexcept;

        void set_time(time_t time);
        void submit_event(SimulationEvent&& event);
        /// Remove and return all events for the next time and element_id.
        event_group_t pop_event_group();
    private:
        time_t time_ { 0 };
        std::priority_queue<SimulationEvent, std::vector<SimulationEvent>, std::greater<>> events_;
    };

    /// Represents multiple logic values
    using logic_vector_t = boost::container::vector<bool>;

    /// Store simulation data.
    struct SimulationState {
        logic_vector_t input_values {};
        SimulationQueue queue {};

        SimulationState(connection_id_t total_inputs);
    };

    /// @brief Advance the simulation by changing the given simulations state
    /// @param state          either new or the old simulation state to start from
    /// @param circuit        the circuit that should be simulated
    /// @param time_delta     tun for this time or, when zero, run until no more new events are generated
    /// @param print_events   if true print each processed event information
    void advance_simulation(
        SimulationState &state, const Circuit& circuit, 
        time_t time_delta = 0, bool print_events = false);


    /// infers the output value from the connected input value, if it exists.
    bool get_output_value(const Circuit::ConstOutput output, const logic_vector_t& input_values, 
        const bool raise_missing = true);
    /// infer vector of all output values from the circuit and input values.
    logic_vector_t output_value_vector(const logic_vector_t& input_values, const Circuit& circuit, 
        const bool raise_missing = true);

    int benchmark_simulation(const int n_elements = 100, bool print = false);

}

#endif
