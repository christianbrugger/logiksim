#include "layout_info_iter.h"

namespace logicsim {

auto iter_input_location(const layout_calculation_data_t &data) -> inputs_vector {
    auto connectors = iter_input_location_base(data);

    for (auto &connector : connectors) {
        connector = simple_input_info_t {
            .position = transform(data.position, data.orientation, connector.position),
            .orientation = transform(data.orientation, connector.orientation),
        };
    }

    return connectors;
}

auto iter_output_location(const layout_calculation_data_t &data) -> outputs_vector {
    auto connectors = iter_output_location_base(data);

    for (auto &connector : connectors) {
        connector = simple_output_info_t {
            .position = transform(data.position, data.orientation, connector.position),
            .orientation = transform(data.orientation, connector.orientation),
        };
    }

    return connectors;
}

auto iter_element_body_points(const layout_calculation_data_t &data)
    -> body_points_vector {
    auto body_points = iter_element_body_points_base(data);

    for (auto &point : body_points) {
        point = transform(data.position, data.orientation, point);
    }

    return body_points;
}

}  // namespace logicsim
