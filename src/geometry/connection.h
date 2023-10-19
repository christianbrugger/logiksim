#ifndef LOGICSIM_GEOMETRY_CONNECTION_H
#define LOGICSIM_GEOMETRY_CONNECTION_H

#include "vocabulary/connection.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/element_id.h"

#include <ranges>

namespace logicsim {

constexpr auto inputs(element_id_t element_id, connection_count_t input_count) {
    return std::ranges::views::transform(
        std::ranges::views::iota(std::size_t {0}, std::size_t {input_count}),
        [element_id](const std::size_t &v) -> input_t {
            return input_t {
                element_id,
                connection_id_t {static_cast<connection_id_t::value_type>(v)},
            };
        });
}

constexpr auto outputs(element_id_t element_id, connection_count_t output_count) {
    return std::ranges::views::transform(
        std::ranges::views::iota(std::size_t {0}, std::size_t {output_count}),
        [element_id](const std::size_t &v) -> output_t {
            return output_t {
                element_id,
                connection_id_t {static_cast<connection_id_t::value_type>(v)},
            };
        });
}

}  // namespace logicsim

#endif
