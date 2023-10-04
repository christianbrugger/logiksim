#include "timer.h"

#include "logging.h"

#include <fmt/core.h>

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

}  // namespace logicsim
