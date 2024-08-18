#ifndef LOGICSIM_FORMAT_TIME_H
#define LOGICSIM_FORMAT_TIME_H

#include <fmt/core.h>

#include <chrono>

namespace logicsim {

template <class Rep, class Period>
auto format_microsecond_time(std::chrono::duration<Rep, Period> time_value) {
    using namespace std::chrono_literals;

    if (-1us < time_value && time_value < 1us) {
        return fmt::format("{}ns", time_value.count());
    }
    auto time_us = std::chrono::duration<double, std::micro> {time_value};
    return fmt::format("{:L}us", time_us.count());
}

template <class Rep, class Period>
auto format_time(std::chrono::duration<Rep, Period> time_value) {
    using namespace std::chrono_literals;

    if (-1us < time_value && time_value < 1us) {
        auto time_ns = std::chrono::duration<double, std::nano> {time_value};
        return fmt::format("{:.3g}ns", time_ns.count());
    }

    if (-1ms < time_value && time_value < 1ms) {
        auto time_us = std::chrono::duration<double, std::micro> {time_value};
        return fmt::format("{:.3g}us", time_us.count());
    }

    if (-1s < time_value && time_value < 1s) {
        auto time_ms = std::chrono::duration<double, std::milli> {time_value};
        return fmt::format("{:.3g}ms", time_ms.count());
    }

    auto time_s = std::chrono::duration<double, std::ratio<1>> {time_value};
    return fmt::format("{:.3g}s", time_s.count());
}

}  // namespace logicsim

#endif
