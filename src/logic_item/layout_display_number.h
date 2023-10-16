#ifndef LOGICSIM_LOGIC_ITEM_LAYOUT_DISPLAY_NUMBER_H
#define LOGICSIM_LOGIC_ITEM_LAYOUT_DISPLAY_NUMBER_H

#include "vocabulary/connection_count.h"
#include "vocabulary/layout_info_vector.h"

namespace logicsim {

struct grid_t;
struct point_t;
struct layout_calculation_data_t;

namespace display_number {

constexpr static inline auto control_inputs = connection_count_t {2};
constexpr static inline auto min_value_inputs = connection_count_t {1};
constexpr static inline auto max_value_inputs = connection_count_t {64};
constexpr static inline auto default_value_inputs = connection_count_t {3};

constexpr static inline auto min_inputs = control_inputs + min_value_inputs;
constexpr static inline auto max_inputs = control_inputs + max_value_inputs;
constexpr static inline auto default_inputs = control_inputs + default_value_inputs;

[[nodiscard]] auto value_inputs(connection_count_t input_count) -> connection_count_t;
[[nodiscard]] auto width(connection_count_t input_count) -> grid_t;
[[nodiscard]] auto height(connection_count_t input_count) -> grid_t;

[[nodiscard]] auto input_shift(connection_count_t input_count) -> grid_t;
[[nodiscard]] auto enable_position(connection_count_t input_count) -> point_t;
[[nodiscard]] auto negative_position(connection_count_t input_count) -> point_t;
constexpr static inline auto negative_input_id = connection_id_t {1};

/**
 * @brief: Vector of the inputs of number display elements
 *         not considering position or orientation.
 */
[[nodiscard]] auto input_locations_base(const layout_calculation_data_t& data)
    -> inputs_vector;

/**
 * @brief: Vector of the outputs of number display elements
 *         not considering position or orientation.
 */
[[nodiscard]] auto output_locations_base(const layout_calculation_data_t& data)
    -> outputs_vector;

/**
 * @brief: Vector of the body points of number display elements
 *         not considering position or orientation.
 */
[[nodiscard]] auto element_body_points_base(const layout_calculation_data_t& data)
    -> body_points_vector;

}  // namespace display_number

}  // namespace logicsim

#endif
