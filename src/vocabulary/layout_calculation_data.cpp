#include "vocabulary/layout_calculation_data.h"

#include <fmt/core.h>

namespace logicsim {

auto layout_calculation_data_t::format() const -> std::string {
    return fmt::format(
        "layout_calculation_data_t("
        "element_type={}, position={}, input_count={}, "
        "output_count={}, orientation={}, internal_state_count={}"
        ")",
        element_type, position, input_count, output_count, orientation,
        internal_state_count);
}

}  // namespace logicsim
