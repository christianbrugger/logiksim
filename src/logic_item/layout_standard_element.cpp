#include "logic_item/layout_standard_element.h"

#include "geometry/connection_count.h"

namespace logicsim {

namespace standard_element {

auto iter_element_body_points_smallvector_private(const layout_calculation_data_t& data)
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
