#include "random/layout_calculation_data.h"

#include "layout_info.h"
#include "random/connection_count.h"
#include "random/logicitem_type.h"
#include "random/internal_state_count.h"
#include "random/orientation.h"
#include "random/point.h"
#include "vocabulary/layout_calculation_data.h"

namespace logicsim {

auto get_random_layout_calculation_data(Rng& rng) -> layout_calculation_data_t {
    return get_random_layout_calculation_data(rng, grid_t::min(), grid_t::max());
}

auto get_random_layout_calculation_data(Rng& rng, grid_t min, grid_t max)
    -> layout_calculation_data_t {
    const auto type = get_random_logic_item_type(rng);

    const auto data = layout_calculation_data_t {
        .internal_state_count = get_random_internal_state_count(rng, type),
        .position = get_random_point(rng, min, max),
        .input_count = get_random_input_count(rng, type),
        .output_count = get_random_output_count(rng, type),
        .orientation = get_random_orientation(rng, type),
        .logicitem_type = type,
    };

    if (!is_valid(data)) {
        return get_random_layout_calculation_data(rng, min, max);
    }

    return data;
}

}  // namespace logicsim
