#include "timer.h"

#include "logging.h"

#include <fmt/core.h>

#include <utility>

namespace logicsim {

Timer::Timer(std::string description, Unit unit, int precision,
             std::optional<logging_function> custom_logging)
    : description_ {std::move(description)},
      precision_ {precision},
      unit_ {unit},
      custom_logging_ {std::move(custom_logging)},
      start_ {clock_t::now()} {};

Timer::~Timer() {
    if (!description_.empty()) {
        if (custom_logging_.has_value()) {
            custom_logging_->operator()(format());
        } else {
            print(format());
        }
    }
}

Timer::Timer(Timer &&other) noexcept : Timer {} {
    swap(*this, other);
};

auto Timer::operator=(const Timer &other) -> Timer & {
    auto tmp = Timer {other};
    swap(*this, tmp);
    return *this;
}

auto Timer::operator=(Timer &&other) noexcept -> Timer & {
    auto tmp = Timer {std::move(other)};
    swap(*this, tmp);
    return *this;
}

auto Timer::delta() const -> delta_t {
    return delta_t {clock_t::now() - start_};
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
        case Unit::ns:
            seconds *= 1e9;
            unit_str = "ns";
            break;
    }

    auto prefix = description_.empty() ? "" : fmt::format("{}: ", description_);
    return fmt::format("{}{:.{}f}{}", prefix, seconds, precision_, unit_str);
}

}  // namespace logicsim
