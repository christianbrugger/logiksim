#ifndef LOGIKSIM_TIMER_H
#define LOGIKSIM_TIMER_H

#include <fmt/core.h>

#include <chrono>
#include <deque>
#include <ratio>
#include <string>

namespace logicsim {

class Timer {
   public:
    using timer_t = std::chrono::steady_clock;
    using timepoint_t = timer_t::time_point;
    using delta_t = std::chrono::duration<double>;

   public:
    enum class Unit { s, us, ms };

    [[nodiscard]] explicit Timer(std::string description = "", Unit unit = Unit::ms,
                                 int precision = 3);
    ~Timer();

    [[nodiscard]] auto delta() const -> delta_t;
    [[nodiscard]] auto format() const -> std::string;

   private:
    int precision_;
    std::string description_;
    Unit unit_;
    timepoint_t start_;
};

class EventCounter {
   public:
    using timer_t = std::chrono::steady_clock;
    using timepoint_t = timer_t::time_point;

    auto count_event() -> void;
    auto reset() -> void;

    auto events_per_second() const -> double;

   private:
    mutable std::deque<timepoint_t> deque_ {};
};

}  // namespace logicsim

// keep this one, as we don't want to introduce a dependency here
template <>
struct fmt::formatter<logicsim::Timer> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    auto format(const logicsim::Timer &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

#endif