#ifndef LOGIKSIM_EDITABLE_CIRCUIT_TYPES_H
#define LOGIKSIM_EDITABLE_CIRCUIT_TYPES_H

#include <fmt/core.h>

namespace logicsim {

enum class LineSegmentType {
    horizontal_first,
    vertical_first,
};

[[nodiscard]] auto format(LineSegmentType type) -> std::string;

}  // namespace logicsim

template <>
struct fmt::formatter<logicsim::LineSegmentType> {
    static constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::LineSegmentType& obj, fmt::format_context& ctx) {
        return fmt::format_to(ctx.out(), "{}", ::logicsim::format(obj));
    }
};
#endif
