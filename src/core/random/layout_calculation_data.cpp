#include "core/random/layout_calculation_data.h"

#include "core/layout_info.h"
#include "core/random/connection_count.h"
#include "core/random/internal_state_count.h"
#include "core/random/logicitem_type.h"
#include "core/random/orientation.h"
#include "core/random/point.h"
#include "core/vocabulary/layout_calculation_data.h"

namespace logicsim {

auto get_random_layout_calculation_data(Rng& rng) -> layout_calculation_data_t {
    return get_random_layout_calculation_data(rng, grid_t::min(), grid_t::max());
}

auto get_random_layout_calculation_data(Rng& rng, grid_t min, grid_t max)
    -> layout_calculation_data_t {
    const auto type = get_random_logicitem_type(rng);

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
