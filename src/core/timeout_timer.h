#ifndef LOGICSIM_TIMEOUT_TIMER_H
#define LOGICSIM_TIMEOUT_TIMER_H

#include <chrono>

namespace logicsim {

class TimeoutTimer {
   public:
    using clock_t = std::chrono::steady_clock;
    using time_point = clock_t::time_point;
    using timeout_t = clock_t::duration;

    struct defaults {
        constexpr static auto no_timeout = timeout_t::max();
    };

    explicit TimeoutTimer(timeout_t timeout);
    [[nodiscard]] auto reached_timeout() const -> bool;

   private:
    timeout_t timeout_;
    time_point start_time_;
};

}  // namespace logicsim

#endif
