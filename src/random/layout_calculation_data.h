#ifndef LOGICSIM_RANDOM_LAYOUT_CALCULATION_DATA_H
#define LOGICSIM_RANDOM_LAYOUT_CALCULATION_DATA_H

#include "random/generator.h"
#include "vocabulary/element_type.h"

namespace logicsim {

struct grid_t;
struct layout_calculation_data_t;

/**
 * @brief: Returns a random and valid layout calculation data object of a logic item
 *         anywhere on the grid.
 */
[[nodiscard]] auto get_random_layout_calculation_data(Rng &rng)
    -> layout_calculation_data_t;

/**
 * @brief: Returns a random and valid layout calculation data object of a logic item.
 */
[[nodiscard]] auto get_random_layout_calculation_data(Rng &rng, grid_t min, grid_t max)
    -> layout_calculation_data_t;

}  // namespace logicsim

#endif
