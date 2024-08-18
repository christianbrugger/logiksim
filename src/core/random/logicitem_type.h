#ifndef LOGICSIM_RANDOM_LOGICITEM_TYPE_H
#define LOGICSIM_RANDOM_LOGICITEM_TYPE_H

#include "random/generator.h"
#include "vocabulary/logicitem_type.h"

namespace logicsim {

/**
 * @brief: Return random element type that is a logic item, except sub-circuits.
 *
 * Each type has equal probability.
 */
[[nodiscard]] auto get_random_logic_item_type(Rng& rng) -> LogicItemType;

}  // namespace logicsim

#endif
