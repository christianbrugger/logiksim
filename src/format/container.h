#ifndef LOGICSIM_FORMAT_CONTAINER_H
#define LOGICSIM_FORMAT_CONTAINER_H

#include "algorithm/fmt_join.h"
#include "concept/member_format_function.h"
#include "concept/string_view.h"

#include <fmt/core.h>

#include <concepts>
#include <stdexcept>
#include <string_view>

namespace logicsim {

template <typename T, typename Char = char>
concept format_range_type =
    !string_view<T, Char> && !has_member_format_function<T> && requires(T container) {
        std::begin(container);
        std::end(container);
    };

}  // namespace logicsim

/**
 * @brief: Formatter for any container like type.
 *
 * Note that each element of the container is formatted according to their type.
 *
 * Variation with and without brackets is possible.
 *
 *      {}  => [1, 2, 3]
 *      {n} => 1, 2, 3
 *
 */
template <typename T, typename Char>
    requires logicsim::format_range_type<T, Char>
struct fmt::formatter<T, Char> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        const auto *it = ctx.begin();
        const auto *const end = ctx.end();

        if (it != end && *it == 'n') {
            use_brackets_ = false;
            std::advance(it, 1);
        }

        if (it == end || *it == '}') {
            return it;
        }

        throw std::runtime_error("no other range formatting supported");
    }

    auto format(const T &obj, fmt::format_context &ctx) const {
        auto inner = ::logicsim::fmt_join(", ", obj);

        if (use_brackets_) {
            return fmt::format_to(ctx.out(), fmt::runtime("[{}]"), inner);
        }
        return fmt::format_to(ctx.out(), fmt::runtime("{}"), inner);
    }

   private:
    bool use_brackets_ {true};
};

#endif
