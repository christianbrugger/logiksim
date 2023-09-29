#ifndef LOGICSIM_FORMAT_STRUCT_H
#define LOGICSIM_FORMAT_STRUCT_H

/**
 * Add fmt formatting to structs and classes that provide `format()` member function.
 *
 *     struct/class Foo {
 *       public:
 *         auto format() const -> std::string;
 *     };
 *
 */

#include "concept/string_view.h"

#include <fmt/core.h>

#include <string>

namespace logicsim {

// clang-format off
template <typename T, typename Char = char>
concept format_obj_with_member_format_function = 
    (!string_view<T, Char>) && 
    requires(T obj) { {obj.format()} -> std::same_as<std::string>; };
// clang-format on

}  // namespace logicsim

template <typename T, typename Char>
    requires logicsim::format_obj_with_member_format_function<T, Char>
struct fmt::formatter<T, Char> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    inline auto format(const T &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

#endif
