
#include "timer.h"

#include "exception.h"
#include "format.h"

#include <numeric>

namespace logicsim {

Timer::Timer(std::string description, Unit unit, int precision)
    : precision_ {precision},
      description_(std::move(description)),
      unit_(unit),
      start_(timer_t::now()) {};

Timer::~Timer() {
    if (!description_.empty()) {
        const auto str = format();
        print_fmt("{}\n", str);
    }
}

auto Timer::delta() const -> std::chrono::duration<double> {
    return delta_t {timer_t::now() - start_};
}

auto Timer::delta_seconds() const -> double {
    return delta().count();
}

auto Timer::delta_ms() const -> double {
    return delta_seconds() * 1000;
}

auto Timer::format() const -> std::string {
    double seconds = delta().count();

    auto unit_str = std::string {"s"};
    switch (unit_) {
        case Unit::s:
            break;
        case Unit::ms:
            seconds *= 1e3;
            unit_str = "ms";
            break;
        case Unit::us:
            seconds *= 1e6;
            unit_str = "us";
            break;
    }

    auto prefix = description_.empty() ? "" : fmt::format("{}: ", description_);
    return fmt::format("{}{:.{}f}{}", prefix, seconds, precision_, unit_str);
}

//
// EventCounter
//

EventCounter::EventCounter() : EventCounter {defaults::event_counter_average_interval} {}

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

    auto time_delta = std::chrono::duration<double>(now - deque_.front()).count();

    if (time_delta == 0) {
        return 0;
    }
    return deque_.size() / time_delta;
}

// Multi Event Counter

MultiEventCounter::MultiEventCounter()
    : MultiEventCounter {defaults::event_counter_average_interval} {}

MultiEventCounter::MultiEventCounter(duration_t average_interval)
    : average_interval_ {average_interval} {}

auto MultiEventCounter::count_events(int64_t count) -> void {
    if (count < 0) [[unlikely]] {
        throw_exception("count cannot be negative");
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
        throw_exception("times and counts need to have same size");
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

    if (time_delta == 0) {
        return 0;
    }
    return total_count / time_delta;
}

}  // namespace logicsim
