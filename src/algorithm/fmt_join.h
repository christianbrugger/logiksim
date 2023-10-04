#ifndef LOGICSIM_ALGORITHM_FMT_JOIN_H
#define LOGICSIM_ALGORITHM_FMT_JOIN_H

#include "iterator_adaptor/transform_view.h"

#include <boost/algorithm/string/join.hpp>
#include <fmt/core.h>

#include <functional>
#include <string_view>

namespace logicsim {

template <typename T, class Proj = std::identity>
    requires requires(T container) {
                 std::begin(container);
                 std::end(container);
             }

/**
 * @brief: Format each projected element of the container with given separator.
 */
[[nodiscard]] constexpr auto fmt_join(std::string_view sep, const T &obj,
                                      std::string_view fmt = "{}", Proj proj = {}) {
    auto format_func = [&fmt, proj](const auto &item) {
        return fmt::format(fmt::runtime(fmt), std::invoke(proj, item));
    };
    return boost::join(transform_view(obj, format_func), sep);
}

}  // namespace logicsim

#endif
