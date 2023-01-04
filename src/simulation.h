#ifndef LOGIKSIM_SIMULATION_H
#define LOGIKSIM_SIMULATION_H

#include "circuit.h"

#include <boost/container/small_vector.hpp>
#include <boost/container/vector.hpp>
#include <fmt/core.h>
#include <folly/small_vector.h>
#include <gsl/gsl>

#include <chrono>
#include <ostream>
#include <queue>
#include <random>
#include <string>

/// Done Features
// * delays for each output, needed for wires
// * add timeout to run simulations
// * use discrete integer type for time, like chronos::sim_time<int64_t>
// * store transition times for wires, so they can be drawn

/// New Features
// * flip flops, which requires access to the last state
// * negation on input and outputs
// * clock generators
// * shift registers, requires memory & internal state

namespace logicsim {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4455)  // literal suffix identifiers are reserved
#endif
using std::literals::chrono_literals::operator""us;
using std::literals::chrono_literals::operator""ns;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

// TODO strong type for time_t
using time_t = std::chrono::duration<int64_t, std::nano>;

struct SimulationEvent {
    time_t time;
    element_id_t element_id;
    connection_size_t input_index;
    bool value;

    auto operator==(const SimulationEvent &other) const -> bool;
    auto operator<(const SimulationEvent &other) const -> bool;

    auto operator!=(const SimulationEvent &other) const -> bool;
    auto operator>(const SimulationEvent &other) const -> bool;
    auto operator<=(const SimulationEvent &other) const -> bool;
    auto operator>=(const SimulationEvent &other) const -> bool;

    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::is_trivial<SimulationEvent>::value);
static_assert(std::is_trivially_copyable<SimulationEvent>::value);
static_assert(std::is_standard_layout<SimulationEvent>::value);

auto make_event(Circuit::ConstInput input, time_t time, bool value) -> SimulationEvent;
}  // namespace logicsim

template <>
struct fmt::formatter<logicsim::SimulationEvent> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::SimulationEvent &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

namespace std {

template <class CharT>
auto operator<<(std::basic_ostream<CharT> &os, const logicsim::SimulationEvent &dt)
    -> std::basic_ostream<CharT> & {
    os << dt.format();
    return os;
}
}  // namespace std

namespace logicsim {

/// groups of events for the same element and time  but different inputs
// using event_group_t = boost::container::small_vector<SimulationEvent, 2>;
using event_group_t = folly::small_vector<SimulationEvent, 4>;
void validate(const event_group_t &events);

class SimulationQueue {
   public:
    [[nodiscard]] auto time() const noexcept -> time_t;
    [[nodiscard]] auto next_event_time() const noexcept -> time_t;
    [[nodiscard]] auto empty() const noexcept -> bool;

    void set_time(time_t time);
    void submit_event(SimulationEvent event);
    /// Remove and return all events for the next time and element_id.
    auto pop_event_group() -> event_group_t;

   private:
    time_t time_ {0us};
    std::priority_queue<SimulationEvent, std::vector<SimulationEvent>, std::greater<>>
        events_;
};

using timeout_clock = std::chrono::steady_clock;
using timeout_t = timeout_clock::duration;

class Simulation {
   private:
    struct ElementState;

   public:
    bool print_events {false};

    /// Represents multiple logic values
    using logic_vector_t = boost::container::vector<bool>;
    using delay_vector_t = boost::container::vector<time_t>;

    // 8 bytes still fit into a small_vector with 32 byte size.
    // using logic_small_vector_t = boost::container::small_vector<bool, 8>;
    // using con_index_small_vector_t = boost::container::small_vector<connection_size_t,
    // 8>;

    using logic_small_vector_t = folly::small_vector<bool, 20, uint32_t>;
    using con_index_small_vector_t = folly::small_vector<connection_size_t, 20, uint32_t>;
    static_assert(sizeof(logic_small_vector_t) == 24);
    static_assert(sizeof(con_index_small_vector_t) == 24);

    struct defaults {
        constexpr static time_t standard_delay = 100us;

        constexpr static auto no_timeout = timeout_t::max();
        constexpr static auto infinite_simulation_time = time_t::max();
        constexpr static int64_t no_max_events {
            std::numeric_limits<int64_t>::max()
            - std::numeric_limits<connection_size_t>::max()};
    };

   public:
    [[nodiscard]] explicit Simulation(const Circuit &circuit);
    [[nodiscard]] auto circuit() const noexcept -> const Circuit &;
    [[nodiscard]] auto time() const noexcept -> time_t;
    auto submit_event(Circuit::ConstInput input, time_t delay, bool value) -> void;
    auto submit_events(Circuit::ConstElement element, time_t delay,
                       logic_small_vector_t values) -> void;

    auto initialize() -> void;

    /// @brief Run the simulation by changing the given simulations state
    /// @param state             either new or the old simulation state to start from
    /// @param circuit           the circuit that should be simulated
    /// @param simulation_time   simulate for this time or, when run_until_steady, run
    /// until
    ///                          no more new events are generated
    /// @param timeout           return if simulation takes longer than this in realtime
    /// @param print_events      if true print each processed event information
    auto run(time_t simulation_time = defaults::infinite_simulation_time,
             timeout_t timeout = defaults::no_timeout,
             int64_t max_events = defaults::no_max_events) -> int64_t;

    [[nodiscard]] auto input_value(Circuit::ConstInput input) const -> bool;
    [[nodiscard]] auto input_values(Circuit::ConstElement element) const
        -> logic_small_vector_t;
    [[nodiscard]] auto input_values() const -> const logic_vector_t;

    /// infers the output value from the connected input value, if it exists.
    [[nodiscard]] auto output_value(Circuit::ConstOutput output,
                                    bool raise_missing = true) const -> bool;
    [[nodiscard]] auto output_values(Circuit::ConstElement element,
                                     bool raise_missing = true) const
        -> logic_small_vector_t;
    [[nodiscard]] auto output_values(bool raise_missing = true) const -> logic_vector_t;

    [[nodiscard]] auto output_delay(Circuit::ConstOutput output) const -> time_t;
    auto set_output_delay(Circuit::ConstOutput output, time_t delay) -> void;

    [[nodiscard]] auto internal_state(Circuit::ConstElement element) const
        -> logic_small_vector_t;

   private:
    class Timer;

    auto check_state_valid() const -> void;

    auto submit_events_for_changed_outputs(const Circuit::ConstElement element,
                                           const logic_small_vector_t &old_outputs,
                                           const logic_small_vector_t &new_outputs)
        -> void;
    auto process_event_group(event_group_t &&events) -> void;
    auto create_event(const Circuit::ConstOutput output,
                      const logic_small_vector_t &output_values) -> void;
    auto apply_events(const Circuit::ConstElement element, const event_group_t &group)
        -> void;
    auto set_input(const Circuit::ConstInput input, bool value) -> void;
    auto set_internal_state(const Circuit::ConstElement element,
                            const logic_small_vector_t &state) -> void;

    [[nodiscard]] auto get_state(ElementOrConnection auto item) -> ElementState &;
    [[nodiscard]] auto get_state(ElementOrConnection auto item) const
        -> const ElementState &;

    struct ElementState {
        logic_small_vector_t input_values {};
        folly::small_vector<time_t, 4, uint32_t> output_delays {};
        logic_small_vector_t internal_state {};

        static_assert(sizeof(output_delays) == 36);
    };

    gsl::not_null<const Circuit *> circuit_;
    std::vector<ElementState> states_ {};
    SimulationQueue queue_ {};
};

inline constexpr int BENCHMARK_DEFAULT_EVENTS {10'000};

template <std::uniform_random_bit_generator G>
auto benchmark_simulation(G &rng, const Circuit &circuit, const int n_events,
                          const bool print) -> int64_t;
auto benchmark_simulation(int n_elements = BENCHMARK_DEFAULT_ELEMENTS,
                          int n_events = BENCHMARK_DEFAULT_EVENTS, bool print = false)
    -> int64_t;

}  // namespace logicsim

#endif
