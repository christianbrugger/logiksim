#ifndef LOGICSIM_FORMAT_STRUCT_H
#define LOGICSIM_FORMAT_STRUCT_H

#include "concept/member_format_function.h"

#include <fmt/core.h>

/**
 * Add fmt formatting to structs and classes that provide `format()` member function.
 *
 *     struct/class Foo {
 *       public:
 *         auto format() const -> std::string;
 *     };
 *
 */
template <typename T, typename Char>
    requires logicsim::has_member_format_function<T, Char>
struct fmt::formatter<T, Char> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    inline auto format(const T &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

#endif
