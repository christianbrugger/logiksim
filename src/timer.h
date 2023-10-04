#ifndef LOGIKSIM_TIMER_H
#define LOGIKSIM_TIMER_H

#include "format/struct.h"

#include <chrono>
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
    [[nodiscard]] auto delta_seconds() const -> double;
    [[nodiscard]] auto delta_ms() const -> double;

    [[nodiscard]] auto format() const -> std::string;

   private:
    int precision_;
    std::string description_;
    Unit unit_;
    timepoint_t start_;
};

}  // namespace logicsim

#endif