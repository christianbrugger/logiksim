#ifndef LOGIKSIM_FORMATTERS_H
#define LOGIKSIM_FORMATTERS_H

#include <boost/container/vector.hpp>
#include <boost/container/small_vector.hpp>

#include <range/v3/core.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/join.hpp>

#include <format>
#include <iostream>
#include <format>
#include <sstream>



template<typename T, std::size_t N, class CharT>
struct std::formatter<boost::container::small_vector<T, N>, CharT> : std::formatter<string_view> {
    auto format(const boost::container::small_vector<T, N>& obj, std::format_context& ctx) {
        std::ostringstream inner;
        inner << ::ranges::views::all(obj);
        return std::formatter<string_view>::format(inner.str(), ctx);
    }
};


template<typename T, class CharT>
struct std::formatter<boost::container::vector<T>, CharT> : std::formatter<string_view> {
    auto format(const boost::container::vector<T>& obj, std::format_context& ctx) {
        std::ostringstream inner;
        inner << ::ranges::views::all(obj);
        return std::formatter<string_view>::format(inner.str(), ctx);
    }
};

#endif
