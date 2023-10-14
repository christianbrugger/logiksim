#include "logic_item/layout_standard_element.h"

#include "geometry/connection_count.h"

namespace logicsim {

namespace standard_element {

auto iter_input_location_base(const layout_calculation_data_t& data)
    -> connectors_vector {
    auto connectors = connectors_vector {};
    connectors.reserve(data.input_count.count());

    for (auto y : range(to_grid(data.input_count))) {
        connectors.push_back({point_t {0, y}, orientation_t::left});
    }

    return connectors;
}

auto iter_output_location_base(const layout_calculation_data_t& data)
    -> connectors_vector {
    const auto output_y = output_height(data.input_count);

    return connectors_vector {
        {point_t {2, output_y}, orientation_t::right},
    };
}

auto iter_element_body_points_base(const layout_calculation_data_t& data)
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
