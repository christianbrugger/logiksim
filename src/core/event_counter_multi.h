#ifndef LOGICSIM_EVENT_COUNTER_MULTI_H
#define LOGICSIM_EVENT_COUNTER_MULTI_H

#include <chrono>
#include <cstdint>
#include <deque>

namespace logicsim {

class MultiEventCounter {
   public:
    using timer_t = std::chrono::steady_clock;
    using timepoint_t = timer_t::time_point;
    using duration_t = timer_t::duration;

    MultiEventCounter() = default;
    explicit MultiEventCounter(duration_t average_interval);

    auto count_events(int64_t count) -> void;
    auto reset() -> void;

    auto events_per_second() const -> double;

   private:
    mutable std::deque<timepoint_t> times_ {};
    mutable std::deque<int64_t> counts_ {};
    duration_t average_interval_ {std::chrono::seconds {2}};
};

}  // namespace logicsim

#endif
