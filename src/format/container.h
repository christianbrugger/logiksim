#ifndef LOGICSIM_FORMAT_CONTAINER_H
#define LOGICSIM_FORMAT_CONTAINER_H

#include "concept/member_format_function.h"
#include "concept/string_view.h"
#include "iterator_adaptor/transform_view.h"

#include <boost/algorithm/string/join.hpp>

#include <concepts>
#include <functional>
#include <stdexcept>
#include <string_view>

namespace logicsim {

template <typename T, typename Char = char>
concept format_range_type = (!string_view<T, Char>) && (!has_member_format_function<T>) &&
                            requires(T container) {
                                std::begin(container);
                                std::end(container);
                            };

template <typename T, class Proj = std::identity>
    requires format_range_type<T>
[[nodiscard]] constexpr auto fmt_join(std::string_view sep, const T &obj,
                                      std::string_view fmt = "{}", Proj proj = {}) {
    auto format_func = [&fmt, proj](const auto &item) {
        return fmt::format(fmt::runtime(fmt), std::invoke(proj, item));
    };
    return boost::join(transform_view(obj, format_func), sep);
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

        throw std::domain_error("no other range formatting supported");
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
