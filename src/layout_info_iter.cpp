#include "layout_info_iter.h"

namespace logicsim {

namespace {
auto transform_connectors_inplace(const layout_calculation_data_t &data,
                                  connectors_vector &connectors) {
    for (auto &connector : connectors) {
        connector = connector_info_t {
            .position = transform(data.position, data.orientation, connector.position),
            .orientation = transform(data.orientation, connector.orientation),
        };
    }
}
}  // namespace

auto iter_input_location(const layout_calculation_data_t &data) -> connectors_vector {
    auto connectors = iter_input_location_base(data);
    transform_connectors_inplace(data, connectors);
    return connectors;
}

auto iter_output_location(const layout_calculation_data_t &data) -> connectors_vector {
    auto connectors = iter_output_location_base(data);
    transform_connectors_inplace(data, connectors);
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

// auto iter_input_location_and_id(const layout_calculation_data_t &data) -> bool {
//     return iter_input_location(
//         data, [&, input_id = connection_id_t {0}](point_t position,
//                                                   orientation_t orientation) mutable {
//             return std::invoke(next_input, input_id++, position, orientation);
//         });
// }
//
// auto iter_output_location_and_id(const layout_calculation_data_t &data) -> bool {
//     return iter_output_location(
//         data, [&, output_id = connection_id_t {0}](point_t position,
//                                                    orientation_t orientation) mutable {
//             return std::invoke(next_output, output_id++, position, orientation);
//         });
// }

}  // namespace logicsim
