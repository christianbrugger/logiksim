
#include "timer.h"

#include <fmt/chrono.h>

namespace logicsim {

Timer::Timer(std::string description, Unit unit, int precision)
    : precision_ {precision},
      description_(std::move(description)),
      unit_(unit),
      start_(timer_t::now()) {};

Timer::~Timer() {
    if (!description_.empty()) {
        fmt::print("{}\n", *this);
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

}  // namespace logicsim
