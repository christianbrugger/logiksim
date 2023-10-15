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
[[nodiscard]] auto input_locations(const layout_calculation_data_t &data)
    -> inputs_vector;

/**
 * @brief: Returns vector of simple_output_info_t
 */
[[nodiscard]] auto output_locations(const layout_calculation_data_t &data)
    -> outputs_vector;

/**
 * @brief: Returns vector of body points, type point_t.
 */
[[nodiscard]] auto element_body_points(const layout_calculation_data_t &data)
    -> body_points_vector;

/**
 * @brief: Returns range of extended_input_info_t
 */
[[nodiscard]] inline auto input_locations_and_id(const layout_calculation_data_t &data);

/**
 * @brief: Returns range of extended_output_info_t
 */
[[nodiscard]] inline auto output_locations_and_id(const layout_calculation_data_t &data);

//
// Implementation
//

[[nodiscard]] inline auto input_locations_and_id(const layout_calculation_data_t &data) {
    return transform_view(
        enumerate<connection_id_t>(input_locations(data)),
        [](auto pair) { return extend_input_info(pair.first, pair.second); });
}

[[nodiscard]] inline auto output_locations_and_id(const layout_calculation_data_t &data) {
    return transform_view(
        enumerate<connection_id_t>(output_locations(data)),
        [](auto pair) { return extend_output_info(pair.first, pair.second); });
}

}  // namespace logicsim

#endif
