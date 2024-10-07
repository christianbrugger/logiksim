#ifndef LOGICSIM_CORE_LOGIC_ITEM_RENDER_RENDER_LOGICITEM_LAYER_H
#define LOGICSIM_CORE_LOGIC_ITEM_RENDER_RENDER_LOGICITEM_LAYER_H

#include "vocabulary/logicitem_type.h"

namespace logicsim {

/**
 * @brief: Return true if the logic item shall be drawn above wires.
 */
[[nodiscard]] auto draw_logic_item_above(LogicItemType type) -> bool;

}  // namespace logicsim

#endif
