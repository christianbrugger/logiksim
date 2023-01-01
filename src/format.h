/// This adds basic formatting for range types similar to fmt/range.h
/// We don't use fmt as it is very slow for this.

#ifndef LOGIKSIM_FORMAT_H
#define LOGIKSIM_FORMAT_H

#include "algorithm.h"
#include "exceptions.h"

#include <boost/algorithm/string/join.hpp>
#include <fmt/core.h>

#include <concepts>
#include <string_view>

namespace logicsim {

template <typename T, typename Char = char>
concept format_range_type
    = (!std::same_as<T, fmt::basic_string_view<Char>>)
      && (!std::same_as<T, std::basic_string<Char>>)
      && (!std::same_as<T, std::basic_string_view<Char>>) && requires(T container) {
                                                                 std::begin(container);
                                                                 std::end(container);
                                                             };

template <typename T>
    requires logicsim::format_range_type<T>
[[nodiscard]] constexpr auto fmt_join(std::string_view fmt, const T &obj,
                                      std::string_view sep = ", ") {
    auto items = ::logicsim::transform_to_vector(
        std::begin(obj), std::end(obj),
        [&](const auto &item) { return fmt::format(fmt::runtime(fmt), item); });
    return boost::join(items, sep);
}

template <typename T>
    requires logicsim::format_range_type<T>
[[nodiscard]] constexpr auto fmt_join(const T &obj, std::string_view sep = ", ") {
    return fmt_join("{}", obj, sep);
}

}  // namespace logicsim

template <typename T, typename Char>
    requires logicsim::format_range_type<T, Char>
struct fmt::formatter<T, Char> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        auto it = ctx.begin();
        auto end = ctx.end();

        if (it != end && *it == 'n') {
            use_brackets_ = false;
            ++it;
        }

        if (it == end || *it == '}') {
            return it;
        }

        ::logicsim::throw_exception("no other range formatting supported");
    }

    auto format(const T &obj, fmt::format_context &ctx) const {
        auto inner = ::logicsim::fmt_join(obj);

        if (use_brackets_) {
            return fmt::format_to(ctx.out(), fmt::runtime("[{}]"), inner);
        }
        return fmt::format_to(ctx.out(), fmt::runtime("{}"), inner);
    }

   private:
    bool use_brackets_ {true};
};

#endif