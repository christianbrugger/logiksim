#ifndef LOGIKSIM_TIMER_H
#define LOGIKSIM_TIMER_H

#include "format/enum.h"
#include "format/struct.h"

#include <chrono>
#include <functional>
#include <optional>
#include <ratio>
#include <string>

namespace logicsim {

namespace timer {

enum class Unit { s, ms, us, ns };

using logging_function = std::function<void(std::string)>;

}  // namespace timer

template <>
[[nodiscard]] auto format(timer::Unit unit) -> std::string;

class Timer {
   public:
    using Unit = timer::Unit;
    using logging_function = timer::logging_function;

    using clock_t = std::chrono::steady_clock;
    using timepoint_t = clock_t::time_point;
    using delta_t = std::chrono::duration<double>;

   public:
    [[nodiscard]] explicit Timer(std::string description = "", Unit unit = Unit::ms,
                                 int precision = 3,
                                 std::optional<logging_function> custom_logging = {});

    ~Timer();
    Timer(const Timer&) = default;
    Timer(Timer&& other) noexcept;
    auto operator=(const Timer& other) -> Timer&;
    auto operator=(Timer&& other) noexcept -> Timer&;
    friend auto swap(Timer& a, Timer& b) noexcept -> void;

    [[nodiscard]] auto delta() const -> delta_t;
    [[nodiscard]] auto delta_seconds() const -> double;
    [[nodiscard]] auto delta_ms() const -> double;

    [[nodiscard]] auto format() const -> std::string;

   private:
    std::string description_;
    int precision_;
    Unit unit_;
    std::optional<logging_function> custom_logging_;
    // initialize start_ last so time is measure after everything is initialized
    timepoint_t start_;
};

inline auto swap(Timer& a, Timer& b) noexcept -> void {
    using std::swap;

    swap(a.description_, b.description_);
    swap(a.precision_, b.precision_);
    swap(a.unit_, b.unit_);
    swap(a.custom_logging_, b.custom_logging_);
    swap(a.start_, b.start_);
}

}  // namespace logicsim

#endif