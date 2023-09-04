/// This adds basic formatting for range types similar to fmt/range.h
/// We don't use fmt as it is very slow for this.

#ifndef LOGIKSIM_FORMAT_H
#define LOGIKSIM_FORMAT_H

#include "algorithm.h"
#include "exception.h"
#include "iterator_adaptor.h"

#include <boost/algorithm/string/join.hpp>
#include <fmt/core.h>

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace logicsim {

template <typename T, typename Char = char>
concept format_string_type = std::same_as<T, fmt::basic_string_view<Char>> ||
                             std::same_as<T, std::basic_string<Char>> ||
                             std::same_as<T, std::basic_string_view<Char>>;

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
// Escape Non-ASCI
//

auto escape_non_ascii(const std::string &input) -> std::string {
    auto result = std::string {};
    for (const auto &c : input) {
        const auto index = static_cast<unsigned char>(c);
        if (index > 127) {
            fmt::format_to(std::back_inserter(result), "\\x{:02x}", index);
        } else {
            result.push_back(c);
        }
    }
    return result;
}

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
// Enum
//

namespace logicsim {
template <typename T>
auto format(T) -> std::string;
}  // namespace logicsim

// clang-format off
template <typename T>
concept HasFormatFunction = requires(T x) {
    { logicsim::format(x) } -> std::convertible_to<std::string>;
};

// clang-format on

template <typename T, typename Char>
    requires std::is_enum_v<T> && HasFormatFunction<T>
struct fmt::formatter<T, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const T &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", logicsim::format(obj));
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
// std::reference_wrapper
//

template <typename T, typename Char>
struct fmt::formatter<std::reference_wrapper<T>, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const std::reference_wrapper<T> &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "std::ref({})", static_cast<T &>(obj));
    }
};

//
// std::tuple
//

template <typename T1, typename T2, typename Char>
struct fmt::formatter<std::tuple<T1, T2>, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const std::tuple<T1, T2> &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "std::tuple({}, {})", std::get<0>(obj),
                              std::get<1>(obj));
    }
};

//
// std::unique_ptr
//

template <typename T, typename Char>
struct fmt::formatter<std::unique_ptr<T>, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const std::unique_ptr<T> &obj, fmt::format_context &ctx) {
        if (obj) {
            return fmt::format_to(ctx.out(), "{}", *obj);
        }
        return fmt::format_to(ctx.out(), "nullptr");
    }
};

//
// struct with format()
//

namespace logicsim {
template <typename T, typename Char = char>
concept format_obj_with_member_format_function = (!format_string_type<T, Char>) &&
                                                 requires(T obj) {
                                                     {
                                                         obj.format()
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

//
// Pointers
//

namespace logicsim {
template <typename T>
    requires std::is_pointer_v<T>
auto fmt_ptr(T pointer) -> std::string {
    if (pointer == nullptr) {
        return "nullptr";
    }
    return fmt::format("{}", *pointer);
}
}  // namespace logicsim

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

        ::logicsim::throw_exception("no other range formatting supported");
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

namespace logicsim {

// string -> hex

[[nodiscard]] inline auto to_hex(std::string text) -> std::string {
    const auto r = std::ranges::subrange(text.begin(), text.end());
    return fmt_join("", r, "{:x}", [](char c) { return static_cast<uint8_t>(c); });
}

}  // namespace logicsim

#endif