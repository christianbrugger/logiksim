#include "logic_item/layout_standard_element.h"

#include "geometry/connection_count.h"

namespace logicsim {

namespace standard_element {

auto height(connection_count_t input_count) -> grid_t {
    return to_grid((input_count - connection_count_t {1}));
}

}  // namespace standard_element

}  // namespace logicsim
