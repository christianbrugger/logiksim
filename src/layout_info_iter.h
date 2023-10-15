#ifndef LOGICSIM_LAYOUT_INFO_ITER_H
#define LOGICSIM_LAYOUT_INFO_ITER_H

#include "iterator_adaptor/enumerate.h"
#include "iterator_adaptor/transform_view.h"
#include "vocabulary/connection_id.h"
#include "vocabulary/connector_info.h"
#include "vocabulary/layout_info_small_vector.h"

namespace logicsim {

struct layout_calculation_data_t;

/**
 * @brief: Returns vector of simple_input_info_t
 */
[[nodiscard]] auto iter_input_location(const layout_calculation_data_t &data)
    -> inputs_vector;

/**
 * @brief: Returns vector of simple_output_info_t
 */
[[nodiscard]] auto iter_output_location(const layout_calculation_data_t &data)
    -> outputs_vector;

/**
 * @brief: Returns vector of body points, type point_t.
 */
[[nodiscard]] auto iter_element_body_points(const layout_calculation_data_t &data)
    -> body_points_vector;

/**
 * @brief: Returns range of extended_input_info_t
 */
[[nodiscard]] inline auto iter_input_location_and_id(
    const layout_calculation_data_t &data);

/**
 * @brief: Returns range of extended_output_info_t
 */
[[nodiscard]] inline auto iter_output_location_and_id(
    const layout_calculation_data_t &data);

//
// Implementation
//

[[nodiscard]] inline auto iter_input_location_and_id(
    const layout_calculation_data_t &data) {
    return transform_view(
        enumerate<connection_id_t>(iter_input_location(data)),
        [](auto pair) { return extend_input_info(pair.first, pair.second); });
}

[[nodiscard]] inline auto iter_output_location_and_id(
    const layout_calculation_data_t &data) {
    return transform_view(
        enumerate<connection_id_t>(iter_output_location(data)),
        [](auto pair) { return extend_output_info(pair.first, pair.second); });
}

}  // namespace logicsim

#endif
