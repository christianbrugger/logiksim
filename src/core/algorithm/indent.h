#ifndef LOGICSIM_CORE_ALGORITHM_INDENT_H
#define LOGICSIM_CORE_ALGORITHM_INDENT_H

#include "core/iterator_adaptor/transform_view.h"

#include <boost/algorithm/string/join.hpp>
#include <fmt/core.h>
#include <gsl/gsl>

#include <ranges>
#include <string>
#include <string_view>

namespace logicsim {

[[nodiscard]] auto indent(std::string_view input, int count) -> std::string {
    Expects(count >= 0);
    if (count == 0) {
        return std::string {input.begin(), input.end()};
    }

    const auto add_indent = [count](auto &&line) -> std::string {
        return fmt::format("{:{}}{}", "", count,
                           std::string_view {line.begin(), line.end()});
    };

    auto split = std::views::split(input, '\n');

    // TODO refactor in c++23 to use std::views::join_with and std::views::transform
    // use our own transform view, as boost does not work with std::views::transform
    return boost::join(transform_view(split, add_indent), "\n");
}

}  // namespace logicsim

#endif
