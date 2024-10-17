#include "core/event_counter_multi.h"

#include <gsl/gsl>

#include <numeric>

namespace logicsim {

MultiEventCounter::MultiEventCounter(duration_t average_interval)
    : average_interval_ {average_interval} {}

auto MultiEventCounter::count_events(int64_t count) -> void {
    if (count < 0) [[unlikely]] {
        throw std::runtime_error("count cannot be negative");
    }
    if (count == 0) {
        return;
    }

    times_.push_back(timer_t::now());
    counts_.push_back(count);
}

auto MultiEventCounter::reset() -> void {
    times_.clear();
    counts_.clear();
}

auto MultiEventCounter::events_per_second() const -> double {
    const auto now = timer_t::now();

    if (times_.size() != counts_.size()) [[unlikely]] {
        throw std::runtime_error("times and counts need to have same size");
    }

    while (!times_.empty() && now - times_.front() > average_interval_) {
        times_.pop_front();
        counts_.pop_front();
    }

    if (times_.empty()) {
        return 0;
    }

    const auto time_delta = std::chrono::duration<double>(now - times_.front()).count();
    const auto total_count = std::accumulate(counts_.begin(), counts_.end(), int64_t {0});

    if (time_delta == double {0}) {
        return double {0};
    }
    return gsl::narrow<double>(total_count) / time_delta;
}

}  // namespace logicsim
