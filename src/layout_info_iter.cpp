#include "layout_info_iter.h"

#include "geometry/layout_calculation.h"
#include "logic_item/layout.h"
#include "vocabulary/layout_calculation_data.h"

namespace logicsim {

auto input_locations(const layout_calculation_data_t &data) -> inputs_vector {
    auto connectors = input_locations_base(data);

    for (auto &connector : connectors) {
        connector = simple_input_info_t {
            .position = transform(data.position, data.orientation, connector.position),
            .orientation = transform(data.orientation, connector.orientation),
        };
    }

    return connectors;
}

auto output_locations(const layout_calculation_data_t &data) -> outputs_vector {
    auto connectors = output_locations_base(data);

    for (auto &connector : connectors) {
        connector = simple_output_info_t {
            .position = transform(data.position, data.orientation, connector.position),
            .orientation = transform(data.orientation, connector.orientation),
        };
    }

    return connectors;
}

auto element_body_points(const layout_calculation_data_t &data)
    -> body_points_vector {
    auto body_points = element_body_points_base(data);

    for (auto &point : body_points) {
        point = transform(data.position, data.orientation, point);
    }

    return body_points;
}

}  // namespace logicsim
