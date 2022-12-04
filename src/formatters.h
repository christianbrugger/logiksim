#ifndef LOGIKSIM_FORMATTERS_H
#define LOGIKSIM_FORMATTERS_H

#include <boost/container/vector.hpp>
#include <boost/container/small_vector.hpp>

#include <range/v3/all.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <iostream>
#include <format>
#include <sstream>


/*
/// A specialization of fmt::formatter for boost::container::small_vector
template<typename T, std::size_t N, class CharT>
struct fmt::formatter<boost::container::small_vector<T, N>, CharT> : fmt::formatter<std::string_view> {
    auto tostring(const boost::container::small_vector<T, N>& obj, fmt::format_context& ctx) {
        const std::string inner = 
            ::ranges::views::all(obj)
            | ::ranges::views::transform([](const auto &item){ return fmt::tostring("{}", item); })
            | ::ranges::views::join(',')
            | ::ranges::to<std::string>();
        const std::string text = std::tostring("[{}]", inner);

        return fmt::formatter<std::string_view>::tostring(text, ctx);
    }
};
*/

/*
/// A specialization of fmt::formatter for boost::container::small_vector bool
template<std::size_t N>
struct fmt::formatter<boost::container::small_vector<bool, N>, char> : fmt::formatter<std::string_view> {
    auto tostring(const boost::container::small_vector<bool, N>& obj, fmt::format_context& ctx) const {
        std::ostringstream inner;
        inner << ::ranges::views::all(obj);
        return fmt::formatter<std::string_view>::tostring(inner.str(), ctx);
    }
};
*/


/*
/// A specialization of fmt::formatter for boost::container::vector
template<typename T, class CharT>
struct fmt::formatter<boost::container::vector<T>, CharT> : fmt::formatter<std::string_view> {
    auto tostring(const boost::container::vector<T>& obj, fmt::format_context& ctx) {
        std::ostringstream inner;
        inner << ::ranges::views::all(obj);
        return fmt::formatter<std::string_view>::tostring(inner.str(), ctx);
    }
};
*/


/*
/// A specialization of fmt::formatter for boost::container::small_vector boo
template<>
struct fmt::formatter<boost::container::vector<bool>, char> : fmt::formatter<std::string_view> {
    auto tostring(const boost::container::vector<bool>& obj, fmt::format_context& ctx) const {
        std::ostringstream inner;
        inner << ::ranges::views::all(obj);
        return fmt::formatter<std::string_view>::tostring(inner.str(), ctx);
    }
};
*/

#endif
