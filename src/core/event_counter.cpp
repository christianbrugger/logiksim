#include "event_counter.h"

#include <gsl/gsl>

namespace logicsim {

EventCounter::EventCounter(duration_t average_interval)
    : average_interval_ {average_interval} {}

auto EventCounter::count_event() -> void {
    deque_.push_back(timer_t::now());
}

auto EventCounter::reset() -> void {
    deque_.clear();
}

auto EventCounter::events_per_second() const -> double {
    using namespace std::literals::chrono_literals;
    const auto now = timer_t::now();

    while (!deque_.empty() && now - deque_.front() > average_interval_) {
        deque_.pop_front();
    }

    if (deque_.empty()) {
        return 0;
    }

    const auto time_delta = std::chrono::duration<double>(now - deque_.front()).count();

    if (time_delta == 0) {
        return 0;
    }
    return gsl::narrow<double>(deque_.size()) / time_delta;
}

}  // namespace logicsim
