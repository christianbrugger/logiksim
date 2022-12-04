#ifndef LOGIKSIM_FORMATTERS_H
#define LOGIKSIM_FORMATTERS_H

#include <boost/container/vector.hpp>
#include <boost/container/small_vector.hpp>

#include <range/v3/core.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/join.hpp>

#include <fmt/format.h>

#include <iostream>
#include <format>
#include <sstream>



/// A specialization of fmt::formatter for boost::container::small_vector
template<typename T, std::size_t N, class CharT>
struct fmt::formatter<boost::container::small_vector<T, N>, CharT> : fmt::formatter<std::string_view> {
    auto format(const boost::container::small_vector<T, N>& obj, fmt::format_context& ctx) {
        std::ostringstream inner;
        inner << ::ranges::views::all(obj);
        return fmt::formatter<std::string_view>::format(inner.str(), ctx);
    }
};


/// A specialization of fmt::formatter for boost::container::vector
template<typename T, class CharT>
struct fmt::formatter<boost::container::vector<T>, CharT> : fmt::formatter<std::string_view> {
    auto format(const boost::container::vector<T>& obj, fmt::format_context& ctx) {
        std::ostringstream inner;
        inner << ::ranges::views::all(obj);
        return fmt::formatter<std::string_view>::format(inner.str(), ctx);
    }
};

#endif
