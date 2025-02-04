#pragma once

#include <chrono>
#include <deque>

namespace logicsim {

class GuiEventCounter {
   public:
    using timer_t = std::chrono::steady_clock;
    using timepoint_t = timer_t::time_point;
    using duration_t = timer_t::duration;

    GuiEventCounter() = default;
    explicit GuiEventCounter(duration_t average_interval);

    auto count_event() -> void;
    auto reset() -> void;

    auto events_per_second() const -> double;

   private:
    mutable std::deque<timepoint_t> deque_ {};
    duration_t average_interval_ {std::chrono::seconds {2}};
};

}  // namespace logicsim
