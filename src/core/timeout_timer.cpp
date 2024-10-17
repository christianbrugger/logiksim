#include "core/timeout_timer.h"

namespace logicsim {

TimeoutTimer::TimeoutTimer(timeout_t timeout)
    : timeout_(timeout), start_time_(clock_t::now()) {};

auto TimeoutTimer::reached_timeout() const -> bool {
    return (timeout_ != defaults::no_timeout) &&
           (clock_t::now() - start_time_) > timeout_;
}

}  // namespace logicsim
