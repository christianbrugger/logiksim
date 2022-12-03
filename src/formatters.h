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
        std::string temp;
        for (const auto& elem : obj)
            std::format_to(std::back_inserter(temp), "{}, ", elem);
        return std::formatter<string_view>::format(temp, ctx);

        //std::ostringstream inner;
        //inner << ::ranges::views::all(obj);
        //return std::formatter<string_view>::format(inner.str(), ctx

        // logicsim::SimulationEvent event { 0.1, 0, 0, true };
        // std::cout << event.format() << "\n";


        // if (std::size(obj) > 0) {
        //     std::cout << obj.at(0);
        // }
       // return std::formatter<string_view>::format("!!!", ctx);
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
