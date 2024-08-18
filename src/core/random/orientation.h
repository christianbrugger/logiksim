#ifndef LOGICSIM_RANDOM_ORIENTATION_H
#define LOGICSIM_RANDOM_ORIENTATION_H

#include "random/generator.h"
#include "vocabulary/logicitem_type.h"
#include "vocabulary/orientation.h"

namespace logicsim {

[[nodiscard]] auto get_random_orientation(Rng &rng) -> orientation_t;

[[nodiscard]] auto get_random_directed_orientation(Rng &rng) -> orientation_t;

[[nodiscard]] auto get_random_orientation(Rng &rng,
                                          LogicItemType logicitem_type) -> orientation_t;

}  // namespace logicsim

#endif
