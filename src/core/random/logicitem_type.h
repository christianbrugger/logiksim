#ifndef LOGICSIM_RANDOM_LOGICITEM_TYPE_H
#define LOGICSIM_RANDOM_LOGICITEM_TYPE_H

#include "core/random/generator.h"
#include "core/vocabulary/logicitem_type.h"

namespace logicsim {

/**
 * @brief: Return random element type that is a logic item, except sub-circuits.
 *
 * Each type has equal probability.
 */
[[nodiscard]] auto get_random_logicitem_type(Rng& rng) -> LogicItemType;

}  // namespace logicsim

#endif
