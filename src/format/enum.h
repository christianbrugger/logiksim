#ifndef LOGICSIM_FORMAT_ENUM_H
#define LOGICSIM_FORMAT_ENUM_H

/**
 * Add fmt formatting to enums that provide a free `format(EnumType)` function.
 *
 *   template <>
 *   auto format(EnumType type) -> std::string;
 *
 * Defined in the logicsim namespace.
 *
 */

#include <fmt/core.h>

#include <concepts>
#include <string>
#include <type_traits>

namespace logicsim {
template <typename T>
auto format(T) -> std::string;

// clang-format off
template <typename T>
concept HasFormatFunction = requires(T x) {
    { logicsim::format(x) } -> std::convertible_to<std::string>;
};

// clang-format on

}  // namespace logicsim

template <typename T, typename Char>
    requires std::is_enum_v<T> && logicsim::HasFormatFunction<T>
struct fmt::formatter<T, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const T &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", logicsim::format(obj));
    }
};

#endif
