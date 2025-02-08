#include "pch.h"

#include "ls_timer.h"

#include <format>
#include <iostream>
#include <utility>

namespace logicsim {

auto format(timer::Unit unit) -> std::string {
    switch (unit) {
        using enum timer::Unit;
        case s:
            return "s";
        case ms:
            return "ms";
        case us:
            return "us";
        case ns:
            return "ns";
    }
    std::terminate();
}

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
            std::cout << format() << '\n';
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
    double seconds = delta_seconds();

    switch (unit_) {
        case Unit::s:
            break;
        case Unit::ms:
            seconds *= 1e3;
            break;
        case Unit::us:
            seconds *= 1e6;
            break;
        case Unit::ns:
            seconds *= 1e9;
            break;
    }

    auto prefix = description_.empty() ? "" : std::format("{}: ", description_);
    return std::format("{}{:.{}f}{}", prefix, seconds, precision_,
                       logicsim::format(unit_));
}

}  // namespace logicsim
