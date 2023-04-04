
#include "timer.h"

namespace logicsim {

Timer::Timer(std::string description, Unit unit, int precision)
    : precision_ {precision},
      description_(std::move(description)),
      unit_(unit),
      start_(timer_t::now()) {};

Timer::~Timer() {
    if (!description_.empty()) {
        const auto str = format();
        fmt::print("{}\n", str);
    }
}

auto Timer::delta() const -> std::chrono::duration<double> {
    return std::chrono::duration<double> {timer_t::now() - start_};
}

auto Timer::format() const -> std::string {
    double seconds = delta().count();

    std::string unit_str = "s";
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

auto EventCounter::count_event() -> void {
    deque_.push_back(timer_t::now());
}

auto EventCounter::reset() -> void {
    deque_.clear();
}

auto EventCounter::events_per_second() const -> double {
    using namespace std::literals::chrono_literals;
    const auto now = timer_t::now();

    while (deque_.size() > 20 && now - deque_.front() > 2s) {
        deque_.pop_front();
    }

    if (deque_.size() < 2) {
        return -1.0;
    }

    auto time_delta = std::chrono::duration<double>(deque_.back() - deque_.front());
    return (deque_.size() - 1) / time_delta.count();
}

}  // namespace logicsim
