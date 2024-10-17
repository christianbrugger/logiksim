#ifndef LOGICSIM_FORMAT_STD_TYPE_H
#define LOGICSIM_FORMAT_STD_TYPE_H

#include "algorithm/path_conversion.h"

#include <fmt/core.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>

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
        return fmt::format_to(ctx.out(), "std::nullopt");
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
// std::shared_ptr
//

template <typename T, typename Char>
struct fmt::formatter<std::shared_ptr<T>, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const std::shared_ptr<T> &obj, fmt::format_context &ctx) {
        if (obj) {
            return fmt::format_to(ctx.out(), "{}", *obj);
        }
        return fmt::format_to(ctx.out(), "nullptr");
    }
};

//
// std::weak_ptr
//

template <typename T, typename Char>
struct fmt::formatter<std::weak_ptr<T>, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const std::weak_ptr<T> &obj, fmt::format_context &ctx) {
        if (auto ptr = obj.lock()) {
            return fmt::format_to(ctx.out(), "{}", *ptr);
        }
        return fmt::format_to(ctx.out(), "nullptr");
    }
};

//
// std::filesystem::path
//

template <typename Char>
struct fmt::formatter<std::filesystem::path, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const std::filesystem::path &obj, fmt::format_context &ctx) {
        // On windows paths are stored as wchar and need to be converted to utf-8.
        const auto string = logicsim::path_to_utf8_or_escape(obj);
        return fmt::format_to(ctx.out(), "{}", string);
    }
};

#endif
