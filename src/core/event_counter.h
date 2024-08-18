#ifndef LOGICSIM_EVENT_COUNTER_H
#define LOGICSIM_EVENT_COUNTER_H

#include <chrono>
#include <deque>

namespace logicsim {

class EventCounter {
   public:
    using timer_t = std::chrono::steady_clock;
    using timepoint_t = timer_t::time_point;
    using duration_t = timer_t::duration;

    EventCounter() = default;
    explicit EventCounter(duration_t average_interval);

    auto count_event() -> void;
    auto reset() -> void;

    auto events_per_second() const -> double;

   private:
    mutable std::deque<timepoint_t> deque_ {};
    duration_t average_interval_ {std::chrono::seconds {2}};
};

}  // namespace logicsim

#endif
