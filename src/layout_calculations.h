#ifndef LOGIKSIM_LAYOUT_CALCULATIONS_H
#define LOGIKSIM_LAYOUT_CALCULATIONS_H

#include "layout.h"
#include "schematic.h"
#include "vocabulary.h"

#include <vector>

namespace logicsim {

[[nodiscard]] auto output_locations(element_id_t element_id) -> std::vector<point_t>;
[[nodiscard]] auto input_locations(element_id_t element_id) -> std::vector<point_t>;

}  // namespace logicsim

#endif