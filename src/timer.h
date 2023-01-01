#ifndef LOGIKSIM_TIMER_H
#define LOGIKSIM_TIMER_H

#include <fmt/core.h>

#include <chrono>
#include <ratio>
#include <string>

namespace logicsim {

class Timer {
    using timer_t = std::chrono::steady_clock;
    using timepoint_t = timer_t::time_point;

   public:
    enum class Unit { s, us, ms };

    explicit Timer(std::string description = "", Unit unit = Unit::s, int precision = 2);
    ~Timer();

    [[nodiscard]] auto delta() const -> std::chrono::duration<double>;
    [[nodiscard]] auto format() const -> std::string;

   private:
    int precision_;
    std::string description_;
    Unit unit_;
    timepoint_t start_;
};

}  // namespace logicsim

template <>
struct fmt::formatter<logicsim::Timer> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Timer &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

#endif