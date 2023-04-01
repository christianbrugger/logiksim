/// This adds basic formatting for range types similar to fmt/range.h
/// We don't use fmt as it is very slow for this.

#ifndef LOGIKSIM_FORMAT_H
#define LOGIKSIM_FORMAT_H

#include "algorithm.h"
#include "exceptions.h"
#include "iterator_adaptor.h"

#include <boost/algorithm/string/join.hpp>
#include <fmt/core.h>

#include <concepts>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace logicsim {

template <typename T, typename Char = char>
concept format_string_type = std::same_as<T, fmt::basic_string_view<Char>>
                             || std::same_as<T, std::basic_string<Char>>
                             || std::same_as<T, std::basic_string_view<Char>>;

//
// print
//

namespace detail {

// repeats {} count times: 2 -> "{} {}\n"
template <std::size_t count>
constexpr auto repeat_format_string() {
    constexpr auto buffer_size = count == 0 ? 1 : count * 3;
    std::array<char, buffer_size> buffer;

    std::size_t pos = 0;
    for (std::size_t i = 0; i < count; ++i) {
        std::string_view format = "{} ";
        for (char c : format) {
            buffer[pos++] = c;
        }
    }
    if constexpr (count > 0) {
        --pos;
    }
    buffer[pos] = '\n';
    return buffer;
}

}  // namespace detail

template <typename... Args>
auto print(Args &&...args) {
    constexpr static std::size_t count = sizeof...(Args);
    constexpr static auto buffer = detail::repeat_format_string<count>();

    constexpr static auto fmt_str = std::string_view {buffer.data(), buffer.size()};
    fmt::print(fmt_str, std::forward<Args>(args)...);
}

}  // namespace logicsim

//
// std::pair
//

template <typename T1, typename T2, typename Char>
struct fmt::formatter<std::pair<T1, T2>, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const std::pair<T1, T2> &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "({}, {})", obj.first, obj.second);
    }
};

//
// std::optional
//

template <typename T, typename Char>
struct fmt::formatter<std::optional<T>, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const std::optional<T> &obj, fmt::format_context &ctx) {
        if (obj.has_value()) {
            return fmt::format_to(ctx.out(), "{}", *obj);
        }
        return fmt::format_to(ctx.out(), "std::nullopt<{}>", typeid(T).name());
    }
};

//
// obj.format() and format(obj)
//

namespace logicsim {
template <typename T, typename Char = char>
concept format_obj_with_member_format_function
    = (!format_string_type<T, Char>) && requires(T container) {
                                            {
                                                container.format()
                                                } -> std::same_as<std::string>;
                                        };
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

// namespace logicsim {
//
//// we need to define a free function format to be able to compile our templates
// struct _format_dummy;
// auto format(_format_dummy) -> std::string;
//
// template <typename T, typename Char = char>
// concept format_obj_with_free_format_function
//     = (!format_string_type<T, Char>) && requires(T container) {
//                                             {
//                                                 ::logicsim::format(container)
//                                                 } -> std::same_as<std::string>;
//                                         };
// }  // namespace logicsim
//
//// TODO remove all format(obj) specializations
// template <typename T, typename Char>
//     requires logicsim::format_obj_with_free_format_function<T, Char>
// struct fmt::formatter<T, Char> {
//     constexpr auto parse(fmt::format_parse_context &ctx) {
//         return ctx.begin();
//     }
//
//     inline auto format(const T &obj, fmt::format_context &ctx) {
//         return fmt::format_to(ctx.out(), "{}", ::logicsim::format(obj));
//     }
// };

//
// range(begin, end)
//

namespace logicsim {
template <typename T, typename Char = char>
concept format_range_type = (!format_string_type<T, Char>) && requires(T container) {
                                                                  std::begin(container);
                                                                  std::end(container);
                                                              };

template <typename T, class Proj = std::identity>
    requires logicsim::format_range_type<T>
[[nodiscard]] constexpr auto fmt_join(std::string_view fmt, const T &obj,
                                      std::string_view sep = ", ", Proj proj = {}) {
    auto format_func = [&fmt, proj](const auto &item) {
        return fmt::format(fmt::runtime(fmt), std::invoke(proj, item));
    };
    return boost::join(transform_view(obj, format_func), sep);
}

template <typename T, class Proj = std::identity>
    requires logicsim::format_range_type<T>
[[nodiscard]] constexpr auto fmt_join(const T &obj, std::string_view sep = ", ",
                                      Proj proj = {}) {
    return fmt_join("{}", obj, sep, proj);
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