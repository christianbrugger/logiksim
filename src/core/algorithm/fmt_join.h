#ifndef LOGICSIM_ALGORITHM_FMT_JOIN_H
#define LOGICSIM_ALGORITHM_FMT_JOIN_H

#include "core/iterator_adaptor/transform_view.h"

#include <boost/algorithm/string/join.hpp>
#include <fmt/core.h>

#include <functional>
#include <string_view>

namespace logicsim {

/**
 * @brief: Format each projected element of the container with given separator.
 */
template <typename T, class Proj = std::identity>
    requires requires(T container) {
        std::begin(container);
        std::end(container);
    }
[[nodiscard]] constexpr auto fmt_join(std::string_view sep, T &&obj,
                                      std::string_view fmt = "{}", Proj proj = {}) {
    auto format_func = [&fmt, proj](auto &&item) {
        return fmt::format(fmt::runtime(fmt),
                           std::invoke(proj, std::forward<decltype(item)>(item)));
    };

    // TODO refactor in c++23 to use std::views::join_with and std::views::transform
    // use our own transform view, as boost does not work with std::views::transform
    return boost::join(transform_view(std::forward<T>(obj), format_func), sep);
}

}  // namespace logicsim

#endif
