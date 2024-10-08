#include "element/logicitem/layout_standard_element.h"

#include "algorithm/range.h"
#include "geometry/connection_count.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/point.h"

namespace logicsim {

namespace standard_element {

auto input_locations_base(const layout_calculation_data_t& data) -> inputs_vector {
    auto connectors = inputs_vector {};
    connectors.reserve(data.input_count.count());

    for (auto y : range(to_grid(data.input_count))) {
        connectors.push_back({point_t {0, y}, orientation_t::left});
    }

    return connectors;
}

auto output_locations_base(const layout_calculation_data_t& data) -> outputs_vector {
    const auto output_y = output_height(data.input_count);

    return outputs_vector {
        {point_t {2, output_y}, orientation_t::right},
    };
}

auto element_body_points_base(const layout_calculation_data_t& data)
    -> body_points_vector {
    const auto output_y = output_height(data.input_count);

    auto result = body_points_vector {};
    for (auto y : range(to_grid(data.input_count))) {
        result.push_back(point_t {1, y});

        if (y != output_y) {
            result.push_back(point_t {2, y});
        }
    }
    return result;
}

}  // namespace standard_element

}  // namespace logicsim
