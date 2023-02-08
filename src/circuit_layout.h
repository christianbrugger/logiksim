#ifndef LOGIKSIM_CIRCUIT_LAYOUT_H
#define LOGIKSIM_CIRCUIT_LAYOUT_H

#include <fmt/core.h>

#include <cstdint>
#include <string>

namespace logicsim {

enum class DisplayState : uint8_t {
    normal,
    selected,

    new_unknown,
    new_valid,
    new_colliding,
};

auto format(DisplayState state) -> std::string;

class CircuitLayout {
   public:
   private:
};

}  // namespace logicsim

template <>
struct fmt::formatter<logicsim::DisplayState> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::DisplayState &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", ::logicsim::format(obj));
    }
};

#endif