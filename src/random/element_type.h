#ifndef LOGICSIM_RANDOM_ELEMENT_TYPE_H
#define LOGICSIM_RANDOM_ELEMENT_TYPE_H

#include "random/generator.h"
#include "vocabulary/element_type.h"

namespace logicsim {

/**
 * @brief: Return random element type that is a logic item, except sub-circuits.
 *
 * Each type has equal probability.
 */
[[nodiscard]] auto get_random_logic_item_type(Rng& rng) -> ElementType;

}  // namespace logicsim

#endif
