#ifndef LOGICSIM_LOGIC_ITEM_LAYOUT_STANDARD_ELEMENT_H
#define LOGICSIM_LOGIC_ITEM_LAYOUT_STANDARD_ELEMENT_H

#include "core/geometry/connection_count.h"
#include "core/vocabulary/connection_count.h"
#include "core/vocabulary/grid.h"
#include "core/vocabulary/layout_info_vector.h"

namespace logicsim {

struct layout_calculation_data_t;

namespace standard_element {

constexpr static inline auto min_inputs = connection_count_t {2};
constexpr static inline auto max_inputs = connection_count_t {128};
constexpr static inline auto default_inputs = connection_count_t {2};

constexpr static inline auto width = grid_t {2};

/**
 * @brief: Returns the dynamic height of the standard element.
 */
[[nodiscard]] constexpr inline auto height(connection_count_t input_count) -> grid_t {
    return to_grid((input_count - connection_count_t {1}));
}

/**
 * @brief: Returns the dynamic y-coordinate of the output of the standard element.
 */
[[nodiscard]] constexpr inline auto output_height(connection_count_t input_count)
    -> grid_t {
    return height(input_count) / 2;
}

/**
 * @brief: Vector of the inputs of standard elements
 *         not considering position or orientation.
 */
[[nodiscard]] auto input_locations_base(const layout_calculation_data_t& data)
    -> inputs_vector;

/**
 * @brief: Vector of the outputs of standard elements
 *         not considering position or orientation.
 */
[[nodiscard]] auto output_locations_base(const layout_calculation_data_t& data)
    -> outputs_vector;

/**
 * @brief: Vector of the body points of standard elements
 *         not considering position or orientation.
 */
[[nodiscard]] auto element_body_points_base(const layout_calculation_data_t& data)
    -> body_points_vector;

}  // namespace standard_element

}  // namespace logicsim

#endif
